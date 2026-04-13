# Code Review — 2026-04-13

**Scope:** Full codebase review — firmware (C++) and Python data pipeline  
**Reviewer:** Claude Code (automated, assisted)  
**Branch at review time:** `docs/code-review`  
**Status:** Point-in-time snapshot — findings may be resolved in later commits

---

## Firmware (C++)

### CRITICAL — fix before any motion testing

**C-1 — Memory leak on repeated `imu.begin()` calls**  
File: `Balancebot_code/src/balance/imu.cpp`  
`begin()` allocates a new `Adafruit_BNO055` with `new` on every call but never deletes the previous object. The reset path in `main.cpp` calls `imu.begin()` a second time, leaking the first allocation. On a device with ~32 KB SRAM this will cause a hard failure.  
Fix: Replace the heap pointer with a value member in `imu.h`:
```cpp
// imu.h
Adafruit_BNO055 _bno{BNO055_SENSOR_ID, BNO055_I2C_ADDR};

// imu.cpp begin(): remove `new`, call _bno.begin() directly
```

**C-2 — Unconditional `update()` dereferences `_bno` without null-guard**  
File: `Balancebot_code/src/balance/imu.cpp`  
If `begin()` fails, `_bno` is left pointing at a heap object but `begin()` returns `false`. `update()` is called unconditionally in the main loop and dereferences `_bno` without checking. Resolved naturally by the C-1 fix (value member cannot be null).

---

### HIGH

**H-1 — `enc.begin()` never called in `setup()`**  
File: `Balancebot_code/src/balance/main.cpp`  
`motors.begin()` and `imu.begin()` are called but `enc.begin()` is not. Hardware encoder counters are not reset at startup. On a soft-reset or watchdog reset, counts will not be zeroed.  
Fix: Add `enc.begin();` in `setup()` after `motors.begin()`.

**H-2 — `fallenStartMs` zero-sentinel is fragile**  
File: `Balancebot_code/src/balance/main.cpp`  
Entry time to FALLEN is recorded by checking `if (fallenStartMs == 0)`. If `millis()` returns 0 at the exact moment of entry (possible at boot), the timer is never started. Pattern relies on `millis()` never being exactly 0 after the first few ms.  
Fix: Add a `bool _inFallen = false` flag alongside the timestamp, or reset `fallenStartMs` inside `changeState()` when entering FALLEN.

**H-3 — `uint32_t` timing accumulator can overflow in `carrier_timing`**  
File: `Balancebot_code/src/tests/carrier_timing/main.cpp`  
`sumUs` is `uint32_t`. If I2C calls take longer than ~4200 µs each, the sum across 1000 iterations overflows silently.  
Fix: Change `sumUs` to `uint64_t`.

**H-4 — `checkStall()` calls `millis()` internally rather than receiving `nowMs`**  
File: `Balancebot_code/src/balance/motor.cpp`  
Inconsistent with the rest of the codebase which passes time explicitly. Minor drift risk.  
Fix: Pass `nowMs` as a parameter to `checkStall()`.

---

### MEDIUM

**M-1 — D-term differentiates error, not measurement — derivative kick risk**  
File: `Balancebot_code/src/balance/pid.cpp`  
Not a problem while `SETPOINT_ANGLE_DEG` is constant, but will cause a derivative kick when the position PID is enabled and modifies the setpoint dynamically.  
Fix: When enabling the position PID, switch to differentiating measurement: `_dTerm = -_kd * ((measurement - _prevMeasurement) / dt)`.

**M-2 — `duty_f` to `int` truncation drops small commands**  
File: `Balancebot_code/src/balance/main.cpp` line ~237  
`(int)duty_f` truncates toward zero. Values like `0.9f` become `0`, hitting the dead-stop path instead of deadzone compensation. PID commands near zero are silently dropped.  
Fix: Use `(int)roundf(duty_f)` or pass `duty_f` as `float` all the way into `setDrive()`.

**M-3 — Magic `20.0f` in `pidPos` constructor**  
File: `Balancebot_code/src/balance/main.cpp` line ~46  
All other limits come from named constants in `config.h` — this one does not.  
Fix: Add `constexpr float INTEGRAL_MAX_POS = 20.0f;` to `config.h`.

**M-4 — `Encoder::begin()` and `Encoder::reset()` are identical**  
File: `Balancebot_code/src/balance/encoder.cpp`  
Both functions have the same body.  
Fix: `void Encoder::begin() { reset(); }`

---

### LOW

**L-1 — `while (!Serial)` hangs headless**  
File: `Balancebot_code/src/balance/main.cpp` line ~94  
On Nano 33 IoT with native USB, if no USB host is connected (e.g. running on battery), the robot never initialises.  
Fix: `while (!Serial && millis() < 3000);`

**L-2 — Test sketches duplicate constants instead of including `config.h`**  
Files: `carrier_timing/main.cpp`, `imu_latency/main.cpp`  
`115200`, `55`, `0x28` are hardcoded. If `config.h` values change, test sketches silently diverge.  
Fix: `#include "../../balance/config.h"` and use the named constants.

**L-3 — `motor.h` includes `<Arduino.h>` unnecessarily**  
Fix: Remove from the header; it is pulled in transitively through `motor.cpp`.

---

## Python Pipeline

### HIGH

**P-H-1 — Side-effect `os.makedirs()` at module scope in `config.py`**  
File: `Balancebot_code/python/config.py` lines 43–44  
Runs on every `import config`, including linters and tests. Also causes ruff E402 (import not at top of file).  
Fix: Expose an `ensure_dirs()` function and call it explicitly at the start of `acquire.py`, `process.py`, and `analyze.py`.

**P-H-2 — Serial port not closed on exception in `acquire.py`**  
File: `Balancebot_code/python/acquire.py`  
`ser` is opened but only closed via `ser.close()` at the end. If anything raises between open and close, the port is left open.  
Fix: Use `serial.Serial` as a context manager (`with serial.Serial(...) as ser:`).

**P-H-3 — `start` variable potentially unbound in `analyze.py`**  
File: `Balancebot_code/python/analyze.py` lines ~73–80  
`start` is assigned inside a conditional loop body but read after the loop without a guaranteed prior assignment.  
Fix: Initialise `start = t[0]` before the loop.

**P-H-4 — Unused imports**  
Files: `process.py` (`numpy`), `analyze.py` (`numpy`, `STATE_NAMES`)  
Fix: Remove unused imports. `ruff check --fix` handles this automatically.

---

### MEDIUM

**P-M-1 — No `if __name__ == "__main__"` guards**  
Files: `acquire.py`, `process.py`, `analyze.py`  
Top-level execution logic runs on import, preventing reuse or testing.  
Fix: Wrap execution logic in a `main()` function called under `if __name__ == "__main__":`.

**P-M-2 — Loop period hardcoded as magic number in `process.py`**  
File: `Balancebot_code/python/process.py` lines ~77,79  
`10.0` ms and `15.0` ms are hardcoded. Should reference a constant from `config.py`.  
Fix: Add `LOOP_PERIOD_MS = 10` and `JITTER_WARN_MS = 15` to `config.py`.

**P-M-3 — `print()` used throughout instead of `logging`**  
No severity levels, no timestamps, no way to suppress output programmatically.  
Fix: Replace with `logging.info()` / `logging.warning()` with a timestamp format.

**P-M-4 — Ruff auto-fixable issues (F541, I001, E702)**  
f-strings without placeholders, unsorted import blocks, semicolons joining two statements.  
Fix: `ruff check --fix Balancebot_code/python/`

**P-M-5 — Hand-rolled `pi` constant instead of `math.pi`**  
File: `Balancebot_code/python/config.py` line ~82  
Fix: `from math import pi` and use `pi * WHEEL_DIAMETER_M`.

---

### LOW

**P-L-1 — Carrier mode header not written to CSV**  
File: `Balancebot_code/python/acquire.py` lines ~127–132  
The first line (header) is logged but skipped — CSV gets integer column names.  
Fix: Write the header row to the CSV before `continue`-ing.

**P-L-2 — Typo in plot title**  
File: `Balancebot_code/python/analyze.py` line ~144  
`"Lateniz-Histogramm"` → `"Latenz-Histogramm"`

---

## Summary

| Area | CRITICAL | HIGH | MEDIUM | LOW |
|------|----------|------|--------|-----|
| Firmware (C++) | 2 | 4 | 4 | 3 |
| Python pipeline | 0 | 4 | 5 | 2 |

**Firmware is blocked for motion testing** until C-1 and C-2 are resolved (imu.cpp memory leak and null-dereference). All other findings are improvements that can be addressed incrementally.
