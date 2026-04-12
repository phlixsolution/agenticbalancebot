#pragma once

// ============================================================
// config.h -- BalanceBot: Alle konfigurierbaren Parameter
//
// Alle Tuning-Parameter, Schwellwerte und Systemkonstanten
// stehen hier zentral. Nichts davon soll im Steuercode
// als Magic Number auftauchen.
// ============================================================

// ------------------------------------------------------------
// Mechanik
// ------------------------------------------------------------

// Raddurchmesser [m]
constexpr float WHEEL_DIAMETER_M       = 0.090f;
// Radradius [m]
constexpr float WHEEL_RADIUS_M         = WHEEL_DIAMETER_M / 2.0f;
// Radumfang [m]
constexpr float WHEEL_CIRCUMFERENCE_M  = 3.14159265f * WHEEL_DIAMETER_M;

// Encoder-Ticks pro Radumdrehung (inkl. 100:1 Getriebe, Quadratur)
constexpr int   COUNTS_PER_WHEEL_REV   = 1200;
// Meter pro Count
constexpr float METERS_PER_COUNT       = WHEEL_CIRCUMFERENCE_M / COUNTS_PER_WHEEL_REV;

// Schwerpunkthöhe über Achsmitte [m]
// TODO: Durch Messung (Knife-Edge-Test) bestimmen!
// Vorlaeufige Schaetzung: Carrier bei ~75mm, Achse bei 45mm --> ~30mm
constexpr float COG_HEIGHT_M           = 0.030f;

// Gesamtmasse [kg]
constexpr float TOTAL_MASS_KG          = 0.204f;

// ------------------------------------------------------------
// Abtastzeit
// ------------------------------------------------------------

// Soll-Abtastzeit der Regelschleife [ms]
// 10 ms = 100 Hz. Nicht unter 10 ms gehen (Bandbreite des inversen Pendels)
constexpr unsigned long LOOP_PERIOD_MS = 10;

// ------------------------------------------------------------
// Motoren / Leistungselektronik
// ------------------------------------------------------------

// Maximaler Duty-Cycle [%]
// Motorstall: 1.03 A, Treibergrenze: 0.50 A
// 0.50 / 1.03 * 100 = 48.5 %  --> konservativ auf 45 % begrenzt
// Waehrend der ersten Inbetriebnahme: noch konservativer beginnen (z.B. 30)
constexpr int   MAX_DUTY               = 45;

// Mindest-Duty fuer Loslaufen (Totzone-Kompensation) [%]
// TODO: Durch deadzone_id-Test bestimmen!
constexpr int   DEADZONE_DUTY          = 10;

// Maximale Aenderung des Duty-Cycles pro Abtastschritt (Slew-Rate) [%/Schritt]
constexpr int   DUTY_SLEW_RATE         = 20;

// Stall-Erkennung:
// Wenn abs(duty) >= STALL_DUTY_THRESHOLD und abs(speed) <= STALL_SPEED_THRESHOLD
// fuer STALL_TIMEOUT_MS ms --> E_STOP ausloesen
constexpr int   STALL_DUTY_THRESHOLD   = 30;    // Duty-Wert ab dem Stall moeglich
constexpr int   STALL_SPEED_THRESHOLD  = 5;     // Encoder-Counts/s unter denen "zu langsam"
constexpr int   STALL_TIMEOUT_MS       = 500;   // Zeit bis Stall-E_STOP [ms]

// ------------------------------------------------------------
// Zustandsmaschine / Sicherheit
// ------------------------------------------------------------

// Sicherheits-Kippwinkel [Grad]
// Ueber diesem Winkel gilt der Bot als "gefallen" --> FALLEN-Zustand
constexpr float FALLEN_ANGLE_DEG       = 45.0f;

// Winkel fuer Rueckkehr aus FALLEN nach IDLE [Grad]
// Bot muss unter diesem Winkel stehen, bevor Motoren wieder aktiv werden
constexpr float RECOVERY_ANGLE_DEG     = 15.0f;

// Wartezeit nach FALLEN vor erneutem Versuch [ms]
constexpr unsigned long FALLEN_WAIT_MS = 1000;

// ------------------------------------------------------------
// PID-Winkelregler (innerer Regler)
// Stellt Kippwinkel auf Sollwert (0 Grad = aufrecht)
// ------------------------------------------------------------

// Proportionalverstaerkung
// TODO: Tuning -- Startpunkt aus Modell berechnen
constexpr float KP_ANGLE               = 2.0f;

// Integralverstaerkung
// Mit Ki=0 starten bis P+D stabil balanciert!
constexpr float KI_ANGLE               = 0.0f;

// Differentialverstaerkung
constexpr float KD_ANGLE               = 0.1f;

// Anti-Windup: Tracking-Verstaerkung (Rueckkopplung bei Saettigung)
// Kaw = 1/Ti = Ki/Kp (typischer Startwert)
// Nur aktiv wenn KI_ANGLE > 0
constexpr float KAW_ANGLE              = 0.1f;

// Integralbegrenzung [%] (als absoluter Wert des Integralbeitrags)
constexpr float INTEGRAL_MAX_ANGLE     = 20.0f;

// Winkel-Sollwert [Grad]
// Kann per Serial justiert werden um mechanischen Offset auszugleichen
constexpr float SETPOINT_ANGLE_DEG     = 0.0f;

// ------------------------------------------------------------
// PID-Positionsregler (aeusserer Regler -- spaetere Phase)
// Stellt Fahrzeugposition (Encoder-Summe) auf Sollwert
// ------------------------------------------------------------

constexpr float KP_POS                 = 0.0f;   // TODO: erst nach stabilem Angle-PD tunen
constexpr float KI_POS                 = 0.0f;
constexpr float KD_POS                 = 0.0f;
constexpr float KAW_POS                = 0.0f;

// Maximaler Positionsfehler der als Winkelkorrektur aufaddiert wird [Grad]
constexpr float MAX_POS_CORRECTION_DEG = 5.0f;

// ------------------------------------------------------------
// IMU
// ------------------------------------------------------------

// BNO055 I2C-Adresse (Adafruit-Bibliothek)
constexpr uint8_t BNO055_I2C_ADDR     = 0x28;
// Initialisierungs-ID (Adafruit-Konvention)
constexpr int     BNO055_SENSOR_ID    = 55;

// ------------------------------------------------------------
// Serielle Kommunikation / Logging
// ------------------------------------------------------------

// Baudrate
constexpr long    SERIAL_BAUD         = 115200;

// CSV-Header fuer Logging (muss mit Logger::printHeader() uebereinstimmen)
// Format: millis,pitch,pitch_rate,p,i,d,duty,pos_counts,pos_m
// Spaltenzahl: 9 (Python-Pipeline erwartet genau diese Anzahl)
