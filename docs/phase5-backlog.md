# Phase 5 Backlog – QA, Tuning, Stretch Goals

## Evaluation & Search Tuning
- [x] Run scripted evaluation suites (targeted FENs, mini self-play) using `Eval Log` to gather before/after metrics (see `tests/unit/evaluation_tests.cpp`).
- [x] Revisit material/PSQT/tempo weights; document conclusions in `docs/eval-notes.md` (no changes required).
- [ ] Prototype optional pruning/TT experiments behind build or runtime flags; benchmark impact on NPS and accuracy prior to rollout.

## Quality & Verification
- [x] Automate CLI scenario playback in CI (convert `tests/cli_scenarios` into scripted runs).
- [x] Extend perft coverage to include endgame mate puzzles and en passant edge cases at depth ≥6.
- [x] Execute sanitizer sweeps on the CLI binary (`chessbot`) in addition to the unit test harness.

## Performance & Time Management
- [x] Profile hotspots under representative time controls; document top optimization opportunities (`docs/profiling-notes.md`).
- [x] Tune default time-manager parameters (reserve ratio, overhead) based on profiling data (defaults now 20 ms safety, 15 ms overhead, 8% reserve).
- [x] Capture regression benchmarks after each tuning iteration and append to `docs/bench-report.md`.

## Release Readiness
- [x] Draft release notes using `docs/release-notes-template.md` (`docs/release-notes-draft.md`).
- [x] Update `README.md` / quick-start instructions with Phase 5 learnings and tuning workflow.
- [x] Assemble distribution checklist (artifacts, configs, regression scripts) for hand-off (`docs/distribution-checklist.md`).

_Update checkboxes as milestones are completed to maintain Phase 5 visibility._
