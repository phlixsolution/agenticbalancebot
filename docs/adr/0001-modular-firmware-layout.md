---
id: 0001
title: Modular firmware layout supersedes balance_v1
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [firmware, architecture]
---

# ADR-0001: Modular firmware layout supersedes balance_v1

## Context

The first working firmware, `Balancebot_code/src/balance.cpp` ("balance_v1"), is a
single 156-line file containing IMU setup, motor control, PID math, serial
logging, and the main loop. It proved the hardware works: the BNO055 reads,
the Motor Carrier drives, encoders count, and a basic PID compiles and runs.

As we move from "does anything work" to the commissioning phases in
`docs/commissioning_plan.md`, several limits of the monolithic file started
to hurt:

- **No unit boundaries.** Phase 3 (deadzone identification) and Phase 6
  (first balance attempt) need to swap controllers and log formats without
  touching IMU or motor code. In v1 every change touches the same file.
- **No test sketches can share code.** `src/tests/` (carrier_timing,
  imu_latency, deadzone_id) would either duplicate v1's driver code or
  include the whole file. Neither is acceptable.
- **Hardware-critical parameters are scattered.** `MAX_DUTY`,
  `DEADZONE_DUTY`, `FALLEN_ANGLE_DEG`, and the PID gains need one canonical
  location so CLAUDE.md's "ask before changing" list maps to real lines.
- **No explicit state machine.** v1 is always running; there is no IDLE,
  no FALLEN recovery, no E-STOP. For Phase 6+ with the wheels on the ground
  this is unsafe.

Alternatives considered:

1. **Keep extending `balance.cpp`.** Rejected — every added concern makes
   the next change harder, and the tuning phases will add many concerns.
2. **Rewrite as a single new monolithic file.** Rejected — same problem,
   just with newer code.
3. **Split into modules with a state machine and a central config header.**
   Chosen.

## Decision

`Balancebot_code/src/balance/` is the active firmware. It is organized as:

| Module | Responsibility |
|---|---|
| `config.h` | All hardware-critical constants (single source of truth) |
| `imu.{h,cpp}` | BNO055 init, read, angle extraction |
| `motor.{h,cpp}` | Motor Carrier wrapper, duty clamping, deadzone compensation |
| `encoder.{h,cpp}` | Encoder read and position tracking |
| `pid.{h,cpp}` | PID with anti-windup |
| `logger.{h,cpp}` | CSV serial output |
| `state_machine.h` | States INIT → CALIBRATING → IDLE → BALANCING → FALLEN → E_STOP, plus transition logic. Header-only (no `.cpp`) since it is enums + inline helpers. |
| `main.cpp` | Setup, loop, serial command dispatch |

`balance.cpp` (v1) and `src/*.cpp.bak` files are **frozen reference
artifacts**. They are kept in the repo but not edited. CLAUDE.md's
"What not to touch" section enforces this.

New work — deadzone compensation, cascade controllers, tuning logging —
lands in `src/balance/` only.

## Consequences

**Positive**
- Test sketches in `src/tests/` can `#include` individual modules instead
  of duplicating driver code.
- The state machine makes safety transitions (FALLEN cutoff, E-STOP)
  explicit and reviewable.
- `config.h` gives CLAUDE.md's parameter table exact line references,
  so "ask before changing" is enforceable in review.
- Per-module files stay small enough to review as units (largest is
  `main.cpp` at ~294 lines).

**Negative**
- Two firmware trees coexist. A reader unfamiliar with the history might
  start from `balance.cpp` by accident. Mitigated by CLAUDE.md and by this
  ADR.
- Slightly more boilerplate (headers, includes) than a single file.

**Neutral**
- v1's serial log format (`millis,pitch,p,i,d,pid_out,duty,position`) is
  preserved by `logger.cpp` so existing `messungen/` recordings remain
  comparable.
