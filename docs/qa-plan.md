# QA & Validation Plan

## Testing Strategy
- **Unit Tests (Catch2)**
  - Bitboard operations: shifts, masks, popcount (guided by `references/BitChess/test/util/test_bitboard.cpp`).
  - Board state: FEN parsing, move application, repetition detection, hash stability.
  - Move generation: per-piece pseudo-legal moves, legal filtering (castling, en passant, promotions).
  - Evaluation: sanity checks for material/PSQT scores, mate detection edge cases.
  - Search: fixed-depth correctness on curated positions (mates-in-1, stalemate, drawn scenarios).

- **Perft Regression**
  - Provide canonical FENs (start position, Kiwipete, tricky positions) with known node counts up to depth 5.
  - Implement CLI/test harness to run perft quickly; fail builds when counts diverge.

- **Integration Tests**
  - Script deterministic UCI command sequences (e.g., `uci`, `isready`, load FEN, `go depth 2`) and assert outputs.
  - Optional loopback tests against lightweight engines once MVP is stable (manual for now).

## Tooling & Automation
- `ctest` integration for unit tests.
- Optional `ninja test` alias.
- Sanitizer builds (`ENABLE_ASAN=ON`) for Debug runs on supported platforms.
- Static analysis (clang-tidy) can be added later; not required for MVP demo.

## Quality Gates
- All unit and perft tests must pass before merging Phase 1+ changes.
- Zero compilation warnings on supported compilers.
- README instructions verified on clean build directory.

## Deliverables
- `tests/` directory with Catch2 harness and data files for perft.
- `docs/test-matrix.md` (Phase 5) summarizing coverage and open gaps.
- Issue backlog for known limitations and future enhancements.
