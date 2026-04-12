#pragma once

// ============================================================
// pid.h -- BalanceBot: PID-Regler mit Back-Calculation Anti-Windup
//
// Anti-Windup-Strategie: Back-Calculation (nicht einfaches Clamping)
//
// Wenn der Ausgang gesaettigt ist (duty > MAX_DUTY), wird die
// Differenz zwischen unbegrenztem und begrenztem Ausgang
// mit dem Tracking-Gain Kaw auf den Integrator zurueckgefuehrt.
// Das verhindert Integrator-Windup bei laengerer Saettigung
// (z.B. waehrend eines Sturzes mit vollem Duty).
//
// Formel:
//   e_aw = output_clamped - output_unclamped
//   integral += (error * dt) + (Kaw * e_aw * dt)
// ============================================================

#include <Arduino.h>

class Pid {
public:
    // Konstruktor -- Parameter setzen
    Pid(float kp, float ki, float kd, float kaw = 0.0f,
        float integralMax = 50.0f, float outputMin = -100.0f, float outputMax = 100.0f);

    // Einen Regelschritt berechnen
    // error: Regelabweichung (Soll - Ist)
    // dt:    verstrichene Zeit seit letztem Aufruf [Sekunden]
    // Gibt: Stellgroesse (begrenzt auf outputMin..outputMax)
    float compute(float error, float dt);

    // Integrator und D-Filter zuruecksetzen
    // Muss bei Uebergang von FALLEN/IDLE nach BALANCING aufgerufen werden
    void reset();

    // Getter fuer Logging (Einzel-Anteile)
    float pTerm() const { return _pTerm; }
    float iTerm() const { return _iTerm; }
    float dTerm() const { return _dTerm; }

    // Parameter zur Laufzeit aendern (fuer serielle Tuning-Schnittstelle)
    void setKp(float kp) { _kp = kp; }
    void setKi(float ki) { _ki = ki; }
    void setKd(float kd) { _kd = kd; }

private:
    // Reglerparameter
    float _kp, _ki, _kd, _kaw;
    float _integralMax;  // Begrenzung des Integratorspeichers
    float _outputMin;    // Ausgangs-Untergrenze (entspricht -MAX_DUTY)
    float _outputMax;    // Ausgangs-Obergrenze  (entspricht +MAX_DUTY)

    // Interner Zustand
    float _integral       = 0.0f; // Integralspeicher
    float _prevError      = 0.0f; // Vorheriger Fehler (fuer D-Anteil)
    bool  _firstStep      = true; // Verhindert Differenzierspike beim ersten Schritt

    // Letzte berechnete Einzel-Anteile (fuer Logging)
    float _pTerm = 0.0f;
    float _iTerm = 0.0f;
    float _dTerm = 0.0f;
};
