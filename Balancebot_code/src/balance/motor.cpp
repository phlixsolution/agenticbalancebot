// ============================================================
// motor.cpp -- BalanceBot: Motor-Implementierung
// ============================================================

#include "motor.h"
#include "config.h"
#include <ArduinoMotorCarrier.h>

void MotorPair::begin() {
    // Startposition: beide Motoren aus
    M1.setDuty(0);
    M2.setDuty(0);
    _lastDuty = 0;
    resetStall();
}

void MotorPair::setDrive(int duty) {
    // --- Slew-Rate-Begrenzung ---
    // Duty darf sich pro Schritt hoechstens um DUTY_SLEW_RATE aendern.
    // Verhindert Strom-Spikes bei abrupten Richtungswechseln.
    int delta = duty - _lastDuty;
    if (delta >  DUTY_SLEW_RATE) duty = _lastDuty + DUTY_SLEW_RATE;
    if (delta < -DUTY_SLEW_RATE) duty = _lastDuty - DUTY_SLEW_RATE;

    // --- Absolute Begrenzung (Treiberschutz) ---
    // MAX_DUTY ist so gewaehlt, dass Stallstrom unter Treibergrenze bleibt
    if (duty >  MAX_DUTY) duty =  MAX_DUTY;
    if (duty < -MAX_DUTY) duty = -MAX_DUTY;

    _lastDuty = duty;
    _applyToMotors(duty);
}

void MotorPair::stop() {
    // Sofortiger Stop ohne Slew-Rate (fuer Sicherheitszustaende)
    _lastDuty = 0;
    M1.setDuty(0);
    M2.setDuty(0);
    controller.ping(); // Watchdog-Ping benoetigt
}

void MotorPair::_applyToMotors(int duty) {
    // Totzonenkompensation:
    // Duty-Werte unterhalb der Totzone werden auf 0 gesetzt,
    // da die Motoren sonst nicht anlaufen wuerden.
    // TODO: Totzonen pro Motor separat messen (deadzone_id-Sketch)!
    int m1Duty, m2Duty;
    if (duty == 0) {
        // Explizit stoppen
        m1Duty = 0;
        m2Duty = 0;
    } else if (abs(duty) < DEADZONE_DUTY) {
        // Zu schwaches Signal -- auf Totzonenrand hochsetzen
        int sign = (duty > 0) ? 1 : -1;
        m1Duty = -sign * DEADZONE_DUTY; // M1 dreht invertiert (Einbaulage)
        m2Duty =  sign * DEADZONE_DUTY;
    } else {
        // Normale Ansteuerung
        // Vorwaerts (duty > 0): M1 bekommt negatives Signal (Einbaulage rechte Seite)
        m1Duty = -duty;
        m2Duty =  duty;
    }

    M1.setDuty(m1Duty);
    M2.setDuty(m2Duty);
    controller.ping(); // Motor Carrier Watchdog-Ping (benoetigt ca. alle 100ms)
}

bool MotorPair::checkStall(int encoderSpeedCps) {
    // Stall-Bedingung: hoher Duty bei zu geringer Encoder-Geschwindigkeit
    bool condition = (abs(_lastDuty) >= STALL_DUTY_THRESHOLD) &&
                     (encoderSpeedCps <= STALL_SPEED_THRESHOLD);

    if (condition) {
        if (!_stallActive) {
            // Stall-Bedingung beginnt gerade -- Startzeit merken
            _stallActive  = true;
            _stallStartMs = millis();
        }
        // Stall-Timeout pruefen
        if ((millis() - _stallStartMs) >= (unsigned long)STALL_TIMEOUT_MS) {
            return true; // Stall bestaetigt --> E_STOP ausloesen
        }
    } else {
        // Bedingung nicht mehr erfuellt -- Zaehler zuruecksetzen
        _stallActive = false;
    }

    return false; // Kein Stall
}

void MotorPair::resetStall() {
    _stallActive  = false;
    _stallStartMs = 0;
}
