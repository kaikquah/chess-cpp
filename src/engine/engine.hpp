#pragma once

#include <optional>
#include <string>
#include <vector>

#include "engine/board.hpp"
#include "engine/evaluation.hpp"
#include "engine/search.hpp"

namespace chessbot {

class Engine {
public:
  Engine();

  [[nodiscard]] std::string name() const;

  void set_search_config(const SearchConfig& config);
  struct Options {
    int hash_mb = 16;
    int eval_tempo_bonus = 10;
    bool ponder_enabled = false;
    bool debug_logging = false;
    bool eval_logging = false;
    int time_safety_margin_ms = 20;
    int move_overhead_ms = 15;
    int time_reserve_percent = 8;
  };

  void set_options(const Options& options);
  [[nodiscard]] Options options() const { return options_; }

  void new_game();
  bool set_start_position(const std::vector<std::string>& moves = {});
  bool set_position(const std::string& fen, const std::vector<std::string>& moves = {});
  bool play_move_uci(const std::string& uci);

  SearchResult search(const SearchLimits& limits, SearchObserver* observer = nullptr);
  void request_stop();
  [[nodiscard]] EvaluationResult evaluate() const;

  [[nodiscard]] const Board& board() const { return board_; }

private:
  std::optional<Move> find_move_by_uci(const std::string& uci, MoveGenerationType type);

  Board board_{};
  SearchConfig config_{};
  Search search_{};
  std::vector<std::uint64_t> repetition_history_{};
  Options options_{};
};

}  // namespace chessbot
