# Build Blueprint

## Toolchain Requirements
- CMake ≥ 3.20
- C++20-capable compiler (clang 14+, g++ 11+, or MSVC 19.30+)
- Ninja or Make (configurable via CMake generator)

## Target Layout
```
engine/         # static library with core, movegen, search, evaluation
apps/chessbot   # executable linking libengine + UCI/CLI front-end
apps/perft      # test harness for perft validation (optional during Phase 2)
```

## CMake Structure
- Top-level `CMakeLists.txt`
  - `project(chessbot VERSION 0.1 LANGUAGES CXX)`
  - Set `CMAKE_CXX_STANDARD 20`, enforce `CMAKE_CXX_STANDARD_REQUIRED ON`.
  - Global warnings: `-Wall -Wextra -Wpedantic` and treat as errors in CI builds (`-Werror`).
  - Configure sanitizer toggles via cache options (`CHESSBOT_ENABLE_ASAN`, `CHESSBOT_ENABLE_UBSAN`).
- `add_subdirectory(src)` for library sources; explicit file lists grouped by module.
- Option `CHESSBOT_BUILD_TESTS` to control test builds (default ON for development).
- Sanitizer toggles exposed via `CHESSBOT_ENABLE_ASAN` / `CHESSBOT_ENABLE_UBSAN` (used by presets below).
- Vendor Catch2 single-header under `external/catch2/catch.hpp`.

## Testing Targets
- `tests/unit` executable using Catch2; covers bitboard ops, board state, move generation, evaluation, and search sanity.
- `tests/perft` (executable or Catch2 section) to run known perft positions from FEN.

## Build Configurations
- Presets (`CMakePresets.json`):
  - `default`: `RelWithDebInfo`, tests enabled.
  - `sanitize`: `Debug` with ASan/UBSan enabled.
- Traditional cache options are still supported for IDE integration.

## Developer Workflow
1. `cmake --preset default`
2. `cmake --build --preset default`
3. `ctest --preset default` (or run `build/default/tests/chessbot-tests` directly)
4. `cmake --preset sanitize && cmake --build --preset sanitize && ctest --preset sanitize` for ASan/UBSan sweeps.
5. Run `build/default/src/chessbot` (or `--uci`) for CLI integration.

## Rationale
- Mirrors codfish’s separation of engine library and front-end (`references/codfish/CMakeLists.txt:18`) while staying explicit on file inclusion.
- Simplifies dependency management compared to BitChess’s GMock requirement (`references/BitChess/CMakeLists.txt:5`).
- Leaves hooks for future perft CLI and sanitizers without complicating MVP setup.
