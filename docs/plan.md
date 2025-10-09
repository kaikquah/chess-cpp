# Chess Engine MVP Plan

## Reference Insights
- **codfish** (`references/codfish/src/engine`): Clean bitboard-centric board/move abstractions, iterative-deepening negamax + quiescence, transposition table hooks, and dual CLI/UCI interfaces. Great template for board state API, move encoding, and simple evaluation heuristics.
- **Kingfish** (`references/Kingfish/src/kingfish`): Compact UCI-first engine using a 120-square board with magic-bitboard attack tables; demonstrates a minimal protocol loop, Zobrist hashing, and conversions between array and bitboard representations.
- **Chess-AI** (`references/Chess-AI/src/chess`): Complete rule coverage with undo stack, clear separation of game logic/UI, and accurate legality checks (castling, en passant, promotions). Ideal source for correctness, history tracking, and test scenarios.
- **CrazyAra** (`references/CrazyAra/engine/src`): Multithread-ready MCTS node/state separation with strong encapsulation. While we skip Python/NN pieces, the C++ engine informs future scalability and concurrency.
- **BitChess** (`references/BitChess/src`): Lightweight bitboard move generator with lookup tables, simple evaluation stubs, and XBoard protocol implementation—helpful for kick-starting attack masks, FEN parsing, and a baseline 1-ply search loop.

## Guiding Principles
- Pure C++20 (or latest available), single binary, CMake-driven build.
- Bitboard-based representation with clear `Board`, `Move`, and utility modules.
- UCI protocol support only (CLI helper optional).
- Correctness first: enforce full legality, repetition, and 50-move detection before advanced search features.
- Modular design so evaluator, move ordering, and search depth controls can evolve without major refactors.
- Keep MVP scope achievable: no neural nets, no multi-threading, no external tablebases, and no GUI.

## Phase Overview

### ✅ Phase 0 – Foundations (Complete)
Key artifacts delivered in `docs/`: charter, architecture sketch, build blueprint, QA plan, readiness checklist. Toolchain verified on Apple clang 17 + CMake 4.1.2 with GNU Make. Stakeholder requirements, scope boundaries, and QA strategy agreed.

### ✅ Phase 1 – Core Types & Board State (Complete)
- Implemented strong enums and helpers for colors, pieces, and squares (`src/core/types.*`).
- Added portable bitboard utilities with directional shifts, masks, and popcount/LSB helpers (`src/core/bitboard.*`).
- Built `Board` class covering placement, occupancy bitboards, FEN round-tripping, castling/en-passant tracking, and Zobrist hashing (`src/engine/board.*`, `src/engine/zobrist.*`).
- Created lightweight unit harness ensuring bitboard math, FEN parsing, and hash transitions behave as expected (`tests/unit/*`).
- CMake scaffolding compiles into `chessbot` executable and `chessbot-tests` binary via `cmake --build build && cmake --build build --target test`.

### ✅ Phase 2 – Move Generation & Legality (Complete)
- Added packed move representation, move list helpers, and reversible `MoveState` snapshots (`engine/move.*`).
- Built precomputed pawn/knight/king attack tables and sliding ray helpers feeding the new `Board::generate_*` APIs (`engine/attacks.*`).
- Implemented incremental make/unmake with Zobrist updates, castling/en passant tracking, and legality filtering via `is_square_attacked`.
- Delivered recursive perft engine and CLI (`engine/perft.*`, `apps/perft`) plus regression suites covering start position and Chess Programming Wiki boards (`tests/unit/perft_tests.cpp`).
- Expanded unit coverage for castling edge cases, en passant capture, pinned pieces, and state restoration guarantees.

### ✅ Phase 3 – Search & Evaluation (Complete)
- Implemented iterative-deepening negamax with alpha-beta pruning, capture-only quiescence, repetition/50-move draw detection, and stop-aware search limits.
- Added material + tapered PSQT evaluation with tempo bonus, exposing structured breakdowns for diagnostics.
- Delivered MVV-LVA, killer, and history heuristics plus node/time caps that feed the new UCI loop.

### ✅ Phase 4 – Engine Interface & Playability (Complete)
Delivered robust UCI protocol support, CLI ergonomics, time management polish, and integration hooks for external GUIs.

**Phase 4 summary**
- UCI loop now exposes a full option registry (`Hash`, `EvalTempoBonus`, `TimeSafetyMargin`, `MoveOverhead`, `TimeReservePercent`, `Ponder`, `Debug Log`) with persistence and validation.
- Adaptive time manager computes soft/hard budgets from clocks, increments, and movestogo; CLI mirrors those options and emits debug traces when enabled.
- CLI upgraded with UCI-parity commands (`setoption`, flexible `go` parameters, `ucinewgame`, bench mode), diagnostics (`board`, `history`, `status`, trace toggles), and optional logging (`--log`).
- Bench harness, CLI trace observer, and automated UCI/CLI scripts provide regression coverage.

See `docs/cli-guide.md` for full CLI command reference and timing-option documentation.

### 🔮 Phase 5 – QA, Tuning, Stretch Goals (Pending)
Perft/e2e regression suite, evaluation tuning, optional advanced pruning/TT, documentation polish.

**Goal**: Harden the MVP for release—prove correctness, polish strength, and ship documentation/backlogs for follow-up work.

**Workstreams & Phases**

1. **QA Expansion** (Owners: QA engineer, support from SWE)
   - Extend perft coverage (depth 6+ on critical FENs) and add tricky legality scenarios.
   - Script CLI/UCI regression sessions (time controls, movetime, ponder hit flows).
   - Integrate AddressSanitizer/UBSan debug builds into `cmake` presets; ensure clean runs.
   - Deliver `docs/qa-matrix.md` summarizing coverage and open QA gaps.

2. **Evaluation & Search Tuning** (Owners: Evaluation specialist + SWE)
   - Instrument evaluation breakdown logging (material, PSQT, tempo) with before/after snapshots.
   - Run targeted self-play or scripted positions to adjust weights; document rationale.
   - Prototype selective pruning/TT hooks guarded by compile flags; benchmark impact on nodes/s and accuracy.
   - Update unit tests to lock in tuned scores on canonical positions.

3. **Performance & Time Management Validation** (Owners: Perf engineer)
   - Capture baseline NPS across depth/bench configurations; store results in `docs/bench-report.md`.
   - Stress test new time-manager edge cases (low clocks, increment-only, movetime) via automated scripts.
   - Profile hotspots (Instruments/`perf`) and identify top 3 optimization opportunities.
   - Recommend parameter defaults (Hash, overhead, safety margins) based on empirical data.

4. **Release Readiness** (Owners: Project architect, technical writer)
   - Polish CLI/engine documentation (`docs/cli-guide.md`, `README.md`) with quick-start and troubleshooting.
   - Draft release notes including known limitations and stretch backlog (TT, pruning upgrades, pondering).
   - Prepare distribution artifacts checklist (build instructions, sample configs, regression scripts).
   - Align repo structure for hand-off (update root README, archive Phase plans, ensure options state defaults).

**Exit Criteria**
1. Automated test pipeline (unit + perft + scripted CLI/UCI) passes on clean build with sanitizers.
2. Evaluation/search tuning changes documented with before/after metrics and committed tests.
3. Benchmarks + profiling summary published in docs with recommended defaults.
4. Release notes and hand-off checklist approved; open risks logged for post-MVP roadmap.

**Next Immediate Actions**
1. ✅ Stand up sanitizer builds and add them to the CI script (QA Expansion). `cmake --preset sanitize` now mirrors the RelWithDebInfo flow and is exercised in tests.
2. ✅ Implement evaluation logging hooks to support tuning experiments (Eval/Search Tuning). `Eval Log` option emits structured breakdowns with new regression coverage and notes in `docs/eval-notes.md`.
3. ✅ Design automated time-manager regression suite (Performance Validation). New matrix lives in `tests/unit/time_manager_tests.cpp` with profiling traces available when debug logging is enabled.
4. ✅ Outline release notes template and assign documentation tasks (Release Readiness). See `docs/release-notes-template.md` and `docs/release-notes-draft.md`.

Perft/e2e regression suite, evaluation tuning, optional advanced pruning/TT, documentation polish.

**Goal**: Hardening the MVP for public release—prove correctness, polish play strength, and package the engine/documentation for external users.

**Primary workstreams**
- **Quality & Verification**: Expand perft/CLI/UCI regression suites, add sanitizer builds, and gate merges on automated runs.
- **Evaluation & Search Tuning**: Instrument eval breakdowns, adjust material/PSQT weights, and prototype targeted pruning/TT experiments with benchmarks.
- **Performance Profiling**: Capture NPS baselines, profile hotspots, and validate time management across diverse TC scripts.
- **Release Readiness**: Finalize CLI/engine documentation, produce a quick-start README, and capture known limitations/backlog.

**Exit criteria**
1. Automated test pipeline (unit, perft, scripted CLI/UCI) green on clean build.
2. Evaluation parameters tuned and documented with before/after comparisons.
3. Benchmarks recorded (NPS, time-budget adherence) and shared in docs/bench-report.md.
4. Release notes + hand-off checklist detailing remaining stretch items (TT, pruning, pondering) and risk log.

## Roles & QA
- **System Architect**: keeps architecture vision aligned with references.
- **Senior Software Engineer(s)**: implement modules per phase with thorough peer review.
- **QA Agent**: enforces tests, sanitizer runs, and VSCode/CI compatibility; validates every new source file before sign-off.

## Next Immediate Actions
1. ✅ Expand automated coverage: deeper perft matrix, new Eval Log UCI regression, and sanitizer preset executed.
2. ✅ Instrument evaluation/search metrics and begin tuning passes (Eval Log option + tests).
3. ✅ Document Phase 4 results and draft Phase 5 stretch backlog (pruning upgrades, TT integration, release checklist). See `docs/phase4-summary.md` and `docs/phase5-backlog.md`.
