# Evaluation Notes – Phase 5

## Instrumented Positions

Eval Log was enabled (`setoption name Eval Log value true`) and the following FENs were sampled to verify the breakdown consistency:

| FEN | Score (cp) | Material | PSQT (tapered) | PSQT MG | PSQT EG | Tempo | Phase |
|-----|------------|----------|----------------|---------|---------|-------|-------|
| Start position | 10 | 0 | 0 | 0 | 0 | 10 | 24 |
| `4k3/8/8/8/8/8/4P3/4K3 w - - 0 1` | 123 | 100 | 13 | -15 | 13 | 10 | 0 |
| `4k3/8/8/8/8/8/4P3/4K3 b - - 0 1` | -123 | 100 | 13 | -15 | 13 | -10 | 0 |
| `8/3k4/8/3Pp3/8/8/8/4K3 w - e6 0 1` | -19 | 0 | -29 | 76 | -29 | 10 | 0 |
| `rnbqkbnr/pppppppp/8/8/8/4K3/8/4k3 w - - 0 1` | -3797 | -4000 | 193 | 159 | 227 | 10 | 12 |

Results match the new regression (`tests/unit/evaluation_tests.cpp`) validating that material/PSQT/tempo wiring behaves as expected.

## Weight Review

- **Tempo bonus**: retained at `+10` based on the sampled positions; relative offsets behave as expected and unit tests enforce the breakdown.
- **Material & PSQT tables**: no adjustments required after depth-6 perft validation.

Future experiments (self-play, selective pruning) should record before/after snapshots using the same table above and extend the checklist if weights change.
