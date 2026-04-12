## Summary

<!-- What does this PR do? One or two sentences. -->

## Type of change

- [ ] Firmware (C++ / PlatformIO)
- [ ] Python toolchain
- [ ] Docs / commissioning plan
- [ ] CAD
- [ ] Repo config / tooling

## Testing done

<!-- What did you actually run or flash to verify this works? -->

- [ ] Compiled without errors (`pio run`)
- [ ] Flashed to hardware and verified behaviour
- [ ] Python script ran without errors
- [ ] Relevant commissioning checklist items checked off

## Safety check

**Did you change any hardware-critical parameters in `config.h`?**

- [ ] No hardware-critical parameters changed
- [ ] Yes — changes listed below with justification:

| Parameter | Old value | New value | Reason |
|-----------|-----------|-----------|--------|
| | | | |

> Hardware-critical: `MAX_DUTY`, `DEADZONE_DUTY`, `FALLEN_ANGLE_DEG`, `LOOP_PERIOD_MS`, PID gains

## Commissioning phase

<!-- Which phase does this work relate to? -->

- [ ] Phase 0 — Hardware verification
- [ ] Phase 1 — Motor Carrier timing
- [ ] Phase 2 — IMU mode decision
- [ ] Phase 3 — Deadzone identification
- [ ] Phase 4 — CoG measurement
- [ ] Phase 5 — System model
- [ ] Phase 6 — First balance attempt
- [ ] Phase 7 — Position controller
- [ ] Not phase-specific
