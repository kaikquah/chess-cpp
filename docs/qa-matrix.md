# QA Coverage Matrix

| Area | Coverage | Notes |
|------|----------|-------|
| Unit Tests | `tests/unit/*.cpp` (54 cases) | Core bitboard/board logic, move generation, search, evaluation, UCI loop. Includes eval breakdown regression and time-manager scenario matrix. |
| Perft Regression | `tests/unit/perft_tests.cpp` | Start position d1–d5, Kiwipete d1–d4, ChessProgramming boards 3–6 (depth 3–5), specialty pawn promotion FENs, and depth-6 endgame/en passant scenarios. |
| UCI Integration | `tests/unit/uci_tests.cpp` | Handshake, time controls, movetime, ponder flows, increment-only budget parsing, and Eval Log breakdown verification. |
| CLI Scenarios | `tests/unit/cli_tests.cpp` + `tests/cli_scenarios/*.txt` | Automated playback validates board/status output, time-budget logging, and trace toggles. |
| Sanitizer Builds | `CMakePresets.json` → `cmake --preset sanitize && cmake --build --preset sanitize && ctest --preset sanitize` | Enables ASan/UBSan for Debug builds; run after major changes. |
| Bench/Performance | `src/apps/perft`, `src/apps/chessbot --bench` | Baselines captured in `docs/bench-report.md`; profiling notes in `docs/profiling-notes.md`. |

## Open Gaps / Next Steps
- Ensure CLI scenario playback runs in CI (now covered by unit tests).
- Add dedicated regression for ponderhit once CLI loop supports it.
- Extend sanitizer runs to cover the CLI binary (`chessbot`) in addition to tests.
- Keep `docs/bench-report.md` refreshed after major tuning changes.

## Execution Recipes
- **Default build & test**: `cmake --preset default && cmake --build --preset default --target chessbot_tests && ctest --preset default`
- **Sanitizer sweep**: `cmake --preset sanitize && cmake --build --preset sanitize --target chessbot_tests && ctest --preset sanitize`
- **Perft spot-check**: `cmake --build build --target perft_tool && ./build/src/chessbot-perft <depth> [fen]`

Keep this matrix synced with new regressions as Phase 5 progresses.
