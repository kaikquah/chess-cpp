#include "uci/uci.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace chessbot::uci {

namespace {

std::vector<std::string> tokenize(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::string> extract_moves(std::istringstream& iss) {
  std::vector<std::string> moves;
  std::string move;
  while (iss >> move) {
    moves.push_back(move);
  }
  return moves;
}

std::string trim(std::string_view view) {
  std::size_t start = 0;
  std::size_t end = view.size();
  while (start < end && std::isspace(static_cast<unsigned char>(view[start]))) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(view[end - 1]))) {
    --end;
  }
  return std::string(view.substr(start, end - start));
}

std::string to_lower(std::string_view view) {
  std::string out;
  out.reserve(view.size());
  for (char ch : view) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

std::string join_pv(const std::vector<Move>& pv) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < pv.size(); ++i) {
    if (i != 0) {
      oss << ' ';
    }
    oss << pv[i].to_uci();
  }
  return oss.str();
}

std::uint64_t safe_nps(std::uint64_t nodes, std::uint64_t elapsed_ms) {
  const std::uint64_t denominator = std::max<std::uint64_t>(1ULL, elapsed_ms);
  if (nodes >= std::numeric_limits<std::uint64_t>::max() / 1000ULL) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return (nodes * 1000ULL) / denominator;
}

}  // namespace

class LoopObserver : public SearchObserver {
public:
  explicit LoopObserver(UciLoop& loop) : loop_(loop) {}

  void on_iteration(const SearchIterationInfo& info) override {
    loop_.emit_info_iteration(info, 1);
  }

private:
  UciLoop& loop_;
};

UciLoop::UciLoop(Engine& engine, std::istream& in, std::ostream& out)
    : engine_(engine), in_(in), out_(out) {
  register_default_options(options_);
  options_.load_persistent_values();
  observer_ = std::make_unique<LoopObserver>(*this);
  set_evaluation_log_sink(this);
  apply_options();
}

UciLoop::~UciLoop() {
  set_evaluation_log_sink(nullptr);
}

void UciLoop::run() {
  running_.store(true, std::memory_order_relaxed);

  std::string line;
  while (running_.load(std::memory_order_relaxed) && std::getline(in_, line)) {
    handle_command(line);
  }

  stop_active_search();
}

void UciLoop::request_stop() {
  stop_active_search();
}

void UciLoop::handle_command(const std::string& line) {
  join_finished_search();

  const auto tokens = tokenize(line);
  if (tokens.empty()) {
    return;
  }

  const std::string& cmd = tokens[0];
  if (cmd == "uci") {
    cmd_uci();
  } else if (cmd == "isready") {
    cmd_isready();
  } else if (cmd == "ucinewgame") {
    cmd_ucinewgame();
  } else if (cmd == "position") {
    std::string rest = line.substr(cmd.size());
    cmd_position(rest);
  } else if (cmd == "setoption") {
    std::string rest = line.substr(cmd.size());
    cmd_setoption(rest);
  } else if (cmd == "go") {
    std::string rest = line.substr(cmd.size());
    cmd_go(rest);
  } else if (cmd == "debug") {
    std::string rest = line.substr(cmd.size());
    cmd_debug(rest);
  } else if (cmd == "register") {
    std::string rest = line.substr(cmd.size());
    cmd_register(rest);
  } else if (cmd == "ponderhit") {
    cmd_ponderhit();
  } else if (cmd == "stop") {
    cmd_stop();
  } else if (cmd == "quit") {
    cmd_quit();
  } else if (cmd == "eval") {
    stop_active_search();
    const auto eval = engine_.evaluate();
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string eval " << eval.score_cp << std::endl;
  } else if (cmd == "draw") {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string draw offer received" << std::endl;
  } else if (cmd == "resign") {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string resignation acknowledged" << std::endl;
  }
}

void UciLoop::cmd_uci() {
  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << "id name " << engine_.name() << std::endl;
  out_ << "id author Codex" << std::endl;
  for (const auto& option : options_.list_options()) {
    out_ << "option name " << option.name;
    switch (option.type) {
      case OptionType::kCheck:
        out_ << " type check default " << option.default_value;
        break;
      case OptionType::kSpin:
        out_ << " type spin default " << option.default_value
             << " min " << option.min
             << " max " << option.max;
        break;
      case OptionType::kString:
        out_ << " type string default " << option.default_value;
        break;
      case OptionType::kCombo:
        out_ << " type combo default " << option.default_value;
        for (const auto& var : option.combo_values) {
          out_ << " var " << var;
        }
        break;
    }
    out_ << std::endl;
  }
  out_ << "uciok" << std::endl;
}

void UciLoop::cmd_isready() {
  join_finished_search();
  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << "readyok" << std::endl;
}

void UciLoop::cmd_ucinewgame() {
  stop_active_search();
  engine_.new_game();
}

void UciLoop::cmd_position(const std::string& rest) {
  stop_active_search();
  std::istringstream iss(rest);
  std::string token;
  if (!(iss >> token)) {
    return;
  }

  bool ok = false;
  if (token == "startpos") {
    std::string moves_tag;
    if (iss >> moves_tag && moves_tag == "moves") {
      const auto moves = extract_moves(iss);
      ok = engine_.set_start_position(moves);
    } else {
      ok = engine_.set_start_position();
    }
  } else if (token == "fen") {
    std::array<std::string, 6> fen_parts{};
    for (std::size_t i = 0; i < fen_parts.size(); ++i) {
      if (!(iss >> fen_parts[i])) {
        fen_parts[0].clear();
        break;
      }
    }
    if (!fen_parts[0].empty()) {
      std::ostringstream fen_ss;
      for (std::size_t i = 0; i < fen_parts.size(); ++i) {
        if (i != 0) fen_ss << ' ';
        fen_ss << fen_parts[i];
      }
      std::string moves_tag;
      std::vector<std::string> moves;
      if (iss >> moves_tag && moves_tag == "moves") {
        moves = extract_moves(iss);
      }
      ok = engine_.set_position(fen_ss.str(), moves);
    }
  }

  if (!ok) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string position error" << std::endl;
  }
}

void UciLoop::cmd_go(const std::string& rest) {
  std::istringstream iss(rest);
  std::string token;
  chessbot::SearchLimits limits;
  std::optional<std::uint64_t> wtime;
  std::optional<std::uint64_t> btime;
  std::optional<std::uint64_t> winc;
  std::optional<std::uint64_t> binc;
  std::optional<std::uint64_t> movetime;
  std::optional<int> movestogo;
  bool ponder = false;

  while (iss >> token) {
    if (token == "depth") {
      int depth = 0;
      if (iss >> depth && depth > 0) {
        limits.depth = depth;
      }
    } else if (token == "movetime") {
      std::uint64_t ms = 0;
      if (iss >> ms) {
        movetime = ms;
      }
    } else if (token == "nodes") {
      std::uint64_t nodes = 0;
      if (iss >> nodes) {
        limits.node_limit = nodes;
      }
    } else if (token == "infinite") {
      limits.infinite = true;
    } else if (token == "ponder") {
      ponder = true;
    } else if (token == "wtime") {
      std::uint64_t ms = 0;
      if (iss >> ms) {
        wtime = ms;
      }
    } else if (token == "btime") {
      std::uint64_t ms = 0;
      if (iss >> ms) {
        btime = ms;
      }
    } else if (token == "winc") {
      std::uint64_t ms = 0;
      if (iss >> ms) {
        winc = ms;
      }
    } else if (token == "binc") {
      std::uint64_t ms = 0;
      if (iss >> ms) {
        binc = ms;
      }
    } else if (token == "movestogo") {
      int moves = 0;
      if (iss >> moves && moves > 0) {
        movestogo = moves;
      }
    }
  }

  stop_active_search();

  if (ponder && !pondering_enabled_) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string ponder ignored (disabled)" << std::endl;
    ponder = false;
  }

  if (ponder) {
    limits.infinite = true;
  }

  if (movetime && !limits.infinite) {
    TimeManagerRequest request;
    request.side_to_move = engine_.board().side_to_move();
    request.ponder = ponder;
    request.infinite = limits.infinite;
    request.move_time_ms = movetime;

    const TimeBudget budget = time_manager_.compute(request);
    if (budget.use_time) {
      limits.soft_time_limit_ms = budget.soft_limit_ms;
      limits.time_limit_ms = budget.hard_limit_ms;

      if (debug_mode_) {
        std::lock_guard<std::mutex> lock(io_mutex_);
        out_ << "info string time budget soft=" << budget.soft_limit_ms
             << " hard=" << budget.hard_limit_ms << std::endl;
      }
    }
  } else if (!limits.infinite) {
    TimeManagerRequest request;
    request.side_to_move = engine_.board().side_to_move();
    request.ponder = ponder;
    request.infinite = limits.infinite;
    request.white_time_ms = wtime;
    request.black_time_ms = btime;
    request.white_increment_ms = winc;
    request.black_increment_ms = binc;
    request.moves_to_go = movestogo;

    const TimeBudget budget = time_manager_.compute(request);
    if (budget.use_time) {
      limits.soft_time_limit_ms = budget.soft_limit_ms;
      limits.time_limit_ms = budget.hard_limit_ms;

      if (debug_mode_) {
        std::lock_guard<std::mutex> lock(io_mutex_);
        out_ << "info string time budget soft=" << budget.soft_limit_ms
             << " hard=" << budget.hard_limit_ms << std::endl;
      }
    }
  }

  awaiting_ponderhit_.store(ponder, std::memory_order_relaxed);
  pondering_.store(ponder, std::memory_order_relaxed);
  start_search(limits);
}

void UciLoop::cmd_setoption(const std::string& rest) {
  const std::string trimmed = trim(rest);
  if (trimmed.empty()) {
    return;
  }

  std::istringstream iss(trimmed);
  std::string keyword;
  if (!(iss >> keyword) || keyword != "name") {
    return;
  }

  std::string token;
  std::string name;
  bool saw_value_token = false;
  while (iss >> token) {
    if (token == "value") {
      saw_value_token = true;
      break;
    }
    if (!name.empty()) {
      name.push_back(' ');
    }
    name.append(token);
  }

  name = trim(name);
  if (name.empty()) {
    return;
  }

  std::string value;
  if (saw_value_token) {
    std::string remainder;
    std::getline(iss, remainder);
    value = trim(remainder);
  }

  const auto meta = options_.find_option(name);
  if (!meta) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string unknown option '" << name << "'" << std::endl;
    return;
  }

  if (value.empty()) {
    if (meta->type == OptionType::kCheck) {
      value = "true";
    } else {
      value = meta->default_value;
    }
  }

  if (!options_.set_option(name, value)) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string invalid value '" << value << "' for option '" << name << "'" << std::endl;
    return;
  }

  options_.persist();
  apply_options();

  if (debug_mode_) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    out_ << "info string option '" << name << "' set to '" << value << "'" << std::endl;
  }
}

void UciLoop::cmd_debug(const std::string& rest) {
  const std::string mode = to_lower(trim(rest));
  if (mode == "on") {
    debug_mode_ = true;
  } else if (mode == "off") {
    debug_mode_ = false;
  }
  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << "info string debug " << (debug_mode_ ? "on" : "off") << std::endl;
}

void UciLoop::cmd_register(const std::string& rest) {
  (void)rest;
  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << "info string registration not required" << std::endl;
}

void UciLoop::cmd_ponderhit() {
  pondering_.store(false, std::memory_order_relaxed);
  awaiting_ponderhit_.store(false, std::memory_order_relaxed);
  flush_deferred_result();
}

void UciLoop::cmd_stop() {
  stop_active_search();
}

void UciLoop::cmd_quit() {
  stop_active_search();
  running_.store(false, std::memory_order_relaxed);
}

void UciLoop::start_search(const SearchLimits& limits) {
  join_finished_search();

  {
    std::lock_guard<std::mutex> lock(result_mutex_);
    deferred_result_.reset();
  }

  search_active_.store(true, std::memory_order_release);
  search_thread_ = std::thread([this, limits]() {
    const auto result = engine_.search(limits, observer_.get());
    report_search_result(result);
    search_active_.store(false, std::memory_order_release);
  });
}

void UciLoop::stop_active_search() {
  if (search_active_.load(std::memory_order_acquire)) {
    engine_.request_stop();
  }
  if (search_thread_.joinable()) {
    search_thread_.join();
    search_thread_ = std::thread{};
  }
  search_active_.store(false, std::memory_order_release);
  awaiting_ponderhit_.store(false, std::memory_order_relaxed);
  pondering_.store(false, std::memory_order_relaxed);
  flush_deferred_result();
}

void UciLoop::join_finished_search() {
  if (search_thread_.joinable() && !search_active_.load(std::memory_order_acquire)) {
    search_thread_.join();
    search_thread_ = std::thread{};
  }
}

void UciLoop::report_search_result(const SearchResult& result) {
  if (awaiting_ponderhit_.load(std::memory_order_relaxed)) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    deferred_result_ = result;
    return;
  }
  publish_bestmove(result);
}

void UciLoop::emit_info_iteration(const SearchIterationInfo& info, int multipv) {
  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << "info depth " << info.depth;
  out_ << " seldepth " << info.seldepth;
  out_ << " multipv " << multipv;
  if (info.is_mate) {
    out_ << " score mate " << info.mate_in;
  } else {
    out_ << " score cp " << info.score_cp;
  }
  out_ << " nodes " << info.nodes;
  out_ << " nps " << info.nps;
  out_ << " time " << info.time_ms;
  if (!info.principal_variation.empty()) {
    out_ << " pv " << join_pv(info.principal_variation);
  }
  out_ << std::endl;
}

void UciLoop::publish_bestmove(const SearchResult& result) {
  std::lock_guard<std::mutex> lock(io_mutex_);
  const std::uint64_t elapsed = result.elapsed_ms;
  const std::uint64_t nps = safe_nps(result.nodes, elapsed);

  out_ << "info depth " << result.depth_completed
       << " seldepth " << result.seldepth
       << " multipv 1";
  if (result.is_mate) {
    out_ << " score mate " << result.mate_in;
  } else {
    out_ << " score cp " << result.score_cp;
  }
  out_ << " time " << result.elapsed_ms
       << " nodes " << result.nodes
       << " nps " << nps;
  if (!result.principal_variation.empty()) {
    out_ << " pv " << join_pv(result.principal_variation);
  }
  out_ << std::endl;

  const std::string best_move =
      result.principal_variation.empty() ? "0000" : result.principal_variation.front().to_uci();
  out_ << "bestmove " << best_move;
  if (pondering_enabled_ && result.principal_variation.size() >= 2) {
    out_ << " ponder " << result.principal_variation[1].to_uci();
  }
  out_ << std::endl;

  if (debug_mode_ && result.has_profiling) {
    const auto& stats = result.profiling;
    const auto to_ms = [](std::chrono::nanoseconds ns) {
      return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(ns).count()) / 1000.0;
    };
    out_ << std::fixed << std::setprecision(3);
    out_ << "info string profile eval_ms=" << to_ms(stats.evaluation_time)
         << " movegen_ms=" << to_ms(stats.movegen_time)
         << " negamax_ms=" << to_ms(stats.negamax_time)
         << " qsearch_ms=" << to_ms(stats.quiescence_time)
         << " eval_calls=" << stats.eval_calls
         << " movegen_calls=" << stats.movegen_calls << std::endl;
    out_.unsetf(std::ios::floatfield);
  }
}

void UciLoop::flush_deferred_result() {
  std::optional<SearchResult> snapshot;
  {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (deferred_result_) {
      snapshot = deferred_result_;
      deferred_result_.reset();
    }
  }

  if (snapshot) {
    publish_bestmove(*snapshot);
  }
}

void UciLoop::apply_options() {
  Engine::Options opts = engine_.options();
  opts = derive_engine_options(options_, opts);
  engine_.set_options(opts);
  pondering_enabled_ = opts.ponder_enabled;
  debug_mode_ = opts.debug_logging;
  const TimeManagerConfig tm_config = derive_time_manager_config(options_, opts);
  time_manager_.set_config(tm_config);
}

void UciLoop::on_evaluation_log(const Board& board, const EvaluationResult& result) {
  std::ostringstream oss;
  const std::string stm = board.side_to_move() == Color::White ? "w" : "b";
  const int tapered_psqt = result.white_minus_black_cp - result.material_balance_cp;
  oss << "info string eval breakdown"
      << " score=" << result.score_cp
      << " abs=" << result.white_minus_black_cp
      << " material=" << result.material_balance_cp
      << " psqt_tapered=" << tapered_psqt
      << " psqt_mg=" << result.psqt_midgame_cp
      << " psqt_eg=" << result.psqt_endgame_cp
      << " tempo=" << result.tempo_cp
      << " phase=" << result.game_phase
      << " stm=" << stm
      << " fen=\"" << board.to_fen() << "\"";

  std::lock_guard<std::mutex> lock(io_mutex_);
  out_ << oss.str() << std::endl;
}

}  // namespace chessbot::uci
