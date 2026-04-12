# Panel Review: BalanceBot
*Generated: 2026-04-10 | Reviewers: data-analyst, embedded-dev, engineer*

**Overall Verdict: NEEDS IMPROVEMENT**

The hardware is adequate and the toolchain is functional, but the project lacks the control engineering backbone (plant model, state definition, control architecture, safety strategy) needed before attempting closed-loop balancing. The motor driver current limit is a real but manageable constraint.

---

## Consensus

All three reviewers agree on the following:

- **The stall current / driver limit mismatch (1.03A vs. 500mA) must be addressed in software before any closed-loop balance attempt.** A duty cycle clamp at ~48% is the minimum viable protection. All three independently derived this figure.
- **No plant model exists yet — this blocks all principled controller design.** Trial-and-error tuning on this system risks repeated overcurrent events and driver damage.
- **Sign/coordinate conventions are undefined.** A single sign error inverts the feedback loop and the robot accelerates into the ground. This must be documented and verified before writing any control code.
- **IMU data is not logged.** The primary controlled variable (tilt angle) is absent from the CSV schema, making post-hoc analysis of balance behavior impossible.
- **Anti-windup is mandatory.** With the hard duty cap, integrator windup during saturation will cause slow reversal and falls. All three reviewers flagged this independently.
- **Sampling time jitter is uncharacterized.** No hardware timer drives the control loop — `loop()` with `millis()` checks produces jitter that can destabilize the controller.

---

## Domain-Specific Insights

### Data Analyst — Dead-time measurement before system identification

Before fitting any transfer function, the total loop delay (serial framing + task scheduling + filter group delay) must be measured. A 20-30ms unmodeled delay in a 10ms sampling system is not negligible and causes `curve_fit` to underestimate time constants. The fix: inject a known step, timestamp the command on the PC with `time.perf_counter()`, and measure the first non-zero response. Then fit a PT1+dead-time model.

### Embedded Developer — The ArduinoMotorCarrier library is a black box that must be characterized

Every control signal and sensor reading passes through I2C transactions to the ATSAMD11 co-processor. The latency, update rate, atomicity, and watchdog behavior of this communication path are unknown. A test sketch measuring round-trip times of `controller.ping()`, `encoder1.getRawCount()`, `M1.setDuty()`, and IMU reads (1000 iterations, log min/max/mean) is mandatory before trusting the library in a closed loop.

### Engineer — The system has ample torque margin but dynamic bandwidth is the real constraint

At the driver-limited torque (0.211 N·m total), the static torque margin is ~48:1 at 5° tilt — more than enough. The natural frequency of the inverted pendulum (with estimated L_cog = 25mm) is ~3.15 Hz, requiring a controller bandwidth of 10-15 Hz. At 100 Hz sampling, this gives only 7-10 samples per cycle — tight but workable. Do not reduce the sampling rate below 100 Hz.

---

## Critical Issues

| # | Issue | Raised by | Impact |
|---|-------|-----------|--------|
| 1 | **Motor stall current (1.03A) exceeds driver limit (500mA) — no software protection exists** | All three | Hardware damage or thermal shutdown during any significant balance correction. Duty clamp at 48% is mandatory. |
| 2 | **No hardware-timer-driven control loop** | Embedded, Engineer | Timing jitter from `loop()` + serial I/O + `controller.ping()` can destabilize the controller. 1-2ms jitter at 10ms sample = 10-20% variation. |
| 3 | **Motor Carrier communication path is uncharacterized** | Embedded | If `controller.ping()` or I2C transactions take 5ms on some calls, the 10ms budget is blown and the controller sees stale data. |

---

## Improvement Roadmap (unified)

| Priority | Improvement | Raised by | Actionable steps |
|----------|-------------|-----------|------------------|
| **HIGH** | Implement duty limiting and stall detection | All three | `duty = constrain(duty, -48, +48)`. Add slew rate limit (max 20 units/sample). Stall detection: if `abs(duty) > 30 && abs(speed) < 5` for 500ms → E_STOP. |
| **HIGH** | Build linearized state-space model | Engineer, Data Analyst | Measure CoG height (knife-edge test). Extract motor params from datasheet (R_a ≈ 11.65Ω, K_t = 0.2115 N·m/A). Write the 4-state [θ, θ̇, x, ẋ] equations per Grasser et al. (2002). Verify open-loop eigenvalues. |
| **HIGH** | Characterize Motor Carrier communication | Embedded | Test sketch: 1000 iterations of each API call, log timing via `micros()`. Determine I2C bus speed, encoder atomicity, and `ping()` latency. |
| **HIGH** | Implement hardware-timer control loop | Embedded | Configure SAMD21 TC3 for 10ms ISR. Execute read→compute→actuate in the ISR or flag-driven highest-priority task. Verify with GPIO toggle. |
| **HIGH** | Add IMU data to log schema | Data Analyst, Embedded | Add `imu_angle_deg`, `imu_rate_dps` to the `Serial.println()` line. Cost: ~30 bytes/line at 100 Hz = 3 kB/s. |
| **HIGH** | Define and verify sign conventions | Engineer, Embedded | One-page diagram. Four test sketches: tilt forward → angle sign? Positive duty → encoder sign? Positive duty → wheel direction? Verify negative feedback loop product. |
| **HIGH** | Implement state machine | Embedded | States: INIT → CALIBRATING → IDLE → BALANCING → FALLEN → E_STOP. Transitions on tilt angle threshold, serial commands, overcurrent. Motors off in FALLEN and E_STOP. |
| **MEDIUM** | Implement anti-windup | Engineer, Data Analyst | Back-calculation: `e_aw = duty_clamped - duty_unclamped`, feed back to integrator with tracking gain `Kaw = 1/Ti`. |
| **MEDIUM** | Configure BNO055 for minimum latency | Engineer, Embedded | Set IMU mode (0x08), not NDOF. Disable magnetometer fusion. If latency still too high, implement complementary filter: `angle = 0.98*(angle + gyro*dt) + 0.02*accel_angle`. |
| **MEDIUM** | Split Python pipeline into acquire/process/analyze | Data Analyst | `acquire.py` (serial → raw CSV), `process.py` (validation + derived signals → Parquet), `analyze.py` (plots + fits). Add `config.py` and `requirements.txt`. |
| **MEDIUM** | Characterize and compensate motor dead zones | Engineer, Data Analyst | Ramp duty 0→100% slowly, log encoder velocity vs. duty. Implement piecewise linear feedforward per motor. |
| **MEDIUM** | Measure dead-time / transport delay | Data Analyst | Inject step from PC, timestamp with `time.perf_counter()`, measure first non-zero response. Fit PT1+dead-time model. |
| **MEDIUM** | Implement cascade control structure | Engineer | Inner loop: PD on tilt angle → duty. Outer loop: PI on position → tilt setpoint. Tune inner first (robot balances but drifts), then close outer. |
| **LOW** | Enable SAMD21 hardware watchdog | Embedded | WDT ~50ms timeout, fed only from control loop ISR. Catches I2C bus lockups. |
| **LOW** | Switch long-run storage to Parquet | Data Analyst | `df.to_parquet()` after validation. Keep raw CSV as archival input. |
| **LOW** | Report confidence intervals on all curve_fit results | Data Analyst | Compute `sigma = sqrt(diag(pcov))`, print 95% CI. Flag if CI width > 50% of nominal. |

---

## Research & Reference Projects

### Control Theory & Modeling

1. **Grasser, D'Arrigo, Colombi, Rufer — "JOE: A Mobile, Inverted Pendulum" (IEEE 2002)**
   Search: `Grasser D'Arrigo "JOE" inverted pendulum IEEE 2002`
   *Relevance:* The canonical reference for two-wheeled inverted pendulum state-space modeling. Provides the exact A and B matrices for BalanceBot's model.
   *What to look for:* State vector definition [θ, θ̇, x, ẋ], linearization procedure, LQR weight selection.

2. **Steve Brunton — Control Bootcamp (YouTube + GitHub)**
   Search: `Steve Brunton Control Bootcamp inverted pendulum`
   *Relevance:* LQR design for inverted pendulum with worked MATLAB examples. Directly applicable once the state-space model is built.
   *What to look for:* Controllability, LQR weight selection, observer design.

3. **Brian Douglas — Control Systems Lectures (YouTube / MATLAB Tech Talks)**
   Search: `Brian Douglas inverted pendulum state space`
   *Relevance:* Best video explanation of the complete state-space design flow from model to LQR to observer.
   *What to look for:* "What is LQR" and "What is a State Observer" videos.

4. **Franklin, Powell, Emami-Naeini — "Feedback Control of Dynamic Systems" (8th ed.)**
   *Relevance:* Chapter 7, Example 7.28 covers inverted pendulum LQR with explicit Q/R selection procedure.
   *What to look for:* Constraint-aware LQR weight tuning (R chosen so gains stay within actuator limits).

### Balancing Robot Implementations

5. **Brokking.net / YABR (Yet Another Balancing Robot)**
   URL: `http://www.brokking.net/yabr.html`
   *Relevance:* Complete Arduino-based balance bot with source code, PID tuning videos, complementary filter, dead zone compensation. Closest pedagogical match.
   *What to look for:* Incremental commissioning methodology (IMU alone → motors alone → combine).

6. **B-ROBOT / JJRobots EVO 2**
   URL: `https://jjrobots.com/b-robot-evo-2/`
   *Relevance:* Open-source Arduino balance bot with cascade PD/PI control, dead zone handling, and anti-windup.
   *What to look for:* Control architecture and practical tuning approach.

### Embedded Firmware Patterns

7. **ArduinoMotorCarrier Library Source Code**
   URL: `https://github.com/arduino-libraries/ArduinoMotorCarrier`
   *Relevance:* Mandatory reading. Reveals I2C transaction timing, co-processor protocol, BNO055 data path.
   *What to look for:* What each API call does at the I2C level. Whether BNO055 is routed through ATSAMD11 or direct I2C.

8. **FreeRTOS-SAMD21 Port**
   URL: `https://github.com/BriscoeTech/Arduino-FreeRTOS-SAMD21`
   *Relevance:* Solves control loop vs. serial I/O vs. safety scheduling with proper task priorities.
   *What to look for:* Task creation, binary semaphores for ISR-to-task signaling, stack sizing for 32KB SRAM.

9. **SimpleFOC Library (Arduino-FOC)**
   URL: `https://github.com/simplefoc/Arduino-FOC`
   *Relevance:* Excellent firmware architecture patterns (timer ISR, sensor/controller/actuator separation, serial commander) transferable to BalanceBot.
   *What to look for:* Timer ISR structure, SAMD21-specific timer configuration, command interface pattern.

10. **Madgwick AHRS Filter**
    URL: `https://github.com/arduino-libraries/MadgwickAHRS`
    *Relevance:* Fallback if BNO055's internal fusion has too much latency. Lightweight quaternion-based filter.
    *What to look for:* Integration with raw accel+gyro data, beta parameter tuning.

### Sensor & Driver References

11. **BNO055 Datasheet + Application Note BST-BNO055-AN007**
    Search: `BNO055 "fusion output data rate" IMU mode vs NDOF mode`
    *Relevance:* IMU mode gives lowest latency. NDOF magnetometer recalibration causes transient jumps — catastrophic for balancing.
    *What to look for:* Section 3.6 (Output Data Rates), operation mode 0x08 (IMU mode).

12. **TI Application Note SLVA249 — "Current Limiting in H-Bridge Motor Drivers"**
    *Relevance:* Duty cycle clamp derivation from motor model, thermal protection for H-bridges.
    *What to look for:* Software current limiting methodology, thermal derating curves.

### Python Analysis Tools

13. **Python Control Systems Library (python-control)**
    URL: `https://python-control.readthedocs.io`
    *Relevance:* Python equivalent of MATLAB's Control System Toolbox. Bode plots, root locus, step response, sisotool.
    *What to look for:* `control.tf()`, `control.step_response()`, `control.bode_plot()`.

14. **SIPPY — System Identification Package for Python**
    URL: `https://github.com/CPCLAB-UNIPI/SIPPY`
    *Relevance:* Proper prediction-error system identification (ARX, ARMAX, state-space) from measured I/O data.
    *What to look for:* State-space identification from step response CSV data.

15. **Phil's Lab (YouTube) — Complementary Filter and PID Tuning**
    Search: `Phil's Lab complementary filter IMU PID`
    *Relevance:* Practical video tutorials bridging textbook theory and embedded implementation, with real hardware.
    *What to look for:* Complementary filter, anti-windup, visual tuning methodology.

16. **TinyMPC — Model Predictive Control for Microcontrollers**
    URL: `https://github.com/TinyMPC/TinyMPC`
    *Relevance:* Future path if progressing beyond LQR. Lightweight MPC for Cortex-M with balance system examples.
    *What to look for:* Fixed-size matrix operations, no dynamic allocation, deterministic iteration count.

---

## Dissent

### BNO055 fusion vs. custom complementary filter

- **Engineer:** Try BNO055 internal IMU mode fusion first, fall back to complementary filter only if latency is measured to be too high.
- **Embedded Developer:** More skeptical — data likely routes through ATSAMD11 (unknown latency), prefers Madgwick/Mahony on SAMD21 from raw sensor data for guaranteed low latency.
- **Chair's recommendation:** Characterize the BNO055 path first (latency measurement). If total sensor latency exceeds ~15ms, switch to complementary or Madgwick filter on SAMD21. Don't build the fallback until proven necessary.

### PID cascade vs. LQR state feedback

- **Engineer:** Recommends building the state-space model and designing LQR — naturally handles coupled angle-position dynamics.
- **Data Analyst / Embedded Developer:** Implicitly assume a PID cascade structure (inner angle, outer position).
- **Chair's recommendation:** Build the state-space model regardless. Start with PD tilt controller for initial balance proof-of-concept, then choose between cascade PID and LQR based on comfort level. Both work; LQR is theoretically cleaner, cascade PID is easier to tune incrementally.

---

## Recommended Next Steps

1. **Start with safety:** Implement the duty clamp (48%) and stall detection. 2-3 hours, protects hardware for all subsequent work.
2. **Characterize the unknowns:** Run the Motor Carrier timing test and BNO055 latency measurement. Determines whether the current software stack is viable for 100 Hz control.
3. **Build the model:** Measure CoG height, compute state-space matrices, verify open-loop eigenvalues. Unlocks principled controller design.
4. **Extend logging:** Add IMU angle and rate to CSV. Split the Python pipeline. Every experiment from this point captures the full state.
