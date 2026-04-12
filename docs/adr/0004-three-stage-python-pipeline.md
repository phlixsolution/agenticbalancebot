---
id: 0004
title: Three-stage Python pipeline supersedes standalone scripts
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [tooling, data, python]
---

# ADR-0004: Three-stage Python pipeline supersedes standalone scripts

## Context

The repository contains several older Python scripts that grew up
alongside the early hardware experiments:

```
python/serial_to_csv.py     — recorded serial data to a CSV
python/plot_serial.py       — plotted live data from the serial port
python/plot_csv.py          — plotted a previously saved CSV
python/plot_pid_live.py     — plotted PID signals live
python/plot_sensor_live.py  — plotted sensor signals live
```

Each script was self-contained: it handled its own serial reading,
its own CSV parsing, and its own plotting in a single file. That was
fine for early exploration, but as the commissioning phases grew more
systematic, three problems emerged:

1. **No reuse between scripts.** If the serial column format changed,
   every script needed updating independently. There was no shared
   definition of "what a valid balance CSV looks like."

2. **No way to re-run one step.** To replot with different axis ranges
   you had to re-run the whole script — including re-reading the serial
   port if the data wasn't already saved, or re-parsing and re-computing
   everything if it was.

3. **No validation layer.** Raw serial data can contain partial lines,
   glitches, and timing noise. None of the old scripts had a dedicated
   step that checked the data before plotting it. Bad rows silently
   produced bad plots.

## Decision

The active pipeline is `acquire.py → process.py → analyze.py`.
Each stage has one job and a clear input/output contract:

| Stage | Input | Output | Job |
|---|---|---|---|
| `acquire.py` | Serial port | `messungen/raw/*.csv` | Read from the robot, write raw data, nothing else |
| `process.py` | raw CSV | `messungen/processed/*_proc.csv` | Validate rows, compute derived signals (dt, t_s, speed_rad_s, pos_diff, pid_out) |
| `analyze.py` | processed CSV | `messungen/plots/*.png` | Plot — pitch, PID components, duty, position, deadzone curves |

Shared constants (serial port, baud rate, column names, directory paths)
live in `config.py` so each stage reads the same definitions.

The old standalone scripts are **frozen**. They are kept in the repo for
reference but must not be extended. New measurement work uses the pipeline.

### Why this split matters

Because the stages are separate programs with files as the handoff,
each one can be re-run independently:

- **Re-process** an old raw recording with a fixed `process.py` (e.g.
  after correcting a unit conversion) without re-recording.
- **Re-plot** with different parameters without re-processing.
- **Validate** data once in `process.py`; `analyze.py` can trust its input.
- **Test sketches** (`carrier_timing`, `imu_latency`, `deadzone_id`)
  use `acquire.py` with a `--mode` flag — no need for separate recording
  scripts per test type.

## Consequences

**Positive**
- Column names, directory paths, and validation rules are defined once
  in `config.py` and imported by all three stages.
- A change to the serial log format (e.g. adding a new signal) means
  updating `config.py` and `process.py` only — `analyze.py` receives
  the new column in the processed CSV automatically.
- The directory split in ADR-0002 (`raw/`, `processed/`, `plots/`) maps
  directly to the three stages. The two decisions reinforce each other.

**Negative**
- Three commands instead of one. For a quick look at live data, the old
  `plot_serial.py` was faster to invoke. `analyze.py --live` partially
  addresses this but is a secondary use case.
- The old scripts are still present in `python/`, which can confuse a
  new reader. Mitigated by this ADR and by CLAUDE.md's "What not to
  touch" section.

**Neutral**
- `process.py` currently computes a fixed set of derived signals. When
  new commissioning phases require new quantities (e.g. frequency-domain
  analysis for Phase 5), they are added to `process.py` as new columns
  — the pipeline structure does not need to change.
