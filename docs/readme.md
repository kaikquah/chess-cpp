# ChessBot MVP

ChessBot MVP is a C++20 chess engine with UCI and CLI frontends, designed for experimentation with evaluation and search tuning.

## Build

```bash
cmake --preset default
cmake --build --preset default
```

## Test

```bash
ctest --test-dir build
ctest --preset sanitize
```

## Usage

- `build/src/chessbot --uci` for UCI mode.
- `build/src/chessbot` for an interactive CLI with commands such as `position`, `go`, `eval`, `trace`, and `bench`.
- `build/src/chessbot --bench` runs a fixed suite of benchmark positions.

See `docs/cli-guide.md` for detailed command reference and tuning workflow.

## Phase 5 Summary

- Evaluation logging instrumentation with structured breakdowns and tests.
- Automated CLI scenario playback and depth-6 perft regressions.
- Profiling hooks with default time-manager tuning.
- Release notes draft, QA matrix updates, and profiling documentation.

Refer to `docs/phase5-backlog.md` for remaining stretch goals.
