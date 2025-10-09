#include "test_framework.hpp"

#include "engine/board.hpp"
#include "engine/evaluation.hpp"

#include <string>
#include <vector>

using namespace chessbot;

CHESSBOT_TEST_CASE(evaluation_start_position_tempo_bias) {
  Board board;
  board.set_start_position();

  const auto result = evaluate(board);

  CHESSBOT_CHECK(result.material_balance_cp == 0);
  CHESSBOT_CHECK(result.psqt_midgame_cp == 0);
  CHESSBOT_CHECK(result.tempo_cp == 10);
  CHESSBOT_CHECK(result.score_cp == result.tempo_cp);
}

CHESSBOT_TEST_CASE(evaluation_detects_material_edge) {
  Board board;
  board.clear();
  board.set_piece(Square::E1, Piece::WhiteKing);
  board.set_piece(Square::E8, Piece::BlackKing);
  board.set_piece(Square::D4, Piece::WhiteQueen);
  board.set_side_to_move(Color::White);

  const auto result = evaluate(board);

  CHESSBOT_CHECK(result.material_balance_cp > 800);
  CHESSBOT_CHECK(result.white_minus_black_cp > 800);
  CHESSBOT_CHECK(result.score_cp > 800);
}

CHESSBOT_TEST_CASE(evaluation_side_to_move_orientation) {
  Board board;
  board.set_from_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
  const auto white_to_move = evaluate(board);

  board.set_from_fen("4k3/8/8/8/8/8/4P3/4K3 b - - 0 1");
  const auto black_to_move = evaluate(board);

  CHESSBOT_CHECK(white_to_move.material_balance_cp == black_to_move.material_balance_cp);
  CHESSBOT_CHECK(white_to_move.score_cp > 0);
  CHESSBOT_CHECK(black_to_move.score_cp < 0);
}

class TestEvalSink : public EvaluationLogSink {
public:
  struct Entry {
    std::string fen;
    EvaluationResult result;
  };

  void on_evaluation_log(const Board& board, const EvaluationResult& result) override {
    entries.push_back({board.to_fen(), result});
  }

  std::vector<Entry> entries;
};

class ScopedEvalLogging {
public:
  explicit ScopedEvalLogging(EvaluationLogSink* sink) : previous_config_(evaluation_config()) {
    EvaluationConfig config = previous_config_;
    config.enable_logging = true;
    set_evaluation_config(config);
    set_evaluation_log_sink(sink);
  }

  ~ScopedEvalLogging() {
    set_evaluation_log_sink(nullptr);
    set_evaluation_config(previous_config_);
  }

private:
  EvaluationConfig previous_config_;
};

CHESSBOT_TEST_CASE(evaluation_logging_reports_expected_breakdown) {
  TestEvalSink sink;
  ScopedEvalLogging guard(&sink);

  struct Fixture {
    std::string fen;
    int score;
    int abs;
    int material;
    int psqt_tapered;
    int psqt_mg;
    int psqt_eg;
    int tempo;
    int phase;
  };

  const std::vector<Fixture> fixtures = {
      {"startpos", 10, 0, 0, 0, 0, 0, 10, 24},
      {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", 257, 247, 100, 147, 68, 147, 10, 0},
      {"4k3/8/8/8/8/8/4P3/4K3 b - - 0 1", -257, 247, 100, 147, 68, 147, -10, 0},
      {"8/3k4/8/3Pp3/8/8/8/4K3 w - e6 0 1", -23, -33, 0, -33, -60, -33, 10, 0},
      {"rnbqkbnr/pppppppp/8/8/8/4K3/8/4k3 w - - 0 1", -4777, -4787, -4000, -787, -436, -1139, 10, 12},
  };

  for (const auto& fixture : fixtures) {
    Board board;
    if (fixture.fen == "startpos") {
      board.set_start_position();
    } else {
      board.set_from_fen(fixture.fen);
    }
    const EvaluationResult result = evaluate(board);
    CHESSBOT_CHECK(result.score_cp == fixture.score);
    CHESSBOT_CHECK(result.white_minus_black_cp == fixture.abs);
    CHESSBOT_CHECK(result.material_balance_cp == fixture.material);
    const int tapered = result.white_minus_black_cp - result.material_balance_cp;
    CHESSBOT_CHECK(result.psqt_midgame_cp == fixture.psqt_mg);
    CHESSBOT_CHECK(result.psqt_endgame_cp == fixture.psqt_eg);
    CHESSBOT_CHECK(result.tempo_cp == fixture.tempo);
    CHESSBOT_CHECK(result.game_phase == fixture.phase);
    CHESSBOT_CHECK(tapered == fixture.psqt_tapered);
  }

  CHESSBOT_CHECK(sink.entries.size() == fixtures.size());
  for (std::size_t i = 0; i < fixtures.size(); ++i) {
    const auto& entry = sink.entries[i];
    const auto& fixture = fixtures[i];
    if (fixture.fen == "startpos") {
      CHESSBOT_CHECK(entry.fen.find(" w ") != std::string::npos);
    } else {
      CHESSBOT_CHECK(entry.fen == fixture.fen);
    }
    CHESSBOT_CHECK(entry.result.score_cp == fixtures[i].score);
    CHESSBOT_CHECK(entry.result.white_minus_black_cp == fixtures[i].abs);
    CHESSBOT_CHECK(entry.result.material_balance_cp == fixtures[i].material);
    CHESSBOT_CHECK(entry.result.tempo_cp == fixtures[i].tempo);
  }
}
