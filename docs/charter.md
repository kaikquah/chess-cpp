# Project Charter: C++ Chess Engine MVP

## Mission Statement
Deliver a single-binary, UCI-compatible C++ chess engine suitable for interview demos. The engine must play complete games correctly, showcase clean architecture, and remain small enough to explain within minutes.

## Objectives
- Implement full chess rules with reliable move legality and repetition detection.
- Provide a fixed-depth search (target 4 plies) using alpha-beta with basic heuristics.
- Expose a minimal yet compliant UCI interface that interoperates with standard GUIs.
- Offer straightforward build/test workflows for macOS and Linux.

## Out-of-Scope
- Neural-network or NNUE evaluations (see `references/CrazyAra`).
- Multi-threaded or distributed search (defer patterns from `references/CrazyAra/engine/src`).
- Opening books, endgame tablebases, or external data files.
- GUIs or alternative protocols (e.g., XBoard from `references/BitChess`).

## Constraints
- Pure C++20 (fallback to C++17 only if compilers require it).
- Third-party libraries limited to Catch2 (vendored header) and standard library.
- Must compile via CMake ≥3.20 using stock compilers (clang/g++/MSVC optional).
- Preserve readable source structure to aid presentation and code review.

## Stakeholders & Roles
- **Lead Engineer (you)** – drives development/explanation during interview.
- **System Architect Agent** – defines structure, interfaces, and non-functional policies.
- **Senior Engine Engineer Agent** – implements board/move/search logic.
- **QA/Code Review Agent** – authors tests, runs perft validations, enforces coding standards.

## Success Criteria
- Engine completes sample matches in a UCI GUI without illegal moves or crashes.
- Perft positions up to depth 5 match known node counts.
- Build, test, and usage instructions fit within a concise README walkthrough.
- Architecture choices are defensible using references (e.g., `references/codfish/src/engine/board.hpp`).

## Reference Projects (subset)
- `references/codfish`: clean bitboard board/move abstractions and UCI loop.
- `references/Kingfish`: lightweight UCI handling and 120-square/bitboard bridge.
- `references/Chess-AI`: robust legality checks and undo history patterns.
- `references/BitChess`: lookup-based move generation tests and XBoard handling.
