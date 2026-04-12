// ============================================================
// imu.cpp -- BalanceBot: IMU-Implementierung
// ============================================================

#include "imu.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

bool Imu::begin() {
    // Adafruit-BNO055-Objekt erzeugen
    _bno = new Adafruit_BNO055(BNO055_SENSOR_ID, BNO055_I2C_ADDR);

    if (!_bno->begin()) {
        // I2C-Verbindung fehlgeschlagen
        return false;
    }

    // Externen Quarz fuer bessere Genauigkeit aktivieren
    _bno->setExtCrystalUse(true);

    // TODO: IMU-Modus pruefen
    // In Adafruit-Bibliothek: Standard ist NDOF (Magnetometer aktiv).
    // Fuer den BalanceBot: IMU-Modus (0x08) bevorzugt -- kein Magnetometer,
    // niedrigere Latenz, keine unvorhergesehenen Kalibrierungsspruenge.
    // Vorher Latenz beider Modi mit imu_latency-Sketch messen!
    // _bno->setMode(OPERATION_MODE_IMUPLUS);  // 0x08 = IMU-Modus

    return true;
}

void Imu::update() {
    uint32_t t0 = micros(); // Startzeit fuer Latenzmessung

    // Orientierungs-Euler-Winkel lesen
    sensors_event_t orientEvent;
    _bno->getEvent(&orientEvent);

    // Winkelgeschwindigkeit lesen (Gyro-Rohdaten)
    sensors_event_t gyroEvent;
    _bno->getEvent(&gyroEvent, Adafruit_BNO055::VECTOR_GYROSCOPE);

    uint32_t t1 = micros();
    _lastDurationUs = t1 - t0; // Dauer beider Lesezugriffe

    // Pitch-Winkel: event.orientation.z ist der Tilt um die Laengsachse
    // Vorzeichen: Vorwaertskippen = negatives Pitch (aus Diagnose verifiziert)
    _pitch = orientEvent.orientation.z;

    // Pitch-Rate aus Gyro-Kanal (y-Achse des Gyros entspricht Kippachse)
    // TODO: Gyro-Achse verifizieren! Ggf. x oder z je nach Einbauorientierung
    _pitchRate = gyroEvent.gyro.y;

    // Kalibrierungsgrad abfragen
    uint8_t calibMag = 0; // nicht benoetigt
    _bno->getCalibration(&_calibSys, &_calibGyro, &_calibAccel, &calibMag);
}

bool Imu::isCalibrated() const {
    // Gyro UND Accelerometer muessen mindestens Stufe 2 erreicht haben
    return (_calibGyro >= 2) && (_calibAccel >= 2);
}
