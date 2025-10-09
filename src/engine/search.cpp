#include "engine/search.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "engine/board.hpp"
#include "engine/evaluation.hpp"

namespace chessbot {

namespace {

constexpr int kInfinity = 32000;
constexpr int kMateValue = 30000;
constexpr std::array<int, 6> kHeuristicPieceValues = {100, 320, 330, 500, 900, 20000};
constexpr int kMaxSupportedDepth = 256;
constexpr int kHistoryDepthCap = 1024;
constexpr int kHistoryMaxValue = 1'000'000;
constexpr int kHistoryReductionThreshold = 100000;

std::uint64_t compute_nps(std::uint64_t nodes, std::uint64_t elapsed_ms) {
  const std::uint64_t denominator = std::max<std::uint64_t>(1ULL, elapsed_ms);
  if (nodes >= std::numeric_limits<std::uint64_t>::max() / 1000ULL) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return (nodes * 1000ULL) / denominator;
}

bool is_in_check(const Board& board) {
  const Color us = board.side_to_move();
  const Square king_sq = board.king_square(us);
  const Color them = opposite(us);
  return is_valid(king_sq) && board.is_square_attacked(king_sq, them);
}

}  // namespace

Search::Search(SearchConfig config) : config_(config) {}

void Search::set_config(SearchConfig config) {
  config_ = config;
}

void Search::set_hash_size(std::size_t megabytes) {
  hash_size_mb_ = megabytes;
}

void Search::request_stop() {
  external_stop_.store(true, std::memory_order_relaxed);
}

void Search::set_profiling_enabled(bool enabled) {
  profiling_enabled_ = enabled;
  if (!profiling_enabled_) {
    profiling_stats_ = {};
  }
}

namespace {

class ScopedTimer {
public:
  ScopedTimer(bool enabled, std::chrono::nanoseconds* target)
      : enabled_(enabled), target_(target) {
    if (enabled_ && target_) {
      start_ = std::chrono::steady_clock::now();
    }
  }

  ~ScopedTimer() {
    if (enabled_ && target_) {
      const auto end = std::chrono::steady_clock::now();
      *target_ += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_);
    }
  }

private:
  bool enabled_ = false;
  std::chrono::steady_clock::time_point start_{};
  std::chrono::nanoseconds* target_ = nullptr;
};

}  // namespace

SearchResult Search::search(Board& board, const SearchLimits& limits,
                           const std::vector<std::uint64_t>& repetition_history,
                           SearchObserver* observer) {
  limits_ = limits;
  reset_counters();
  if (profiling_enabled_) {
    profiling_stats_ = {};
  }
  observer_ = observer;
  external_stop_.store(false, std::memory_order_relaxed);
  start_time_ = std::chrono::steady_clock::now();

  SearchResult result{};

  MoveList root_moves;
  {
    ScopedTimer movegen_timer(profiling_enabled_, &profiling_stats_.movegen_time);
    board.generate_legal_moves(root_moves, MoveGenerationType::All);
  }
  if (profiling_enabled_) {
    profiling_stats_.movegen_calls++;
  }
  const std::size_t root_move_count = root_moves.size();
  if (root_move_count == 0) {
    result.nodes = nodes_;
    result.elapsed_ms = 0;
    result.is_mate = is_in_check(board);
    result.is_stalemate = !result.is_mate;
    result.score_cp = result.is_mate ? -kMateValue : 0;
    result.depth_completed = 0;
    result.seldepth = 0;
    result.mate_in = result.is_mate ? compute_mate_distance(result.score_cp) : 0;
    return result;
  }

  repetition_hashes_ = repetition_history;
  const bool history_has_current =
      !repetition_hashes_.empty() && repetition_hashes_.back() == board.hash();
  const std::size_t base_history_size = repetition_hashes_.size();
  if (!history_has_current) {
    repetition_hashes_.push_back(board.hash());
  }

  int max_depth = limits.depth > 0 ? limits.depth : config_.max_depth;
  if (max_depth <= 0) {
    max_depth = 1;
  }
  max_depth = std::min(max_depth, kMaxSupportedDepth);

  const std::size_t killer_table_size = static_cast<std::size_t>(max_depth) + 32ULL;
  killer_moves_.assign(killer_table_size, std::array<Move, 2>{});
  for (auto& color_table : history_) {
    for (auto& from_table : color_table) {
      from_table.fill(0);
    }
  }

  for (int depth = 1; depth <= max_depth; ++depth) {
    if (should_stop()) {
      break;
    }

    reset_iteration_state();
    std::vector<Move> pv;
    const int score = negamax(board, depth, 0, -kInfinity, kInfinity, pv);

    if (aborted_) {
      break;
    }

    if (!pv.empty()) {
      result.best_move = pv.front();
      result.principal_variation = pv;
      result.depth_completed = depth;
      result.score_cp = score;
      result.seldepth = current_seldepth_;
      result.is_mate = std::abs(score) >= (kMateValue - depth);
      result.mate_in = result.is_mate ? compute_mate_distance(score) : 0;
      result.is_stalemate = false;
    }

    notify_iteration(pv, depth, score);
    result.nodes = nodes_;

    if (should_stop()) {
      break;
    }
  }

  const auto elapsed = std::chrono::steady_clock::now() - start_time_;
  result.elapsed_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
  result.nodes = nodes_;

  if (!history_has_current && repetition_hashes_.size() > base_history_size) {
    repetition_hashes_.pop_back();
  }

  if (result.principal_variation.empty() && root_move_count > 0) {
    result.best_move = root_moves[0];
    result.principal_variation = {root_moves[0]};
    result.score_cp = evaluate_score(board);
    result.depth_completed = 0;
    result.seldepth = current_seldepth_;
    result.is_mate = false;
    result.mate_in = 0;
  }

  if (board.has_insufficient_material()) {
    result.is_stalemate = true;
  }

  if (profiling_enabled_) {
    result.has_profiling = true;
    result.profiling = profiling_stats_;
  }

  observer_ = nullptr;
  return result;
}

int Search::negamax(Board& board, int depth, int ply_from_root, int alpha, int beta,
                    std::vector<Move>& pv) {
  ScopedTimer negamax_timer(profiling_enabled_, &profiling_stats_.negamax_time);
  record_seldepth(ply_from_root);
  if (should_stop()) {
    aborted_ = true;
    return 0;
  }

  nodes_++;

  if (board.halfmove_clock() >= 100) {
    return 0;
  }

  const std::uint64_t current_hash = board.hash();
  int repetition_count = 0;
  for (std::uint64_t hash : repetition_hashes_) {
    if (hash == current_hash) {
      ++repetition_count;
    }
  }
  if (repetition_count >= 3) {
    return 0;
  }

  if (board.has_insufficient_material()) {
    return 0;
  }

  if (depth == 0) {
    if (config_.enable_quiescence) {
      return quiescence(board, alpha, beta, ply_from_root);
    }
    return evaluate_with_profiling(board);
  }

  MoveList moves;
  {
    ScopedTimer movegen_timer(profiling_enabled_, &profiling_stats_.movegen_time);
    board.generate_legal_moves(moves, MoveGenerationType::All);
  }
  if (profiling_enabled_) {
    profiling_stats_.movegen_calls++;
  }

  if (moves.size() == 0) {
    const bool mate = is_in_check(board);
    return mate ? -(kMateValue - ply_from_root) : 0;
  }

  struct ScoredMove {
    Move move;
    int score;
  };

  std::vector<ScoredMove> ordered_moves;
  ordered_moves.reserve(moves.size());

  const Color mover = board.side_to_move();
  for (const Move& move : moves) {
    ordered_moves.push_back({move, score_move(move, mover, ply_from_root)});
  }
  std::sort(ordered_moves.begin(), ordered_moves.end(),
            [](const ScoredMove& lhs, const ScoredMove& rhs) {
              return lhs.score > rhs.score;
            });

  Move best_move;
  std::vector<Move> best_line;
  int best_score = -kInfinity;
  int current_alpha = alpha;

  for (const auto& entry : ordered_moves) {
    const Move& move = entry.move;
    MoveState state;
    if (!board.make_move(move, state)) {
      continue;
    }

    repetition_hashes_.push_back(board.hash());
    std::vector<Move> child_pv;
    const int score = -negamax(board, depth - 1, ply_from_root + 1, -beta, -current_alpha, child_pv);
    repetition_hashes_.pop_back();
    board.unmake_move(move, state);

    if (aborted_) {
      return 0;
    }

    if (should_stop()) {
      aborted_ = true;
      break;
    }

    const bool raised_alpha = score > current_alpha;

    if (score > best_score) {
      best_score = score;
      best_move = move;
      best_line = std::move(child_pv);
    }

    if (raised_alpha) {
      current_alpha = score;
    }

    if (current_alpha >= beta) {
      if (!move.is_capture()) {
        store_killer(ply_from_root, move);
        update_history(mover, move, depth);
      }
      break;
    }
    if (!move.is_capture() && raised_alpha) {
      update_history(mover, move, depth);
    }
  }

  pv.clear();
  if (!best_move.is_null() && !aborted_) {
    pv.push_back(best_move);
    pv.insert(pv.end(), best_line.begin(), best_line.end());
  }

  return best_score;
}

int Search::quiescence(Board& board, int alpha, int beta, int ply_from_root) {
  ScopedTimer quiescence_timer(profiling_enabled_, &profiling_stats_.quiescence_time);
  record_seldepth(ply_from_root);
  if (should_stop()) {
    aborted_ = true;
    return alpha;
  }

  nodes_++;

  if (board.halfmove_clock() >= 100) {
    return 0;
  }

  const std::uint64_t current_hash = board.hash();
  int repetition_count = 0;
  for (std::uint64_t hash : repetition_hashes_) {
    if (hash == current_hash) {
      ++repetition_count;
    }
  }
  if (repetition_count >= 3) {
    return 0;
  }

  if (board.has_insufficient_material()) {
    return 0;
  }

  const bool in_check = is_in_check(board);

  int stand_pat = std::numeric_limits<int>::min();
  if (!in_check) {
    stand_pat = evaluate_with_profiling(board);
    if (stand_pat >= beta) {
      return beta;
    }
    if (stand_pat > alpha) {
      alpha = stand_pat;
    }
  }

  MoveList moves;
  const MoveGenerationType gen_type = in_check ? MoveGenerationType::All : MoveGenerationType::Captures;
  {
    ScopedTimer movegen_timer(profiling_enabled_, &profiling_stats_.movegen_time);
    board.generate_legal_moves(moves, gen_type);
  }
  if (profiling_enabled_) {
    profiling_stats_.movegen_calls++;
  }

  if (moves.size() == 0) {
    if (in_check) {
      return -(kMateValue - ply_from_root);
    }
    return stand_pat;
  }

  struct ScoredMove {
    Move move;
    int score;
  };

  std::vector<ScoredMove> ordered;
  ordered.reserve(moves.size());
  const Color mover = board.side_to_move();

  for (const Move& move : moves) {
    ordered.push_back({move, score_move(move, mover, ply_from_root)});
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const ScoredMove& lhs, const ScoredMove& rhs) {
              return lhs.score > rhs.score;
            });

  for (const auto& entry : ordered) {
    const Move& move = entry.move;
    if (should_stop()) {
      aborted_ = true;
      break;
    }
    MoveState state;
    if (!board.make_move(move, state)) {
      continue;
    }
    repetition_hashes_.push_back(board.hash());
    const int score = -quiescence(board, -beta, -alpha, ply_from_root + 1);
    repetition_hashes_.pop_back();
    board.unmake_move(move, state);

    if (aborted_) {
      return alpha;
    }

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;
}

int Search::evaluate_with_profiling(const Board& board) {
  if (!profiling_enabled_) {
    return evaluate_score(board);
  }
  profiling_stats_.eval_calls++;
  ScopedTimer timer(true, &profiling_stats_.evaluation_time);
  return evaluate_score(board);
}

int Search::score_move(const Move& move, Color mover, int ply) const {
  if (move.is_capture()) {
    const int victim = kHeuristicPieceValues[static_cast<int>(move.captured_piece())];
    const int attacker = kHeuristicPieceValues[static_cast<int>(move.moving_piece())];
    return 10000 + 10 * victim - attacker;
  }

  if (move.is_promotion()) {
    const int promo = kHeuristicPieceValues[static_cast<int>(move.promotion_piece())];
    return 9000 + promo;
  }

  if (ply < static_cast<int>(killer_moves_.size())) {
    const auto& killers = killer_moves_[static_cast<std::size_t>(ply)];
    if (move.raw() == killers[0].raw()) {
      return 8000;
    }
    if (move.raw() == killers[1].raw()) {
      return 7900;
    }
  }

  const int color_idx = mover == Color::White ? 0 : 1;
  const int from = square_index(move.from());
  const int to = square_index(move.to());
  return history_[color_idx][from][to];
}

void Search::store_killer(int ply, Move move) {
  if (ply >= static_cast<int>(killer_moves_.size())) {
    return;
  }
  auto& killers = killer_moves_[static_cast<std::size_t>(ply)];
  if (killers[0].raw() == move.raw()) {
    return;
  }
  killers[1] = killers[0];
  killers[0] = move;
}

void Search::update_history(Color mover, const Move& move, int depth) {
  if (depth <= 0) {
    return;
  }
  const int color_idx = mover == Color::White ? 0 : 1;
  const int from = square_index(move.from());
  const int to = square_index(move.to());
  auto& entry = history_[color_idx][from][to];
  const int capped_depth = std::min(depth, kHistoryDepthCap);
  const std::int64_t bonus = static_cast<std::int64_t>(capped_depth) * capped_depth;
  const std::int64_t updated = static_cast<std::int64_t>(entry) + bonus;
  const std::int64_t clamped = std::min<std::int64_t>(updated, kHistoryMaxValue);
  entry = static_cast<int>(clamped);
  if (entry > kHistoryReductionThreshold) {
    entry /= 2;
  }
}

void Search::reset_iteration_state() {
  current_seldepth_ = 0;
}

void Search::record_seldepth(int ply_from_root) {
  if (ply_from_root > current_seldepth_) {
    current_seldepth_ = ply_from_root;
  }
}

void Search::notify_iteration(const std::vector<Move>& pv, int depth, int score) {
  if (observer_ == nullptr || pv.empty()) {
    return;
  }
  SearchIterationInfo info;
  info.depth = depth;
  info.seldepth = current_seldepth_;
  info.score_cp = score;
  info.is_mate = is_mate_score(score);
  if (info.is_mate) {
    info.mate_in = compute_mate_distance(score);
  }
  info.principal_variation = pv;
  info.nodes = nodes_;

  const auto now = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
  const auto elapsed_ms = elapsed.count();
  info.time_ms = elapsed_ms > 0 ? static_cast<std::uint64_t>(elapsed_ms) : 0ULL;
  info.nps = compute_nps(info.nodes, info.time_ms);

  observer_->on_iteration(info);
}

bool Search::is_mate_score(int score) {
  constexpr int kMateThreshold = kMateValue - 1000;
  return std::abs(score) >= kMateThreshold;
}

int Search::compute_mate_distance(int score) {
  const int distance = kMateValue - std::abs(score);
  const int plies = (distance + 1) / 2;
  return score > 0 ? plies : -plies;
}

bool Search::should_stop() const {
  if (external_stop_.load(std::memory_order_relaxed)) {
    return true;
  }
  if (limits_.infinite) {
    return false;
  }
  if (limits_.node_limit > 0 && nodes_ >= limits_.node_limit) {
    return true;
  }
  if (limits_.soft_time_limit_ms > 0 || limits_.time_limit_ms > 0) {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
    if (limits_.soft_time_limit_ms > 0 &&
        elapsed_ms >= static_cast<std::int64_t>(limits_.soft_time_limit_ms)) {
      return true;
    }
    if (limits_.time_limit_ms > 0 &&
        elapsed_ms >= static_cast<std::int64_t>(limits_.time_limit_ms)) {
      return true;
    }
  }
  return false;
}

void Search::reset_counters() {
  nodes_ = 0;
  aborted_ = false;
  current_seldepth_ = 0;
}

}  // namespace chessbot
