---
id: 0007
title: ruff as the Python linter for the data pipeline
status: accepted
date: 2026-04-13
deciders: [both collaborators]
tags: [python, tooling]
---

# ADR-0007: ruff as the Python linter for the data pipeline

## Context

The data pipeline (`acquire.py`, `process.py`, `analyze.py`) is shared
between two collaborators. Without a linter, style inconsistencies and
common mistakes (unused imports, wrong import order, outdated syntax)
accumulate silently and make the code harder to read over time.

The Python ecosystem has several established linting tools:
- **flake8** — style and error checking, widely used but slow, requires
  separate plugins for import sorting and syntax modernisation
- **pylint** — thorough but very slow and verbose; typically overkill for
  a small focused codebase
- **black** — formatter only, does not catch logical errors or imports
- **ruff** — covers style errors, warnings, undefined names, import
  sorting (`isort`), and syntax modernisation (`pyupgrade`) in a single
  tool; written in Rust and runs in milliseconds

For a two-person project running linting manually before commits, speed
and simplicity matter more than exhaustive analysis. Maintaining separate
tools for formatting, import sorting, and error checking adds friction
without proportional benefit.

## Decision

**ruff** is configured in `Balancebot_code/pyproject.toml` with the
following rule sets:

| Rule set | What it checks |
|---|---|
| `E`, `W` | pycodestyle style errors and warnings |
| `F` | pyflakes — undefined names, unused imports |
| `I` | import order (replaces isort) |
| `UP` | outdated Python syntax (replaces pyupgrade) |

`E501` (line too long) is suppressed globally — matplotlib and pandas
call chains make strict line limits impractical without hurting
readability.

The superseded standalone scripts (`plot_serial.py`, `serial_to_csv.py`,
`plot_csv.py`, `plot_pid_live.py`, `plot_sensor_live.py`) are excluded
from linting entirely (`ALL`). They are frozen reference artifacts
(ADR-0004); generating lint noise from them would obscure real issues
in the active pipeline.

Line length is set to 100 characters — a pragmatic middle ground between
the traditional 79 (PEP 8) and the unconstrained style of the pipeline's
data manipulation chains.

## Consequences

**Positive**
- One tool replaces flake8 + isort + pyupgrade. A single `ruff check`
  invocation covers all rule sets in under a second.
- Excluding the frozen scripts means lint output is signal only — every
  reported issue is in code that is actively maintained.
- `pyproject.toml` pins the rule sets and line length, so both
  collaborators get identical results regardless of ruff version.

**Negative**
- ruff does not auto-fix all violations (though `ruff check --fix`
  handles most `I` and `UP` rules). Some style errors still require
  manual correction.

**Neutral**
- ruff is not run as a commit hook by default. The Makefile's `lint`
  target (`make lint`) is the intended invocation point. Adding a
  pre-commit hook is possible but not required for a two-person project.
