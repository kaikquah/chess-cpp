# ChessBot MVP

ChessBot MVP is a C++20 chess engine with both UCI and interactive CLI front-ends. The project focuses on clean architecture, rich diagnostics, and instrumentation for evaluation/search tuning.

## Getting Started

```bash
cmake --preset default
cmake --build --preset default
```

Run the unit and integration suite (≈10 s because CLI scenarios exercise real searches):

```bash
ctest --test-dir build
ctest --preset sanitize  # ASan + UBSan
```

## Usage

- `build/src/chessbot --uci` — standard UCI loop suitable for GUIs.
- `build/src/chessbot` — interactive CLI mirroring UCI commands (`position`, `go`, `eval`, `trace`, `bench`, etc.).
- `build/src/chessbot --bench` — three-position benchmark with node/time/NPS reporting.

Debug mode (`setoption name Debug Log value true` or CLI `trace on`) now emits `info string profile …` lines summarising evaluation, move generation, negamax, and quiescence timings plus call counts—ideal for tuning experiments.

See `docs/cli-guide.md` for the full command reference and tuning workflow.

## Phase 5 Highlights

- Evaluation breakdown logging with unit coverage for representative FENs.
- Automated CLI scenario playback and depth-6 perft regression to guard edge cases.
- Built-in profiling hooks and retuned time-manager defaults (20 ms safety, 15 ms overhead, 8 % reserve).
- Updated documentation: profiling notes, bench baselines, release draft, backlog tracking.

Remaining stretch goals are tracked in `docs/phase5-backlog.md`.
