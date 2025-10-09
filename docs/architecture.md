# Architecture Sketch

## Module Overview
- **core/bitboard**: bit manipulations, masks, and population-count helpers inspired by `references/codfish/src/engine/bitboard.hpp` and validated against BitChess shift tests.
- **core/types**: enums and strong types for colors, pieces, and squares (mirrors `references/codfish/src/engine/constants.hpp`).
- **core/zobrist**: randomized hash keys for incremental updates (patterned after `references/Kingfish/src/kingfish/zobrist.cpp`).
- **core/board**: board state, occupancy masks, move history, repetition detection, and FEN parsing (borrowing undo ideas from `references/Chess-AI/src/chess/game.cpp`).
- **movegen**: pseudo-legal move generation per piece plus legality filtering via make/unmake (mixing BitChess lookup tables and Kingfish attack masks).
- **search**: iterative deepening negamax w/ alpha-beta, quiescence, simple move ordering (modelled on `references/codfish/src/engine` search files).
- **evaluation**: material + piece-square tables (PeSTO style) with future hooks for mobility.
- **uci**: minimal protocol loop handling `uci`, `isready`, `ucinewgame`, `position`, `go depth`, `stop`, `quit` commands; structure follows `references/Kingfish/src/kingfish/uci.cpp` and `references/codfish/src/interfaces/uci`.
- **cli (optional)**: debug commands for running perft and self-play without a GUI.

## Data Flow
1. `uci` parses commands into `EngineController` requests.
2. Controller updates `core/board` using `movegen` for legal move application.
3. `search` queries `movegen` to explore positions and uses `evaluation` to score leaf nodes.
4. `board` exposes hash/position features for repetition and transposition table (future) support.
5. `cli`/tests call the same controller to maintain consistent behavior.

## Key Simplifications
- Single-threaded search loop (contrast with CrazyAra's threaded nodes in `references/CrazyAra/engine/src/node.h`).
- No custom memory pools; rely on STL containers with reservation for PV/history data.
- Single executable linking a static `engine` library, matching codfish’s layout but with explicitly listed sources to keep MVP manageable.

## Future Extension Points
- Transposition table integration via `core/zobrist` fingerprints.
- Optional NNUE or external evaluation drop-in via `evaluation` interface.
- Parallel search once MVP stabilizes (drawing from CrazyAra design patterns).
