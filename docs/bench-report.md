# Benchmarks – Phase 5 Baseline

Captured on Apple clang 17 (RelWithDebInfo default preset) using the built-in `--bench` command.

## Summary

| Build preset | Total nodes | Elapsed (ms) | NPS |
|--------------|-------------|--------------|-----|
| `default` (`cmake --preset default`) | 407,302 | 1,626 | 250,493 |
| `sanitize` (`cmake --preset sanitize`) | 407,302 | 4,318 | 94,326 |

Both runs executed the three bundled bench positions (start position @ depth 5, kingside attack @ depth 5, endgame @ depth 6). Evaluation logging was enabled during capture, so verbose `info string eval breakdown` lines appear in the console; disable the `Eval Log` option for quieter benchmarking.

## Reproduction

```bash
# Default build
cmake --preset default
cmake --build --preset default --target chessbot
./build/default/src/chessbot --bench

# Sanitized build (ASan + UBSan)
cmake --preset sanitize
cmake --build --preset sanitize --target chessbot
./build/sanitize/src/chessbot --bench
```

## Notes
- Sanitizers roughly double wall-clock time versus the RelWithDebInfo build; node counts stay identical, confirming logical parity.
- Additional profiling (see `docs/profiling-notes.md`) shows search time dominated by negamax recursion with ~0.29 s spent in evaluation.
- Future tuning experiments should record before/after NPS deltas in this file to maintain a historical log.
