#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>

#include "core/types.hpp"
#include "engine/move.hpp"

namespace chessbot {

class Board;

struct SearchConfig {
  int max_depth = 64;
  bool enable_quiescence = true;
};

struct SearchLimits {
  int depth = 0;                     // Fixed depth; 0 means use config max.
  std::uint64_t node_limit = 0;      // Optional hard node budget.
  std::uint64_t time_limit_ms = 0;   // Hard time budget (search forcibly stops at/after this).
  std::uint64_t soft_time_limit_ms = 0;  // Preferred budget; search stops once reached when possible.
  bool infinite = false;
};

struct SearchResult {
  Move best_move{};
  int score_cp = 0;                        // Relative to side to move at root.
  int depth_completed = 0;
  int seldepth = 0;
  bool is_mate = false;
  int mate_in = 0;
  bool is_stalemate = false;
  std::uint64_t nodes = 0;
  std::uint64_t elapsed_ms = 0;
  std::vector<Move> principal_variation;
  bool has_profiling = false;
  struct ProfilingStats {
    std::uint64_t eval_calls = 0;
    std::uint64_t movegen_calls = 0;
    std::chrono::nanoseconds negamax_time{};
    std::chrono::nanoseconds quiescence_time{};
    std::chrono::nanoseconds evaluation_time{};
    std::chrono::nanoseconds movegen_time{};
  } profiling;
};

struct SearchIterationInfo {
  int depth = 0;
  int seldepth = 0;
  int score_cp = 0;
  bool is_mate = false;
  int mate_in = 0;  // Positive: mate for side to move, negative: mate against.
  std::uint64_t nodes = 0;
  std::uint64_t time_ms = 0;
  std::uint64_t nps = 0;
  std::vector<Move> principal_variation;
};

class SearchObserver {
public:
  virtual ~SearchObserver() = default;
  virtual void on_iteration(const SearchIterationInfo& info) = 0;
};

class Search {
public:
  explicit Search(SearchConfig config = {});

  void set_config(SearchConfig config);
  void set_hash_size(std::size_t megabytes);

  SearchResult search(Board& board, const SearchLimits& limits,
                      const std::vector<std::uint64_t>& repetition_history = {},
                      SearchObserver* observer = nullptr);
  void request_stop();
  void set_profiling_enabled(bool enabled);

private:
  int negamax(Board& board, int depth, int ply_from_root, int alpha, int beta,
              std::vector<Move>& pv);
  int quiescence(Board& board, int alpha, int beta, int ply_from_root);
  int evaluate_with_profiling(const Board& board);
  int score_move(const Move& move, Color mover, int ply) const;
  void store_killer(int ply, Move move);
  void update_history(Color mover, const Move& move, int depth);
  bool should_stop() const;
  void reset_counters();
  void reset_iteration_state();
  void record_seldepth(int ply_from_root);
  void notify_iteration(const std::vector<Move>& pv, int depth, int score);

  static bool is_mate_score(int score);
  static int compute_mate_distance(int score);

  SearchConfig config_{};
  SearchLimits limits_{};

  std::atomic<bool> external_stop_{false};
  bool aborted_ = false;
  std::uint64_t nodes_ = 0;
  std::chrono::steady_clock::time_point start_time_{};
  SearchObserver* observer_ = nullptr;

  std::vector<std::array<Move, 2>> killer_moves_{};
  std::vector<std::uint64_t> repetition_hashes_{};
  std::array<std::array<std::array<int, kBoardSquareCount>, kBoardSquareCount>, 2> history_{};
  std::size_t hash_size_mb_ = 0;
  int current_seldepth_ = 0;
  bool profiling_enabled_ = false;
  SearchResult::ProfilingStats profiling_stats_{};
};

}  // namespace chessbot
