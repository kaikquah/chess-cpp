# ChessBot MVP – Candidate Release Notes

## Overview
- **Version**: v0.5.0-rc1
- **Release Date**: 2024-07-18
- **Maintainers**: Core ChessBot team
- **Distribution Artifacts**:
  - `chessbot` (CLI + UCI engine)
  - `chessbot-perft` (perft regression tool)
  - `chessbot-tests` (unit/integration test bundle)

## Highlights
- UCI loop exposes persistent options (hash, tempo bonus, time margins, Eval Log) with full protocol compliance.
- New evaluation logging toggle + instrumentation captures material/PSQT/tempo breakdown per position for tuning.
- CLI automation now mirrors UCI workflows, including time-budget traces, scripted scenario playback, and bench harness.
- Time manager hardened with configurable safety/overhead parameters and regression matrix covering classical, movetime, and increment-only controls.

## Detailed Changes
### Engine & Search
- Iterative-deepening negamax with quiescence, killer/history heuristics, and repetition/50-move detection.
- Time manager computes soft/hard limits with reserve handling; sanitizer builds validated alongside release builds.
- Default timing knobs retuned (20 ms safety margin, 15 ms move overhead, 8% reserve) based on profiling data, with optional profiling metrics emitted when Debug Log is enabled.
- Added CLI playback tests invoking the built binaries to ensure protocol output remains stable.

### Evaluation & Tuning
- Evaluation result now tracks tempo, mid/endgame PSQT, absolute score, and logs structured breakdowns when enabled.
- Perft regression suite extended to depth-6 endgame/en passant scenarios to protect edge-case evaluation during tuning.

### Interface & Tooling
- CLI mirrors UCI options, supports logging/trace toggles, bench mode, and scripted scenario playback.
- Benchmarks captured for default and sanitized builds (see `docs/bench-report.md`), and `info string profile …` outputs surface eval/movegen/negamax/quiescence timings for tuning sessions.
- Release documentation refreshed: Phase 4 summary, Phase 5 backlog, QA matrix updates.

### Bug Fixes
- None beyond regression coverage tightening; no critical bugs outstanding for this milestone.

## Testing & Verification
- **Unit Tests**: `ctest --test-dir build`
- **Sanitizers**: `ctest --preset sanitize`
- **Benchmarks**: refer to `docs/bench-report.md` (default and ASan+UBSan runs)
- **CLI Scenarios**: automated via `tests/unit/cli_tests.cpp` consuming `tests/cli_scenarios/*.txt`

## Known Issues / Limitations
- Transposition table and advanced pruning remain stretch goals; feature flags planned for future experiments.
- No built-in pondering or multi-threading.
- CLI automation currently serial; large scenario suites may impact test runtimes (~10 s per full run).

## Backlog & Follow-Ups
- Evaluation retuning/self-play workflows (see `docs/phase5-backlog.md`).
- Performance profiling + default time-manager tuning based on empirical data.
- Finalize README/quick-start instructions and distribution checklist.

## Acknowledgements
- Engineering: Engine core, evaluation, and UCI loop by ChessBot MVP team.
- QA: Automated regression matrix, sanitizer coverage, CLI playback harness contributions.

---
Draft updated 2024-07-18; revise before tagging the release.
