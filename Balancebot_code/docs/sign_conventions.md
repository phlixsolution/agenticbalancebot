# BalanceBot -- Vorzeichen-Konventionen

**Status:** Aus Diagnose-Sketch verifiziert (diagnose.cpp.bak)

---

## Koordinatensystem

Der Bot steht auf dem Boden, Fahrtrichtung ist "vorwärts" (definiert als Richtung, in die der USB-Port zeigt).

```
         ^ oben (Kippachse: positives Pitch = kippt nach hinten)
         |
    _____________________
   |                     |
   |   [USB]  [Motor C.] |  <-- Rückseite
   |_____________________|
         |
    === Achse ===
    (Wheel L)  (Wheel R)
         |
      Fahrtrichtung VORWÄRTS -->
```

---

## IMU (BNO055) -- Euler-Winkel

| Achse | event.orientation.x | Bedeutung |
|-------|---------------------|-----------|
| x     | Heading [°]         | Kompass-Nord (nicht genutzt) |
| y     | Roll [°]            | Seitwärtskippen |
| **z** | **Pitch [°]**       | **Vorwärts/Rückwärts-Kippen** |

**Vorzeichenkonvention Pitch:**
- `pitch = 0°` → Bot steht aufrecht
- `pitch < 0°` → Bot kippt **vorwärts** (USB-Seite neigt sich nach vorne)
- `pitch > 0°` → Bot kippt **rückwärts**

*Verifiziert durch: Manuelles Kippen des Bots, Beobachtung des Serial-Outputs.*

---

## Motoren (M1 / M2)

| Motor | Lage | Positive Duty-Richtung |
|-------|------|------------------------|
| M1    | Rechts (Fahrtrichtung) | dreht **rückwärts** (wegen Einbaulage gespiegelt) |
| M2    | Links                   | dreht **vorwärts** |

**Ansteuerung für Vorwärtsfahrt:**
```
Vorwärts:  M1.setDuty(-duty)  +  M2.setDuty(+duty)   // duty > 0
Rückwärts: M1.setDuty(+duty)  +  M2.setDuty(-duty)   // duty > 0
```

In `motor.cpp` (`_applyToMotors`):
```cpp
m1Duty = -duty;   // invertiert wegen Einbaulage
m2Duty = +duty;
```

---

## Encoder

| Encoder | Motor | Vorwärtsfahrt |
|---------|-------|---------------|
| encoder1 | M1 (rechts) | **negativer** Count |
| encoder2 | M2 (links)  | **positiver** Count |

**Positions-Berechnung (Geradeausfahrt):**
```
position = (encoder2.getRawCount() - encoder1.getRawCount()) / 2
```
- Formel hebt die gegenläufige Montage auf
- Positiver Wert = Vorwärtsfahrt ✓

*Auflösung: 1200 Counts / Umdrehung → 0.236 mm / Count*

---

## Regelkreis -- Vorzeichen-Überprüfung

**KRITISCH:** Falsches Vorzeichen = instabiler Regelkreis (beschleunigt in Absturzrichtung)

Checkliste vor dem ersten Balancierversuch:

1. Bot nach **vorne** kippen → `pitch` wird **negativ** ✓/✗
2. Positiver Duty → Motor M2 dreht **vorwärts**, M1 dreht **rückwärts** ✓/✗
3. Bot nach vorne kippen → Regler berechnet **positiven** Fehler (`0 - negative_pitch > 0`) ✓/✗
4. Positiver Fehler → positiver PID-Ausgang → positive Duty → Bot fährt **vorwärts** ✓/✗
5. Vorwärtsfahren bei vorwärts-Kippen **stabilisiert** den Bot ✓/✗

**Negativer Feedback-Loop muss gelten:**
```
Vorwärts-Kippen → Fehler positiv → Duty positiv → vorwärts fahren → Kippen wird reduziert
```

Wenn der Bot beim Start sofort wegfährt: Vorzeichen von Kp umkehren!

---

## Zusammenfassung für Copy-Paste

```
pitch (event.orientation.z):  0=aufrecht, negativ=vorwärts-Kippen
Motor-Ansteuerung:             Vorwärts = M1(-), M2(+)
Encoder:                       Vorwärts = Enc1↓, Enc2↑
Position:                      (Enc2 - Enc1) / 2, positiv=vorwärts
PID-Fehler:                    setpoint(0) - pitch
Regelung:                      Fehler>0 → Duty>0 → vorwärts (korrigiert ✓)
```
