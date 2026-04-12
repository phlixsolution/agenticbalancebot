#pragma once

// ============================================================
// state_machine.h -- BalanceBot: Zustandsmaschine
//
// Zustaende und Uebergangsbedingungen des BalanceBots.
//
// Zustandsdiagramm:
//
//   INIT
//    |-- Carrier + IMU OK --> CALIBRATING
//    |-- Fehler           --> E_STOP
//
//   CALIBRATING
//    |-- Kalibrierung fertig und |pitch| < RECOVERY_ANGLE --> IDLE
//    |-- Kalibrierung Timeout    --> E_STOP
//
//   IDLE
//    |-- Serial "s" (start)     --> BALANCING
//    |-- |pitch| > FALLEN_ANGLE --> FALLEN
//
//   BALANCING
//    |-- Serial "q" (quit)      --> IDLE (Motoren aus)
//    |-- |pitch| > FALLEN_ANGLE --> FALLEN
//    |-- Stall erkannt          --> E_STOP
//
//   FALLEN
//    |-- Wartezeit abgelaufen und |pitch| < RECOVERY_ANGLE --> IDLE
//
//   E_STOP
//    |-- Serial "r" (reset)     --> INIT  (Hardware-Reset noetig)
//    |-- Immer: Motoren sofort 0!
// ============================================================

// Aufzaehlung aller moeglichen Zustaende
enum class State : uint8_t {
    INIT,           // Systemstart, Peripherie noch nicht initialisiert
    CALIBRATING,    // IMU-Kalibrierung laeuft
    IDLE,           // Bereit, Motoren aus, wartet auf Startbefehl
    BALANCING,      // Aktive Regelschleife
    FALLEN,         // Sicherheitswinkel ueberschritten, Motoren aus
    E_STOP          // Not-Aus (Stall, Init-Fehler), Motoren aus
};

// Lesbare Bezeichnung fuer Serial-Ausgaben
inline const char* stateName(State s) {
    switch (s) {
        case State::INIT:        return "INIT";
        case State::CALIBRATING: return "CALIBRATING";
        case State::IDLE:        return "IDLE";
        case State::BALANCING:   return "BALANCING";
        case State::FALLEN:      return "FALLEN";
        case State::E_STOP:      return "E_STOP";
        default:                 return "UNKNOWN";
    }
}
