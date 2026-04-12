# BalanceBot -- Systemmodell (Inverses Pendel auf Rädern)

**Status:** Platzhalter -- wird nach CoG-Messung (Phase 4) ausgefüllt

**Referenz:** Grasser et al. 2002, "JOE: A Mobile, Inverted Pendulum"

---

## Zustandsvektor

```
x = [θ, θ̇, x_pos, ẋ]
     │   │   │      └── Fahrgeschwindigkeit [m/s]
     │   │   └───────── Fahrposition [m]
     │   └───────────── Kippwinkelgeschwindigkeit [rad/s]
     └───────────────── Kippwinkel [rad] (0 = aufrecht)
```

---

## Systemparameter (aus Datenblatt / Messung)

| Symbol | Bedeutung | Wert | Quelle |
|--------|-----------|------|--------|
| m      | Gesamtmasse [kg] | 0.204 | Gewogen |
| r      | Radradius [m] | 0.045 | CAD |
| L      | CoG-Höhe über Achse [m] | **TODO: Messen!** | Knife-Edge |
| J_w    | Trägheit Rad [kg·m²] | TODO | Abschätzung |
| J_b    | Trägheit Körper [kg·m²] | TODO | Abschätzung |
| R_a    | Ankerwiderstand [Ω] | ≈ 11.65 | Datenblatt* |
| K_t    | Drehmomentkonstante [N·m/A] | ≈ 0.2115 | Datenblatt* |
| K_e    | Gegen-EMK-Konstante [V·s/rad] | = K_t | (BLDC-Näherung) |
| n      | Getriebeübersetzung [-] | 100 | Datenblatt |

*Abschätzung aus Stallmoment und Stallstrom:  
  Kt = Stallmoment / (Stallstrom × n) = 0.0222 N·m / (1.03 A × 100) ≈ 0.000215 N·m/A (Motor)  
  Nach Getriebe: Kt_eff = Kt × n = 0.0215 N·m/A (Abtrieb)

*Ankerwiderstand: Ra = Nennspannung / Stallstrom = 12 V / 1.03 A ≈ 11.65 Ω

---

## Linearisiertes Zustandsraummodell

```
ẋ = A·x + B·u
y = C·x
```

Eingang: `u` = normierter Duty-Wert [-1, +1] (wird intern auf Spannung umgerechnet)  
Ausgang: `y` = [θ, x_pos] (IMU + Encoder)

Die A- und B-Matrizen werden nach Messung von L berechnet.

**TODO:** Python-Skript `python/system_id.py` erstellen:
1. Parameter eintragen
2. A, B aufstellen
3. Eigenwerte berechnen (offener Kreis: ein instabiler Pol erwartet!)
4. Steuerbarkeit prüfen: `control.ctrb(A, B)`
5. LQR-Design: `K, S, E = control.lqr(A, B, Q, R)`

---

## Natürliche Frequenz (Abschätzung)

Mit L ≈ 0.025 m (Schätzwert):

```
ω_n = sqrt(g / L) = sqrt(9.81 / 0.025) ≈ 19.8 rad/s ≈ 3.15 Hz
```

**Folgerung:** Controller-Bandbreite muss ≥ 3 × ω_n ≈ 10 Hz sein.  
Bei 100 Hz Abtastrate: 10 Samples pro Pendel-Schwingung → knapp ausreichend.  
→ **Abtastrate nicht unter 100 Hz reduzieren!**

---

## Nächste Schritte

- [ ] CoG-Höhe L messen (Knife-Edge-Test, siehe commissioning_plan.md Phase 4)
- [ ] Trägheitsmomente J_w und J_b abschätzen oder messen
- [ ] `python/system_id.py` schreiben
- [ ] LQR- oder Kaskadenregler-Entwurf durchführen
- [ ] Regler in `src/balance/main.cpp` einbauen
