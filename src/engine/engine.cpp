
#include "engine.hpp"

#include <exception>

namespace chessbot {

namespace {

std::optional<Move> find_uci_move(Board& board, const std::string& uci, MoveGenerationType type) {
  MoveList moves;
  board.generate_legal_moves(moves, type);
  for (const Move& move : moves) {
    if (move.to_uci() == uci) {
      return move;
    }
  }
  return std::nullopt;
}

}  // namespace

Engine::Engine() : search_(config_) {
  new_game();
}

std::string Engine::name() const {
  return "ChessBot MVP";
}

void Engine::set_search_config(const SearchConfig& config) {
  config_ = config;
  search_.set_config(config_);
}

void Engine::set_options(const Options& options) {
  options_ = options;
  search_.set_hash_size(static_cast<std::size_t>(options_.hash_mb));
  EvaluationConfig eval_config;
  eval_config.tempo_bonus = options_.eval_tempo_bonus;
  eval_config.enable_logging = options_.eval_logging;
  set_evaluation_config(eval_config);
}

void Engine::new_game() {
  board_.set_start_position();
  repetition_history_.clear();
  repetition_history_.push_back(board_.hash());
}

bool Engine::set_start_position(const std::vector<std::string>& moves) {
  Board scratch;
  scratch.set_start_position();
  std::vector<std::uint64_t> history;
  history.push_back(scratch.hash());

  for (const auto& uci : moves) {
    auto move = find_uci_move(scratch, uci, MoveGenerationType::All);
    if (!move) {
      return false;
    }
    MoveState state;
    if (!scratch.make_move(*move, state)) {
      return false;
    }
    history.push_back(scratch.hash());
  }

  board_ = scratch;
  repetition_history_ = std::move(history);
  return true;
}

bool Engine::set_position(const std::string& fen, const std::vector<std::string>& moves) {
  Board scratch;
  try {
    scratch.set_from_fen(fen);
  } catch (const std::exception&) {
    return false;
  }

  std::vector<std::uint64_t> history;
  history.push_back(scratch.hash());

  for (const auto& uci : moves) {
    auto move = find_uci_move(scratch, uci, MoveGenerationType::All);
    if (!move) {
      return false;
    }
    MoveState state;
    if (!scratch.make_move(*move, state)) {
      return false;
    }
    history.push_back(scratch.hash());
  }

  board_ = scratch;
  repetition_history_ = std::move(history);
  return true;
}

bool Engine::play_move_uci(const std::string& uci) {
  auto move = find_move_by_uci(uci, MoveGenerationType::All);
  if (!move) {
    return false;
  }

  MoveState state;
  if (!board_.make_move(*move, state)) {
    return false;
  }
  repetition_history_.push_back(board_.hash());
  return true;
}

SearchResult Engine::search(const SearchLimits& limits, SearchObserver* observer) {
  search_.set_config(config_);
  search_.set_hash_size(static_cast<std::size_t>(options_.hash_mb));
  search_.set_profiling_enabled(options_.debug_logging);
  return search_.search(board_, limits, repetition_history_, observer);
}

void Engine::request_stop() {
  search_.request_stop();
}

EvaluationResult Engine::evaluate() const {
  return chessbot::evaluate(board_);
}

std::optional<Move> Engine::find_move_by_uci(const std::string& uci, MoveGenerationType type) {
  return find_uci_move(board_, uci, type);
}

}  // namespace chessbot
