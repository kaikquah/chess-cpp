#pragma once

#include <cstdint>

#include "core/types.hpp"

namespace chessbot {

class Board;

struct EvaluationResult {
  int score_cp = 0;               // Perspective of the side to move.
  int white_minus_black_cp = 0;   // Absolute score (white positive).
  int material_balance_cp = 0;    // Material difference (white positive).
  int psqt_midgame_cp = 0;        // Midgame positional balance (white positive).
  int psqt_endgame_cp = 0;        // Endgame positional balance (white positive).
  int game_phase = 0;             // 0..24 where 24 represents a pure midgame.
  int tempo_cp = 0;               // Tempo contribution (relative to side to move).
};

struct EvaluationConfig {
  int tempo_bonus = 10;
  bool enable_logging = false;
};

void set_evaluation_config(const EvaluationConfig& config);
[[nodiscard]] EvaluationConfig evaluation_config();

class EvaluationLogSink {
public:
  virtual ~EvaluationLogSink() = default;
  virtual void on_evaluation_log(const Board& board, const EvaluationResult& result) = 0;
};

void set_evaluation_log_sink(EvaluationLogSink* sink);

[[nodiscard]] EvaluationResult evaluate(const Board& board);
[[nodiscard]] inline int evaluate_score(const Board& board) {
  return evaluate(board).score_cp;
}

}  // namespace chessbot
