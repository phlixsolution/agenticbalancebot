# Schwerpunkt (Center of Mass) — Bedeutung und Messmethoden

## Warum ist der Schwerpunkt so wichtig?

Der BalanceBot ist ein **inverses Pendel** — ein Stab, der auf seiner Spitze balanciert. Die Physik dahinter:

### 1. Kippmoment und Reaktionszeit

Je **hoeher** der CoM ueber der Radachse liegt, desto **langsamer** kippt der Bot. Das klingt paradox, ist aber das gleiche Prinzip wie beim Besenstiel-Balancieren: ein langer Stiel ist leichter zu balancieren als ein kurzer.

Physikalisch: Die Winkelfrequenz des inversen Pendels ist

```
omega_0 = sqrt(m * g * h / J)
```

wobei `h` = Abstand CoM zur Achse, `J` = Traegheitsmoment, `m` = Masse, `g` = Erdbeschleunigung. Ein hoeherer CoM erhoeht `h`, aber `J` waechst mit `h^2` — netto wird das System **traeger** und damit einfacher zu regeln.

### 2. Y-Offset (vor/hinter der Achse)

Wenn der CoM nicht genau ueber der Radachse sitzt, muss der Regler **permanent gegensteuern**, um das statische Kippmoment auszugleichen. Das kostet Energie und reduziert den nutzbaren Regelbereich. Ideal: `CoM_Y = 0`.

### 3. X-Offset (links/rechts)

Ein seitlicher Versatz erzeugt eine **Kurvenfahrt-Tendenz** — der Bot driftet zur schweren Seite. Ideal: `CoM_X = 0`.

### 4. Regelparameter haengen direkt vom CoM ab

Die PID-Gains (`KP_ANGLE`, `KD_ANGLE` in `config.h`) werden auf Basis des Systemmodells berechnet. Das Modell braucht `COG_HEIGHT_M` — den Abstand des CoM zur Radachse. Ein falscher Wert fuehrt zu:

- Zu aggressive Gains -> Oszillation
- Zu schwache Gains -> Bot kippt um

---

## Messmethoden

### Methode 1: CAD-Berechnung (was `com_analysis.py` macht)

Das Skript berechnet den CoM aus den Volumina und Materialdichten aller Teile. Jedem Teil wird eine Materialdichte zugewiesen:

| Material | Dichte [g/cm^3] |
|----------|-----------------|
| PLA (Rahmen, Motorhalter, Radnaben) | 1.04 |
| FR4 / PCB (Motor Carrier, Arduino) | 1.85 |
| Batterie (18650 Li-Ion) | 2.70 - 2.75 |
| N20-Motor mit Getriebe | 4.50 |
| Radreifen (Gummi/TPU) | 1.15 |

**Vorteil:** Schnell, iterativ anpassbar, keine Hardware noetig.
**Nachteil:** Nur so genau wie die CAD-Modelle und die angenommenen Dichten.

Ausfuehrung:

```bash
cat com_analysis.py | /snap/bin/freecad --console
```

---

### Methode 2: Knife-Edge-Test (empfohlen zur Validierung)

Dies ist die in `commissioning_plan.md` Phase 4 vorgesehene Methode.

1. Bot komplett aufbauen (alle Teile montiert, Akku eingesetzt)
2. Den Bot auf eine **scharfe Kante** (Lineal, Metallschiene) legen
3. Die Kante verschieben, bis der Bot **genau im Gleichgewicht** liegt
4. Den Abstand der Kante zur Radachse messen -> das ist `h` (CoG height)
5. In **zwei Achsen** wiederholen (Y und X), um den vollstaendigen CoM zu bestimmen

```
        +----------+
        | BalanceBot|
        |    x CoM |
        +----+-----+
             |
    =========+=========  <- Knife edge (Gleichgewichtspunkt)
             |
         ----o----       <- Radachse
```

**Vorteil:** Einfach, keine Spezialgeraete noetig.
**Nachteil:** Nur eine Achse pro Messung.

---

### Methode 3: Aufhaenge-Methode (Plumb-Line)

1. An zwei verschiedenen Stellen am Bot je einen Haken oder Faden befestigen (nicht an der Achse)
2. Bot am ersten Punkt aufhaengen, frei pendeln lassen
3. Lotlinie entlang des Fadens auf dem Bot markieren
4. Bot am zweiten Punkt aufhaengen, erneut Lotlinie markieren
5. **Schnittpunkt der beiden Linien = CoM**
6. Vertikalen Abstand vom Schnittpunkt zur Radachse messen

**Vorteil:** Bestimmt den CoM in zwei Dimensionen gleichzeitig.
**Nachteil:** Markierungen muessen praezise sein.

---

### Methode 4: Zwei-Waagen-Methode (Two-Scale)

Den Bot flach auf zwei identische Kuechenwaagen legen, die in bekanntem Abstand stehen:

```
Waage A (zeigt F_A)         Waage B (zeigt F_B)
    |___________________________|
    <------- d_gesamt --------->
```

Berechnung:

```
L_cog_von_A = (F_B * d_gesamt) / (F_A + F_B)
```

Dies ergibt die horizontale CoM-Position. Den Bot 90 Grad drehen (aufrecht abstuetzen) und wiederholen, um die **vertikale CoM-Hoehe** zu bestimmen.

**Vorteil:** Rechnerisch genau, reproduzierbar.
**Nachteil:** Aufwendiger Aufbau, Bot muss stabil positioniert werden.

---

## Empfohlenes Vorgehen fuer den BalanceBot

1. **CAD-Berechnung** mit `com_analysis.py` -> erste Abschaetzung
2. **Knife-Edge-Test** (Phase 4) -> Validierung am echten Bot
3. Gemessenen Wert als `COG_HEIGHT_M` in `config.h` eintragen
4. Erst dann mit PID-Tuning (Phase 6) beginnen
