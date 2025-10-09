# CLI Usage Guide

## Overview

The `chessbot` binary now ships with a feature-rich interactive CLI that mirrors the engine's UCI capabilities while providing additional diagnostics for manual testing and tuning. This document captures available commands, timing configuration options, and sample workflows.

## Launching

```bash
./chessbot               # Interactive CLI
./chessbot --uci         # Run as a UCI engine
./chessbot --bench       # Run built-in benchmark script
./chessbot --trace       # Emit search iteration info during CLI searches
./chessbot --log run.log # Mirror CLI input/output to run.log
```

Options can be combined, e.g. `./chessbot --trace --log session.txt`.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Print a summary of supported commands. |
| `new` / `ucinewgame` | Reset to the starting position and clear move history. |
| `position startpos [moves ...]` | Load the initial position and optionally replay moves. |
| `position fen <six FEN fields> [moves ...]` | Load an arbitrary FEN and optionally replay moves. |
| `setoption name <id> [value <x>]` | Adjust engine options (hash size, tempo bonus, timing margins, etc.). |
| `go <params>` | Launch a search using UCI-style parameters (`depth`, `movetime`, `wtime`, `btime`, `winc`, `binc`, `nodes`, `infinite`, `ponder`). |
| `move <uci>` | Manually play a move in UCI notation. |
| `board` | Render an ASCII board and display the current FEN. |
| `history` | Print the list of moves applied to the current game. |
| `status` | Show side to move and current static evaluation. |
| `eval` | Print detailed evaluation breakdown. |
| `trace on` / `trace off` | Toggle live search iteration logging (`info depth ...`). |
| `bench` | Run a three-position benchmark and report aggregate nodes/s. |
| `quit` / `exit` | Leave the CLI. |

All commands accept the same case-insensitive verbs as the UCI interface.

## Timing Options

The CLI and UCI layers expose additional knobs that tailor time management:

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `Hash` | Spin (1–4096) | `16` | Transposition table size in MB (future use). |
| `EvalTempoBonus` | Spin (-50 – 50) | `10` | Tempo bonus in centipawns added to root evaluation. |
| `TimeSafetyMargin` | Spin (0 – 1000) | `20` | Milliseconds reserved to avoid overstepping the clock. |
| `MoveOverhead` | Spin (0 – 1000) | `15` | Millisecond overhead accounting for GUI latency and housekeeping. |
| `TimeReservePercent` | Spin (0 – 50) | `8` | Percentage of remaining time to keep in reserve. |
| `Ponder` | Check | `false` | Enable pondering (UCI only). |
| `Debug Log` | Check | `false` | Emit additional diagnostics (UCI & CLI). |

Use `setoption name TimeSafetyMargin value 100` to update these parameters in the CLI. Values persist across runs via `options_state.ini`.

## Sample Workflow

```
> setoption name TimeSafetyMargin value 80
ok
> position startpos moves e2e4 e7e5
aok
> board
    +------------------------+
 8 | r  n  b  q  k  b  n  r |
...
> go depth 4
info depth 1 ...
bestmove g1f3 ponder d7d6
> history
1. e2e4 e7e5 2. ...
```

## Bench Mode

`./chessbot --bench` runs a fixed suite of three positions at depth 5–6, reporting per-case nodes and aggregate NPS. The engine resets to the initial position after the bench completes.

## Logging

Passing `--log <file>` writes every prompt, command, and engine response to the specified file. This is invaluable for reproducing issues or capturing search traces when combined with `--trace`.

## Trace Mode

`--trace` (or `trace on` within the CLI) hooks a search observer so each root iteration emits a standard UCI `info` line, allowing manual monitoring of depth, score, NPS, and PV progression.

## UCI Parity

Every `go` parameter supported through UCI is accepted by the CLI, meaning scripted smoke tests can be written once and reused in both environments.
