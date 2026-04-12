// ============================================================
// main.cpp -- BalanceBot: Modulare Hauptschleife mit Zustandsmaschine
//
// Zustaende: INIT --> CALIBRATING --> IDLE --> BALANCING --> FALLEN --> E_STOP
//
// Serial-Befehle (ein Zeichen, Newline):
//   's'  --> IDLE nach BALANCING (start)
//   'q'  --> BALANCING nach IDLE (quit)
//   'r'  --> E_STOP nach INIT (reset, startet Initialisierung neu)
//   'h'  --> CSV-Header erneut ausgeben
//   'p'  --> Parameter ausgeben (Kp, Ki, Kd, Setpoint)
//
// TODO: Erweiterung um serielle Parameter-Aenderung (z.B. "Kp=3.5")
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <ArduinoMotorCarrier.h>

#include "config.h"
#include "state_machine.h"
#include "imu.h"
#include "motor.h"
#include "encoder.h"
#include "pid.h"
#include "logger.h"

// ------------------------------------------------------------
// Globale Objekte
// ------------------------------------------------------------

Imu        imu;
MotorPair  motors;
Encoder    enc;

// Winkel-PID (innerer Regler)
Pid pidAngle(
    KP_ANGLE, KI_ANGLE, KD_ANGLE, KAW_ANGLE,
    INTEGRAL_MAX_ANGLE,
    -(float)MAX_DUTY, (float)MAX_DUTY
);

// Position-PID (aeusserer Regler -- erst nach stabilem Angle-PD aktivieren)
Pid pidPos(
    KP_POS, KI_POS, KD_POS, KAW_POS,
    20.0f, -MAX_POS_CORRECTION_DEG, MAX_POS_CORRECTION_DEG
);

// ------------------------------------------------------------
// Zustandsmaschine
// ------------------------------------------------------------

State        currentState = State::INIT;
unsigned long fallenStartMs = 0; // Zeitstempel des Eintritts in FALLEN

// Zustandswechsel mit Logging
void changeState(State next) {
    if (next == currentState) return;
    Logger::printStateChange(currentState, next);
    currentState = next;
}

// ------------------------------------------------------------
// Serial-Befehl lesen (non-blocking)
// Gibt '\0' zurueck wenn keine neuen Daten
// ------------------------------------------------------------

char readSerialCommand() {
    if (Serial.available() > 0) {
        return (char)Serial.read();
    }
    return '\0';
}

// ------------------------------------------------------------
// Parameter-Ausgabe fuer Debugging
// ------------------------------------------------------------

void printParams() {
    Logger::printComment("--- Parameter ---");
    Serial.print("# Kp=");  Serial.print(KP_ANGLE);
    Serial.print(" Ki=");   Serial.print(KI_ANGLE);
    Serial.print(" Kd=");   Serial.print(KD_ANGLE);
    Serial.print(" SP=");   Serial.println(SETPOINT_ANGLE_DEG);
    Serial.print("# MAX_DUTY="); Serial.println(MAX_DUTY);
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10); // Auf Serial-Monitor warten

    Logger::printComment("BalanceBot startet...");

    // Motor Carrier initialisieren
    if (!controller.begin()) {
        Logger::printComment("FEHLER: Motor Carrier nicht gefunden!");
        changeState(State::E_STOP);
        return;
    }
    controller.reboot();
    delay(500);
    motors.begin();

    // IMU initialisieren
    if (!imu.begin()) {
        Logger::printComment("FEHLER: BNO055 nicht gefunden!");
        motors.stop();
        changeState(State::E_STOP);
        return;
    }

    // Initialisierung erfolgreich --> Kalibrierung starten
    changeState(State::CALIBRATING);
    Logger::printComment("Warte auf IMU-Kalibrierung... (Gyro und Accel Stufe 2)");
    Logger::printHeader();
}

// ------------------------------------------------------------
// Hauptschleife
// ------------------------------------------------------------

void loop() {
    // --- Timing ---
    static unsigned long prevMs = 0;
    unsigned long nowMs = millis();
    unsigned long elapsedMs = nowMs - prevMs;

    // Regelzyklus einhalten: nicht vor LOOP_PERIOD_MS
    if (elapsedMs < LOOP_PERIOD_MS) return;
    float dt = (float)elapsedMs / 1000.0f; // [s]
    prevMs = nowMs;

    // --- Serial-Befehle lesen ---
    char cmd = readSerialCommand();
    if (cmd == 'h') { Logger::printHeader(); }
    if (cmd == 'p') { printParams(); }

    // --- IMU immer lesen (fuer Sicherheitscheck) ---
    imu.update();
    float pitch = imu.pitch();

    // --- Encoder lesen ---
    enc.update(dt);

    // ============================================================
    // Zustandsmaschine
    // ============================================================
    switch (currentState) {

        // ---------------------------------------------------------
        case State::INIT:
            // Sollte nach setup() nicht mehr auftreten
            // Falls E_STOP --> INIT per 'r' Reset:
            if (cmd == 'r') {
                // Erneuter Initialisierungsversuch
                motors.stop();
                motors.resetStall();
                enc.reset();
                pidAngle.reset();
                pidPos.reset();
                if (controller.begin() && imu.begin()) {
                    changeState(State::CALIBRATING);
                } else {
                    Logger::printComment("FEHLER: Reset fehlgeschlagen");
                }
            }
            break;

        // ---------------------------------------------------------
        case State::CALIBRATING:
            // Warten bis IMU kalibriert ist
            if (imu.isCalibrated()) {
                Logger::printComment("IMU kalibriert. 's' druecken zum Starten.");
                changeState(State::IDLE);
            }
            // Sicherheit: auch bei Kalibrierung auf grossen Winkel reagieren
            if (abs(pitch) > FALLEN_ANGLE_DEG) {
                motors.stop();
                changeState(State::FALLEN);
            }
            break;

        // ---------------------------------------------------------
        case State::IDLE:
            motors.stop();

            if (cmd == 's') {
                // Startbefehl: PID zuruecksetzen und balancieren starten
                pidAngle.reset();
                pidPos.reset();
                enc.reset();
                motors.resetStall();
                if (abs(pitch) < RECOVERY_ANGLE_DEG) {
                    changeState(State::BALANCING);
                } else {
                    Logger::printComment("FEHLER: Zu grosser Startwinkel -- aufrecht hinstellen!");
                }
            }

            if (abs(pitch) > FALLEN_ANGLE_DEG) {
                changeState(State::FALLEN);
            }
            break;

        // ---------------------------------------------------------
        case State::BALANCING: {
            // --- Sicherheitscheck: Kippwinkel ---
            if (abs(pitch) > FALLEN_ANGLE_DEG) {
                motors.stop();
                changeState(State::FALLEN);
                break;
            }

            // --- Quit-Befehl ---
            if (cmd == 'q') {
                motors.stop();
                changeState(State::IDLE);
                break;
            }

            // --- Aeusserer Regler: Position --> Winkel-Sollwert ---
            // TODO: Positionsregler erst aktivieren wenn Winkelregler stabil ist!
            // Vorlaeufig: Winkel-Sollwert fix aus config.h
            float angleSp = SETPOINT_ANGLE_DEG;
            // Wenn Positionsregler aktiv (KP_POS > 0):
            // float posError = 0.0f - enc.positionM();  // Soll: an Ort bleiben
            // float posCorrection = pidPos.compute(posError, dt);
            // angleSp = SETPOINT_ANGLE_DEG + posCorrection;

            // --- Innerer Regler: Winkel --> Duty ---
            float angleError = angleSp - pitch;
            float duty_f     = pidAngle.compute(angleError, dt);
            int   duty       = (int)duty_f;

            // --- Motoren ansteuern ---
            motors.setDrive(duty);

            // --- Stall-Erkennung ---
            if (motors.checkStall(abs(enc.speedCps()))) {
                Logger::printComment("STALL erkannt --> E_STOP");
                motors.stop();
                changeState(State::E_STOP);
                break;
            }

            // --- Logging ---
            Logger::printData(
                nowMs,
                currentState,
                pitch,
                imu.pitchRate(),
                pidAngle.pTerm(),
                pidAngle.iTerm(),
                pidAngle.dTerm(),
                motors.lastDuty(),
                enc.position(),
                enc.positionM()
            );
            break;
        }

        // ---------------------------------------------------------
        case State::FALLEN:
            motors.stop();

            if (fallenStartMs == 0) {
                fallenStartMs = nowMs; // Eintrittszeit merken
            }

            // Warten bis Wartezeit abgelaufen und Bot aufgerichtet
            if ((nowMs - fallenStartMs) >= FALLEN_WAIT_MS) {
                if (abs(pitch) < RECOVERY_ANGLE_DEG) {
                    fallenStartMs = 0;
                    pidAngle.reset();
                    changeState(State::IDLE);
                }
            }
            break;

        // ---------------------------------------------------------
        case State::E_STOP:
            // Motoren AUS -- bleibt hier bis manueller Reset ('r')
            motors.stop();

            if (cmd == 'r') {
                changeState(State::INIT);
            }
            break;
    }
}
