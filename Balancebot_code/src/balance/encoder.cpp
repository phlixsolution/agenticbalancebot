// ============================================================
// encoder.cpp -- BalanceBot: Encoder-Implementierung
// ============================================================

#include "encoder.h"
#include "config.h"
#include <ArduinoMotorCarrier.h>

void Encoder::begin() {
    encoder1.resetCounter(0);
    encoder2.resetCounter(0);
    _position = 0;
    _speedCps = 0;
    _prevPos  = 0;
}

void Encoder::update(float dt) {
    // Rohe Encoder-Counts lesen
    int raw1 = encoder1.getRawCount(); // negativ bei Vorwaertsfahrt
    int raw2 = encoder2.getRawCount(); // positiv bei Vorwaertsfahrt

    // Gemittelte Position (positiv = vorwaerts)
    // Formel: (enc2 - enc1) / 2 hebt die gegenlaefige Montage auf
    _position = (raw2 - raw1) / 2;

    // Geschwindigkeit als Ableitung der Position [Counts/s]
    // Schutz vor dt = 0 (sollte durch Hauptschleife verhindert sein)
    if (dt > 0.0f) {
        int deltaPos = _position - _prevPos;
        _speedCps = (int)((float)deltaPos / dt);
    }
    _prevPos = _position;
}

float Encoder::positionM() const {
    return (float)_position * METERS_PER_COUNT;
}

void Encoder::reset() {
    encoder1.resetCounter(0);
    encoder2.resetCounter(0);
    _position = 0;
    _speedCps = 0;
    _prevPos  = 0;
}
