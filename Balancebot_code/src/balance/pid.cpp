// ============================================================
// pid.cpp -- BalanceBot: PID-Implementierung
// ============================================================

#include "pid.h"

Pid::Pid(float kp, float ki, float kd, float kaw,
         float integralMax, float outputMin, float outputMax)
    : _kp(kp), _ki(ki), _kd(kd), _kaw(kaw),
      _integralMax(integralMax), _outputMin(outputMin), _outputMax(outputMax)
{}

float Pid::compute(float error, float dt) {
    // Schutz vor dt=0 oder Negativwerten (z.B. millis()-Ueberlauf)
    if (dt <= 0.0f) return 0.0f;

    // --- P-Anteil ---
    _pTerm = _kp * error;

    // --- D-Anteil ---
    // Beim ersten Schritt keinen D-Anteil berechnen (kein gültiger prevError)
    if (_firstStep) {
        _dTerm     = 0.0f;
        _firstStep = false;
    } else {
        // Differenzierer auf Fehler (nicht auf Messgroesse -- kein Derivative Kick)
        // TODO: Spaeter auf Messgroesse (Gyro-Rate) wechseln um Sollwert-Sprung-Kick zu vermeiden
        _dTerm = _kd * ((error - _prevError) / dt);
    }
    _prevError = error;

    // --- Ungesaettigter Ausgang ---
    // Integralanteil wird erst nach Anti-Windup-Update berechnet
    float outputUnclamped = _pTerm + (_ki * _integral) + _dTerm;

    // --- Ausgangsbegrenzung ---
    float outputClamped = outputUnclamped;
    if (outputClamped >  _outputMax) outputClamped =  _outputMax;
    if (outputClamped < _outputMin)  outputClamped =  _outputMin;

    // --- Anti-Windup: Back-Calculation ---
    // e_aw = Differenz zwischen gesaettigtem und ungesaettigtem Ausgang
    // Bei Saettigung: e_aw != 0 --> bremst Integrator ab
    // Bei normaler Operation: e_aw = 0 --> kein Einfluss
    float e_aw = outputClamped - outputUnclamped;

    // --- I-Anteil: Integrator-Update ---
    // Normales Integral + Anti-Windup-Rueckkopplung
    _integral += (error * dt) + (_kaw * e_aw * dt);

    // Sekundaere Begrenzung des Integratorspeichers (Fallback)
    if (_integral >  _integralMax) _integral =  _integralMax;
    if (_integral < -_integralMax) _integral = -_integralMax;

    // I-Term fuer Logging
    _iTerm = _ki * _integral;

    return outputClamped;
}

void Pid::reset() {
    // Zustand vollstaendig zuruecksetzen
    // Muss vor jedem neuen Balancierversuch aufgerufen werden
    _integral  = 0.0f;
    _prevError = 0.0f;
    _firstStep = true;
    _pTerm     = 0.0f;
    _iTerm     = 0.0f;
    _dTerm     = 0.0f;
}
