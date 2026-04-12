---
id: 0003
title: Motor duty cycle hard-capped at 45%
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [firmware, hardware, safety]
---

# ADR-0003: Motor duty cycle hard-capped at 45%

## Context

The Arduino Motor Carrier's motor driver is rated for a continuous current
of **500 mA**. Measured stall current of the motors is **1.03 A** (see
`config.h` line 48: `// Motorstall: 1.03 A, Treibergrenze: 0.50 A`).

At 100% PWM duty the motors can draw their full stall current, which is
roughly 2× the driver's rating. Sustained overcurrent destroys the driver.

The PID output is unbounded by design (integral wind-up and large angle
errors can push it well above 100). Without a hard cap in the motor layer,
a transient or tuning mistake passes a destructive command directly to the
hardware.

The cap must be set low enough to keep current within the driver rating
under worst-case (stall) conditions, while leaving enough authority for
the balance controller to be effective.

At 45% duty and the measured stall current of 1.03 A:

```
I_stall_at_45% ≈ 0.45 × 1.03 A ≈ 0.46 A  <  0.50 A rated
```

This gives a small but real margin without requiring a precise motor
characterization.

## Decision

`MAX_DUTY = 45` in `config.h`. The motor layer clamps every duty command
to `[-MAX_DUTY, +MAX_DUTY]` before writing to the Motor Carrier.

This value must not be raised without either:
- measuring that the chosen duty does not produce sustained overcurrent, or
- replacing the motor driver with one rated for higher current.

## Consequences

**Positive**
- Driver is protected during all commissioning phases, including Phase 6
  where large PID transients are expected during the first balance attempts.
- A tuning mistake that saturates the PID output does not damage hardware.

**Negative**
- Limits peak torque and top speed. If the balance controller requires
  more authority (e.g. recovering from a large disturbance), 45% may be
  insufficient. This will become apparent during Phase 6 testing.
- If the motors are lightly loaded (wheels off ground), 45% may still
  produce audible coil whine or thermal stress at sustained stall — the cap
  does not replace the stall-detection timeout (`STALL_TIMEOUT_MS`).

**Neutral**
- `DEADZONE_DUTY = 10` (Phase 3 estimate) sits well below `MAX_DUTY`.
  The effective command range for balancing is therefore [10, 45]% in
  each direction, shrunk further by whatever deadzone compensation is
  added after Phase 3.
