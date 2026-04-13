---
id: 0006
title: Control loop period fixed at 10 ms (100 Hz)
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [firmware, control]
---

# ADR-0006: Control loop period fixed at 10 ms (100 Hz)

## Context

A self-balancing robot is an *inverted pendulum* — an inherently unstable
system that will fall over unless the controller corrects it fast enough.
"Fast enough" is determined by the physics: the natural falling speed of
the pendulum sets a minimum update rate below which no controller can
recover it.

Two hardware constraints bound the choice from above and below:

**Lower bound — physics and sensor bandwidth.**
A typical two-wheeled balancing robot with a centre-of-gravity height of
a few centimetres has a natural frequency in the range of a few Hz. The
controller must run significantly faster than the falling dynamics —
conventionally at least 10× the natural frequency — to have enough time
to react. 50–100 Hz is the standard range for robots of this size.

**Upper bound — IMU read time.**
The BNO055 IMU is read over I²C on every loop iteration. The `imu_latency`
test sketch (`src/tests/imu_latency/`) exists specifically to measure how
long a `getEvent()` call takes on this hardware. Early characterisation
showed the read takes roughly 2–3 ms. At a 5 ms period (200 Hz) the IMU
read alone would consume 40–60% of the budget; any overrun makes the loop
non-deterministic. The Arduino Nano 33 IoT also runs serial command
parsing, encoder updates, PID computation, and state machine transitions
in the same loop body.

Going below 10 ms risks overrunning the IMU read time, which was noted
explicitly in `config.h`:

```cpp
// 10 ms = 100 Hz. Nicht unter 10 ms gehen (Bandbreite des inversen Pendels)
constexpr unsigned long LOOP_PERIOD_MS = 10;
```

## Decision

`LOOP_PERIOD_MS = 10` (100 Hz). The loop uses a `millis()`-based
non-blocking scheduling pattern — if less than 10 ms has elapsed since
the last iteration, `loop()` returns immediately:

```cpp
if (elapsedMs < LOOP_PERIOD_MS) return;
float dt = (float)elapsedMs / 1000.0f;
```

`dt` is measured from actual elapsed time rather than the nominal 10 ms,
so small jitter in the Arduino's task scheduling does not accumulate as
a timing error in the PID integrator.

Timer interrupts were not used. The `millis()` pattern is simpler,
sufficient for this application, and avoids the complexity of interrupt
safety around shared state (IMU object, PID state, encoder counts).

## Consequences

**Positive**
- 10 ms leaves ~7–8 ms headroom after the IMU read for all other
  loop work — PID, encoder, serial, state machine.
- `dt` from actual elapsed time makes the PID integrator correct even
  if occasional loop iterations run slightly long.
- Simple to understand and debug: no interrupt handlers, no shared
  state concerns.

**Negative**
- `millis()` scheduling is cooperative, not preemptive. A blocking call
  anywhere in the loop (e.g. a slow I²C transaction) delays the next
  iteration. This is why all loop work must be non-blocking.
- 100 Hz may prove insufficient if the CoG height measurement (Phase 4)
  reveals a faster natural frequency than expected. In that case
  `LOOP_PERIOD_MS` can be lowered, but only after re-running
  `imu_latency` to confirm the new period is safe.

**Neutral**
- The `imu_latency` test sketch exists specifically to validate the upper
  bound assumption. It must be re-run before any change to `LOOP_PERIOD_MS`.
