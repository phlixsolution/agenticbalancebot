#pragma once

// ============================================================
// imu.h -- BalanceBot: IMU-Schnittstelle (BNO055)
//
// Kapselt alle BNO055-Zugriffe. Gibt einen einzigen
// Kippwinkel (pitch) und eine Winkelgeschwindigkeit (pitch_rate)
// zurueck. Keine Magic Numbers ausserhalb dieser Datei.
//
// Vorzeichenkonvention (aus Diagnose-Sketch verifiziert):
//   pitch > 0  --> Bot kippt rueckwaerts
//   pitch < 0  --> Bot kippt vorwaerts
//   pitch = 0  --> aufrecht
// ============================================================

#include <Arduino.h>

// Vorwaertsdeklaration (Adafruit-Klasse)
class Adafruit_BNO055;

class Imu {
public:
    // Initialisierung -- gibt false zurueck bei I2C-Fehler
    bool begin();

    // Einen Messwert vom BNO055 lesen
    // Muss einmal pro Regelzyklus aufgerufen werden
    void update();

    // Kippwinkel [Grad] -- relativ zur aufrechten Position
    float pitch() const { return _pitch; }

    // Winkelgeschwindigkeit um Kippachse [Grad/s]
    float pitchRate() const { return _pitchRate; }

    // Kalibrierungsgrad des Systems (0-3, 3 = voll kalibriert)
    // Fuer BNO055 IMU-Modus (ohne Magnetometer) zaehlt nur gyro+accel
    uint8_t calibration() const { return _calibSys; }

    // Gibt true wenn der Gyro und Accelerometer kalibriert sind (Schwelle 2/3)
    bool isCalibrated() const;

    // Rohe Messzeit des letzten update()-Aufrufs [Mikrosekunden]
    // Fuer Latenz-Diagnose; 0 vor erstem Aufruf
    uint32_t lastUpdateDurationUs() const { return _lastDurationUs; }

private:
    Adafruit_BNO055* _bno = nullptr; // Zeiger auf Adafruit-Objekt (dynamisch erzeugt in begin())
    float     _pitch          = 0.0f;
    float     _pitchRate      = 0.0f;
    uint8_t   _calibSys       = 0;
    uint8_t   _calibGyro      = 0;
    uint8_t   _calibAccel     = 0;
    uint32_t  _lastDurationUs = 0;
};
