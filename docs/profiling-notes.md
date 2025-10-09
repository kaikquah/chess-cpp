# Profiling Notes – Phase 5

## Bench Overview (`--bench`)
- **Default build**: `1.62 s` real, `1.41 s` user, `0.20 s` sys, ~`250 kNPS` (Eval Log enabled).
- **Sanitized build**: `4.35 s` real, `3.99 s` user, `0.34 s` sys, ~`94 kNPS`.
- Logging overhead: sanitizer run is ~2.7× slower, but node counts remain constant (407,302).

## Time-Control Scenario (`go wtime 3000 btime 1500 winc 500 binc 100 movestogo 12`)
- **Default build**: ~`0.46 s` wall time.
- **Sanitized build**: ~`0.49 s` wall time.
- Profiling output (Debug Log enabled):
  - `eval_ms=293.359`
  - `movegen_ms=110.448`
  - `negamax_ms=2520.133`
  - `qsearch_ms=674.587`
  - `eval_calls=50,384`
  - `movegen_calls=21,033`

### Observations
- Search time is dominated by negamax recursion (~2.5s cumulative) with quiescence contributing ~0.67s.
- Evaluation cost (~0.29s) is non-trivial; repeated calls (50k) highlight potential for caching/material incrementals.
- Move generation accounts for ~0.11s across 21k calls; optimising capture-only generation may help.

## Next Steps
- Investigate incremental evaluation or TT/pruning experiments to shave negamax/quiescence time.
- Consider batching time-manager logging to reduce debug overhead during profiling.
- Re-run metrics after any tuning to keep this log current.
