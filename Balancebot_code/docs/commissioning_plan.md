# BalanceBot -- Inbetriebnahmeplan

> **⚠️ BLOCKING FINDINGS (Python pipeline, 2026-04-13)**
> Before recording any further measurements, fix the blocking issues in the data pipeline flagged by the code review:
> 1. `analyze.py` — latent `UnboundLocalError` on `start` when a recording contains no balancing phase (crash).
> 2. `acquire.py` — serial port leak on exception (port stays busy, next run fails).
> 3. `acquire.py` — CSV header row dropped in carrier mode (downstream `df["col"]` access breaks).
> 4. `config.py` — `os.makedirs()` runs at import time (test/lint side effects).
> 5. Unused imports (ruff F401 will block CI once linting runs).
>
> Full report: [`docs/reviews/2026-04-13-code-review.md`](../../docs/reviews/2026-04-13-code-review.md)

**Prinzip:** Jede Phase erst abschließen bevor die nächste beginnt.  
**Sicherheit:** Bei jedem Motortest Räder in der Luft! Nur bei Phase 5+ frei stellen.

---

## Phase 0: Hardware-Verifikation (einmalig, ohne Code-Änderung)

Ziel: Sicherstellen dass alle Komponenten korrekt verdrahtet und funktionsfähig sind.

- [ ] Motor Carrier erkennt Arduino (rote LED blinkt, `controller.getFWVersion()` gibt Version aus)
- [ ] Beide Motoren laufen bei Duty=30 (motor_test → `pio run -e balance_v1 -t upload`)
- [ ] Beide Encoder zählen korrekt (encoder_test, Rad von Hand drehen)
- [ ] BNO055 liefert plausible Pitch-Werte (sensor_test, Bot kippen)
- [ ] Vorzeichen-Konventionen verifiziert (diagnose, siehe `docs/sign_conventions.md`)

---

## Phase 1: Motor Carrier Timing-Charakterisierung

Ziel: I2C-Timing aller API-Aufrufe messen.

```bash
pio run -e carrier_timing -t upload
python acquire.py --mode carrier
```

**Erfolgskriterium:** Summe aller Mean-Werte < 8000 µs (80% von 10 ms Budget)  
**Falls nicht erfüllt:** I2C-Takt erhöhen oder Aufrufe reduzieren

- [ ] `controller.ping()` latenz notiert: _______ µs
- [ ] `encoder1.getRawCount()` latenz: _______ µs
- [ ] `encoder2.getRawCount()` latenz: _______ µs
- [ ] `M1.setDuty()` latenz: _______ µs
- [ ] `bno.getEvent()` latenz (NDOF): _______ µs
- [ ] `bno.getEvent()` latenz (IMU-Modus): _______ µs
- [ ] Summe: _______ µs → Budget OK? ✓/✗

---

## Phase 2: BNO055 Modus-Entscheidung

Ziel: NDOF vs. IMU-Modus vergleichen, BNO055-Konfiguration festlegen.

```bash
pio run -e imu_latency -t upload
python acquire.py --mode imu > messungen/raw/imu_latency.csv
python analyze.py messungen/raw/imu_latency.csv --mode imu
```

**Entscheidungsregel:**
- Wenn `bno.getEvent()` Latenz < 5 ms **und** Kalibrierung stabil → NDOF-Modus OK
- Wenn Latenz > 5 ms **oder** Kalibrierungssprünge → IMU-Modus (0x08) verwenden
- Wenn IMU-Modus immer noch > 5 ms → Complementary Filter auf SAMD21 implementieren

- [ ] Entscheidung getroffen: _________________ (NDOF / IMU-Modus / Comp. Filter)
- [ ] `imu.cpp` entsprechend konfiguriert (TODO-Kommentar in imu.cpp)

---

## Phase 3: Totzone-Identifikation

Ziel: Minimal-Duty für Anlaufen beider Motoren messen.

```bash
pio run -e deadzone_id -t upload
python acquire.py --mode deadzone
python process.py messungen/raw/deadzone_*.csv --mode deadzone
python analyze.py messungen/processed/deadzone_*_proc.csv --mode deadzone
```

- [ ] Totzone M1 gemessen: _______ % Duty
- [ ] Totzone M2 gemessen: _______ % Duty
- [ ] `DEADZONE_DUTY` in `config.h` aktualisiert: _______

---

## Phase 4: CoG-Höhe messen (Knife-Edge-Test)

Ziel: Schwerpunkthöhe `COG_HEIGHT_M` für Systemmodell bestimmen.

**Methode:**
1. Bot auf eine schmale Kante (Lineal) legen
2. Kante verschieben bis Bot balanciert
3. Vertikalen Abstand der Kante von der Radachse messen
4. Das ist die CoG-Höhe über Achsmitte

- [ ] CoG-Höhe gemessen: _______ mm über Achsmitte
- [ ] `COG_HEIGHT_M` in `config.h` aktualisiert

---

## Phase 5: Systemmodell aufstellen

Ziel: Linearisiertes Zustandsraummodell des inversen Pendels.

Zustandsvektor: `[θ, θ̇, x, ẋ]` (Winkel, Winkelgeschwindigkeit, Position, Geschwindigkeit)

Referenz: Grasser et al. 2002 "JOE: A Mobile, Inverted Pendulum"

**Schritte:**
1. Motorparameter aus Datenblatt extrahieren (Ra ≈ 11.65 Ω, Kt aus Stallmoment)
2. Trägheitsmomente schätzen (Masse × CoG² als Näherung)
3. A- und B-Matrizen aufstellen
4. Eigenwerte des offenen Kreises berechnen (Python: `numpy.linalg.eig`)
5. Steuerbarkeit prüfen (Python: `control.ctrb`)

- [ ] A-Matrix aufgestellt
- [ ] B-Matrix aufgestellt
- [ ] Offene-Kreis-Eigenwerte: _________________ (unstabiler Pol erwartet!)
- [ ] System steuerbar: ✓/✗
- [ ] Dokumentation in `docs/system_model.md`

---

## Phase 6: Erster Balancierversuch (PD-Regler)

**NUR nach Abschluss von Phasen 0-4!**

Startkonfiguration in `config.h`:
```c
KP_ANGLE  = 2.0     // Startwert, aus Modell verfeinern
KI_ANGLE  = 0.0     // Ki=0 bis PD stabil!
KD_ANGLE  = 0.1     // Startwert
MAX_DUTY  = 30      // Konservativ beginnen, später auf 45 erhöhen
```

**Prozedur:**
1. Bot in der Hand halten
2. `balance`-Firmware flashen: `pio run -e balance -t upload`
3. Serial-Monitor öffnen: `pio device monitor`
4. Bot senkrecht halten, `s` eingeben (Start)
5. Sanft loslassen -- Bot sollte Korrekturkräfte spüren
6. Wenn Bot sofort wegfährt: Kp-Vorzeichen umkehren!
7. Logging starten: `python acquire.py`

**Tuning-Reihenfolge:**
1. Kp erhöhen bis Bot beginnt zu zittern, dann auf 70% reduzieren
2. Kd erhöhen bis Zittern verschwindet
3. Erst dann Ki vorsichtig einschalten (und Anti-Windup prüfen)

- [ ] PD-Regler balanciert stabil (ohne Drift-Korrektur)
- [ ] Ki eingeschaltet, Positionsdrift reduziert
- [ ] MAX_DUTY schrittweise auf 45 erhöht

---

## Phase 7: Positions-Außenregler

Ziel: Bot bleibt an einem Ort stehen.

- [ ] `KP_POS`, `KI_POS` in `config.h` gesetzt (nach PD-Phase)
- [ ] Positions-Regler in `main.cpp` aktiviert (TODO-Kommentar entfernen)
- [ ] Drift < 10 cm nach 30 Sekunden

---

## Messungen archivieren

Nach jedem Versuch:
```bash
python acquire.py                          # aufzeichnen
python process.py messungen/raw/*.csv      # verarbeiten
python analyze.py --save messungen/processed/*.csv  # Plots speichern
```
