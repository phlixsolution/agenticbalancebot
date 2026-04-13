---
id: 0005
title: PlatformIO as the build and upload system
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [tooling, firmware, build]
---

# ADR-0005: PlatformIO as the build and upload system

## Context

Arduino firmware can be built and uploaded in several ways. The most
common starting point is the **Arduino IDE** — a graphical tool that
hides most of the build process. It works well for single-file sketches
but has significant limitations as a project grows:

- No support for a proper `src/` directory structure; all `.cpp` files
  must live in the sketch folder.
- Library management is global and per-machine, not per-project.
  Two collaborators can silently have different library versions installed.
- No CLI — building and uploading requires the GUI, making it harder
  to script or reproduce.
- No named build environments — switching between `balance_v1` and the
  modular `balance/` target, or between production firmware and test
  sketches, requires manual file swapping.

This project has five distinct firmware targets that need to coexist:
`balance_v1` (reference), `balance` (active development), and three
test sketches (`carrier_timing`, `imu_latency`, `deadzone_id`). Managing
these in the Arduino IDE would require five separate sketch folders with
duplicated library references.

## Decision

**PlatformIO** (`platformio.ini` at `Balancebot_code/`) is the build
and upload system for all firmware.

The configuration defines a shared `[common]` block (board, platform,
framework, baud rate, library dependencies) that all environments inherit.
Each environment overrides only `build_src_filter` to select its source:

| Environment | Source | Purpose |
|---|---|---|
| `balance_v1` | `src/balance.cpp` | Reference build — frozen |
| `balance` | `src/balance/` | Active modular firmware |
| `carrier_timing` | `src/tests/carrier_timing/` | Phase 1 test |
| `imu_latency` | `src/tests/imu_latency/` | Phase 2 test |
| `deadzone_id` | `src/tests/deadzone_id/` | Phase 3 test |

Library versions are pinned in `platformio.ini`:

```ini
lib_deps =
    adafruit/Adafruit BNO055@^1.6.1
    adafruit/Adafruit Unified Sensor@^1.1.14
    arduino-libraries/ArduinoMotorCarrier
```

Both collaborators build against the same versions regardless of what
is installed globally on their machines.

Common commands:
```bash
pio run -e balance -t upload   # build and flash active firmware
pio run -e balance_v1          # build reference (verify it still compiles)
pio device monitor             # open serial monitor at 115200 baud
```

## Consequences

**Positive**
- Library versions are locked in the repo. Both collaborators always
  build with identical dependencies.
- `build_src_filter` is the mechanism that enforces the firmware layout
  decision from ADR-0001: each environment compiles exactly one target,
  so `balance_v1` and `balance/` cannot accidentally include each other.
- Test sketches are first-class build targets — no file swapping needed
  to flash `carrier_timing` or `deadzone_id`.
- Builds and uploads are scriptable from the command line, which is
  required once we add CI or automated test runs.

**Negative**
- PlatformIO must be installed (`pip install platformio` or via the VS
  Code extension). A collaborator with only the Arduino IDE cannot build
  the project without installing it first.
- PlatformIO downloads board support packages and libraries on first
  use, which requires an internet connection and takes a few minutes.

**Neutral**
- The Arduino Nano 33 IoT board definition (`board = nano_33_iot`,
  `platform = atmelsam`) is specified once in `[common]` and inherited
  by all environments. Changing to a different board requires one line.
