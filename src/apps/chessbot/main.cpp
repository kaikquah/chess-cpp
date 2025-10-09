#include "engine/engine.hpp"
#include "engine/time_manager.hpp"
#include "uci/options.hpp"
#include "uci/uci.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

using chessbot::Color;
using chessbot::Engine;
using chessbot::Move;
using chessbot::SearchIterationInfo;
using chessbot::SearchLimits;
using chessbot::SearchObserver;
using chessbot::SearchResult;
using chessbot::TimeBudget;
using chessbot::TimeManager;
using chessbot::TimeManagerRequest;
using chessbot::uci::OptionRegistry;

class ScopedEvaluationSink {
public:
  explicit ScopedEvaluationSink(chessbot::EvaluationLogSink* sink)
      : active_(sink != nullptr) {
    if (active_) {
      chessbot::set_evaluation_log_sink(sink);
    }
  }

  ~ScopedEvaluationSink() {
    if (active_) {
      chessbot::set_evaluation_log_sink(nullptr);
    }
  }

  ScopedEvaluationSink(const ScopedEvaluationSink&) = delete;
  ScopedEvaluationSink& operator=(const ScopedEvaluationSink&) = delete;

private:
  bool active_ = false;
};

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

void log_line(std::ofstream* log, const std::string& line) {
  if (log && log->is_open()) {
    *log << line << '\n';
    log->flush();
  }
}

void print_line(const std::string& line, std::ofstream* log) {
  std::cout << line << std::endl;
  log_line(log, line);
}

std::uint64_t safe_nps(std::uint64_t nodes, std::uint64_t elapsed_ms) {
  const std::uint64_t denominator = std::max<std::uint64_t>(1ULL, elapsed_ms);
  if (nodes >= std::numeric_limits<std::uint64_t>::max() / 1000ULL) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return (nodes * 1000ULL) / denominator;
}

char piece_to_char(chessbot::Piece piece) {
  if (piece == chessbot::Piece::None) {
    return '.';
  }
  const chessbot::PieceType type = chessbot::type_of(piece);
  char symbol = '?';
  switch (type) {
    case chessbot::PieceType::Pawn:
      symbol = 'p';
      break;
    case chessbot::PieceType::Knight:
      symbol = 'n';
      break;
    case chessbot::PieceType::Bishop:
      symbol = 'b';
      break;
    case chessbot::PieceType::Rook:
      symbol = 'r';
      break;
    case chessbot::PieceType::Queen:
      symbol = 'q';
      break;
    case chessbot::PieceType::King:
      symbol = 'k';
      break;
    case chessbot::PieceType::None:
      symbol = '.';
      break;
  }
  if (chessbot::color_of(piece) == Color::White) {
    symbol = static_cast<char>(std::toupper(symbol));
  }
  return symbol;
}

void print_board_ascii(const chessbot::Board& board, std::ofstream* log) {
  print_line("    +------------------------+", log);
  for (int rank = 7; rank >= 0; --rank) {
    std::ostringstream row;
    row << ' ' << (rank + 1) << " |";
    for (int file = 0; file < 8; ++file) {
      const chessbot::Square square = chessbot::make_square(static_cast<std::uint8_t>(file), static_cast<std::uint8_t>(rank));
      const chessbot::Piece piece = board.piece_at(square);
      row << ' ' << piece_to_char(piece) << ' ';
    }
    row << '|';
    print_line(row.str(), log);
  }
  print_line("    +------------------------+", log);
  print_line("      a  b  c  d  e  f  g  h", log);
}

void print_move_history(const std::vector<std::string>& history, std::ofstream* log) {
  if (history.empty()) {
    print_line("No moves played yet.", log);
    return;
  }
  std::ostringstream oss;
  for (std::size_t idx = 0; idx < history.size(); ++idx) {
    if (idx % 2 == 0) {
      oss << (idx / 2 + 1) << ". ";
    }
    oss << history[idx] << ' ';
  }
  print_line(trim(oss.str()), log);
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

class CliObserver : public SearchObserver {
public:
  explicit CliObserver(std::ofstream* log) : log_(log) {}

  void on_iteration(const SearchIterationInfo& info) override {
    std::ostringstream oss;
    oss << "info depth " << info.depth
        << " seldepth " << info.seldepth
        << " multipv 1";
    if (info.is_mate) {
      oss << " score mate " << info.mate_in;
    } else {
      oss << " score cp " << info.score_cp;
    }
    oss << " nodes " << info.nodes
        << " nps " << info.nps
        << " time " << info.time_ms;
    if (!info.principal_variation.empty()) {
      oss << " pv " << join_pv(info.principal_variation);
    }
    print_line(oss.str(), log_);
  }

private:
  std::ofstream* log_;
};

class CliEvaluationLogger : public chessbot::EvaluationLogSink {
public:
  explicit CliEvaluationLogger(std::ofstream* log) : log_stream_(log) {}

  void set_log(std::ofstream* log) { log_stream_ = log; }

  void on_evaluation_log(const chessbot::Board& board,
                         const chessbot::EvaluationResult& result) override {
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
    print_line(oss.str(), log_stream_);
  }

private:
  std::ofstream* log_stream_ = nullptr;
};

void print_search_summary(const SearchResult& result, std::ofstream* log) {
  const std::uint64_t elapsed = result.elapsed_ms;
  const std::uint64_t nps = safe_nps(result.nodes, elapsed);

  std::ostringstream info;
  info << "info depth " << result.depth_completed
       << " seldepth " << result.seldepth
       << " multipv 1";
  if (result.is_mate) {
    info << " score mate " << result.mate_in;
  } else {
    info << " score cp " << result.score_cp;
  }
  info << " time " << result.elapsed_ms
       << " nodes " << result.nodes
       << " nps " << nps;
  if (!result.principal_variation.empty()) {
    info << " pv " << join_pv(result.principal_variation);
  }
  print_line(info.str(), log);

  const std::string best_move =
      result.principal_variation.empty() ? "0000" : result.principal_variation.front().to_uci();
  std::ostringstream best;
  best << "bestmove " << best_move;
  if (result.principal_variation.size() >= 2) {
    best << " ponder " << result.principal_variation[1].to_uci();
  }
  print_line(best.str(), log);

  if (result.has_profiling) {
    const auto to_ms = [](std::chrono::nanoseconds ns) {
      return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(ns).count()) / 1000.0;
    };
    std::ostringstream profile;
    profile << std::fixed << std::setprecision(3);
    profile << "info string profile eval_ms=" << to_ms(result.profiling.evaluation_time)
            << " movegen_ms=" << to_ms(result.profiling.movegen_time)
            << " negamax_ms=" << to_ms(result.profiling.negamax_time)
            << " qsearch_ms=" << to_ms(result.profiling.quiescence_time)
            << " eval_calls=" << result.profiling.eval_calls
            << " movegen_calls=" << result.profiling.movegen_calls;
    print_line(profile.str(), log);
  }
}

std::vector<std::string> collect_moves(std::istringstream& iss) {
  std::vector<std::string> moves;
  std::string token;
  while (iss >> token) {
    moves.push_back(token);
  }
  return moves;
}

bool handle_position_command(Engine& engine, const std::string& rest, std::vector<std::string>& history,
                             std::ofstream* log) {
  std::istringstream iss(rest);
  std::string token;
  if (!(iss >> token)) {
    print_line("error: expected 'startpos' or 'fen'", log);
    return false;
  }

  bool ok = false;
  if (token == "startpos") {
    std::string maybe_moves;
    if (iss >> maybe_moves) {
      if (maybe_moves != "moves") {
        print_line("error: unexpected token after startpos", log);
        return false;
      }
      const auto moves = collect_moves(iss);
      ok = engine.set_start_position(moves);
      if (ok) {
        history = moves;
      }
    } else {
      ok = engine.set_start_position();
      if (ok) {
        history.clear();
      }
    }
  } else if (token == "fen") {
    std::array<std::string, 6> fen_parts{};
    for (std::size_t i = 0; i < fen_parts.size(); ++i) {
      if (!(iss >> fen_parts[i])) {
        print_line("error: incomplete FEN", log);
        fen_parts[0].clear();
        break;
      }
    }
    if (!fen_parts[0].empty()) {
      std::ostringstream fen_builder;
      for (std::size_t i = 0; i < fen_parts.size(); ++i) {
        if (i != 0) {
          fen_builder << ' ';
        }
        fen_builder << fen_parts[i];
      }
      std::string moves_tag;
      std::vector<std::string> moves;
      if (iss >> moves_tag) {
        if (moves_tag != "moves") {
          print_line("error: expected 'moves' or end of line", log);
          return false;
        }
        moves = collect_moves(iss);
      }
      ok = engine.set_position(fen_builder.str(), moves);
      if (ok) {
        history = moves;
      }
    }
  } else {
    print_line("error: expected 'startpos' or 'fen'", log);
    return false;
  }

  if (ok) {
    print_line("ok", log);
  } else {
    print_line("error: could not set position", log);
  }
  return ok;
}

bool handle_setoption_command(const std::string& rest, OptionRegistry& options, Engine& engine,
                              TimeManager& time_manager, std::ofstream* log) {
  const std::string trimmed = trim(rest);
  if (trimmed.empty()) {
    print_line("error: expected option name", log);
    return false;
  }

  std::istringstream iss(trimmed);
  std::string keyword;
  if (!(iss >> keyword) || keyword != "name") {
    print_line("error: expected 'name'", log);
    return false;
  }

  std::string token;
  std::string name;
  bool saw_value = false;
  while (iss >> token) {
    if (token == "value") {
      saw_value = true;
      break;
    }
    if (!name.empty()) {
      name.push_back(' ');
    }
    name.append(token);
  }

  name = trim(name);
  if (name.empty()) {
    print_line("error: missing option name", log);
    return false;
  }

  std::string value;
  if (saw_value) {
    std::string remainder;
    std::getline(iss, remainder);
    value = trim(remainder);
  }

  const auto meta = options.find_option(name);
  if (!meta) {
    print_line("error: unknown option '" + name + "'", log);
    return false;
  }

  if (value.empty()) {
    value = (meta->type == chessbot::uci::OptionType::kCheck) ? "true" : meta->default_value;
  }

  if (!options.set_option(name, value)) {
    print_line("error: invalid value '" + value + "'", log);
    return false;
  }

  options.persist();
  Engine::Options updated = chessbot::uci::derive_engine_options(options, engine.options());
  engine.set_options(updated);
  time_manager.set_config(chessbot::uci::derive_time_manager_config(options, updated));

  if (updated.debug_logging) {
    print_line("info string option '" + name + "' set to '" + value + "'", log);
  } else {
    print_line("ok", log);
  }
  return true;
}

struct GoParseResult {
  bool ok = false;
  std::string error;
  SearchLimits limits{};
};

GoParseResult parse_go_command(const std::string& rest, Engine& engine, TimeManager& time_manager,
                               std::ofstream* log) {
  GoParseResult result;
  std::istringstream iss(rest);
  std::string token;
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
      if (!(iss >> depth) || depth <= 0) {
        result.error = "invalid depth";
        return result;
      }
      result.limits.depth = depth;
    } else if (token == "movetime") {
      std::uint64_t ms = 0;
      if (!(iss >> ms)) {
        result.error = "invalid movetime";
        return result;
      }
      movetime = ms;
    } else if (token == "nodes") {
      std::uint64_t nodes = 0;
      if (!(iss >> nodes)) {
        result.error = "invalid nodes";
        return result;
      }
      result.limits.node_limit = nodes;
    } else if (token == "infinite") {
      result.limits.infinite = true;
    } else if (token == "ponder") {
      ponder = true;
    } else if (token == "wtime") {
      std::uint64_t ms = 0;
      if (!(iss >> ms)) {
        result.error = "invalid wtime";
        return result;
      }
      wtime = ms;
    } else if (token == "btime") {
      std::uint64_t ms = 0;
      if (!(iss >> ms)) {
        result.error = "invalid btime";
        return result;
      }
      btime = ms;
    } else if (token == "winc") {
      std::uint64_t ms = 0;
      if (!(iss >> ms)) {
        result.error = "invalid winc";
        return result;
      }
      winc = ms;
    } else if (token == "binc") {
      std::uint64_t ms = 0;
      if (!(iss >> ms)) {
        result.error = "invalid binc";
        return result;
      }
      binc = ms;
    } else if (token == "movestogo") {
      int moves = 0;
      if (!(iss >> moves) || moves <= 0) {
        result.error = "invalid movestogo";
        return result;
      }
      movestogo = moves;
    } else {
      result.error = "unsupported go option: " + token;
      return result;
    }
  }

  if (ponder) {
    result.limits.infinite = true;
  }

  if (movetime && !result.limits.infinite) {
    TimeManagerRequest request;
    request.side_to_move = engine.board().side_to_move();
    request.ponder = ponder;
    request.infinite = result.limits.infinite;
    request.move_time_ms = movetime;

    const TimeBudget budget = time_manager.compute(request);
    if (budget.use_time) {
      result.limits.soft_time_limit_ms = budget.soft_limit_ms;
      result.limits.time_limit_ms = budget.hard_limit_ms;
      if (engine.options().debug_logging) {
        std::ostringstream oss;
        oss << "info string time budget soft=" << budget.soft_limit_ms
            << " hard=" << budget.hard_limit_ms;
        print_line(oss.str(), log);
      }
    }
  } else if (!result.limits.infinite) {
    TimeManagerRequest request;
    request.side_to_move = engine.board().side_to_move();
    request.ponder = ponder;
    request.infinite = result.limits.infinite;
    request.white_time_ms = wtime;
    request.black_time_ms = btime;
    request.white_increment_ms = winc;
    request.black_increment_ms = binc;
    request.moves_to_go = movestogo;

    const TimeBudget budget = time_manager.compute(request);
    if (budget.use_time) {
      result.limits.soft_time_limit_ms = budget.soft_limit_ms;
      result.limits.time_limit_ms = budget.hard_limit_ms;
      if (engine.options().debug_logging) {
        std::ostringstream oss;
        oss << "info string time budget soft=" << budget.soft_limit_ms
            << " hard=" << budget.hard_limit_ms;
        print_line(oss.str(), log);
      }
    }
  }

  if (result.limits.depth == 0 && result.limits.node_limit == 0 &&
      result.limits.time_limit_ms == 0 && !result.limits.infinite) {
    result.error = "no search parameters supplied";
    return result;
  }

  result.ok = true;
  return result;
}

void run_bench(Engine& engine, std::ofstream* log) {
  struct BenchCase {
    std::string description;
    std::string fen;
    std::vector<std::string> moves;
    int depth;
  };

  const std::vector<BenchCase> cases = {
      {"Start position", "startpos", {}, 5},
      {"Kingside attack", "rnbq1rk1/pppp1ppp/5n2/4p3/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 2 6", {"d4e5", "f6g4"}, 5},
      {"Endgame", "8/5k2/3p1p2/4p3/4P3/3P1P2/5K2/8 w - - 0 1", {}, 6},
  };

  print_line("Running bench...", log);
  const auto bench_start = std::chrono::steady_clock::now();
  std::uint64_t total_nodes = 0;

  for (const auto& test : cases) {
    bool ok = false;
    if (test.fen == "startpos") {
      ok = engine.set_start_position(test.moves);
    } else {
      ok = engine.set_position(test.fen, test.moves);
    }
    if (!ok) {
      print_line("bench error: unable to load position", log);
      continue;
    }

    SearchLimits limits;
    limits.depth = test.depth;
    const auto start = std::chrono::steady_clock::now();
    const SearchResult result = engine.search(limits);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto elapsed_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    total_nodes += result.nodes;

    std::ostringstream oss;
    oss << "bench " << test.description << " depth=" << test.depth
        << " nodes=" << result.nodes
        << " time=" << elapsed_ms << "ms";
    print_line(oss.str(), log);
  }

  const auto bench_elapsed = std::chrono::steady_clock::now() - bench_start;
  const auto total_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(bench_elapsed).count());
  const std::uint64_t nps = safe_nps(total_nodes, total_ms);
  std::ostringstream summary;
  summary << "bench total nodes=" << total_nodes
          << " time=" << total_ms << "ms"
          << " nps=" << nps;
  print_line(summary.str(), log);

  engine.new_game();
}

void print_help() {
  std::cout << "Commands:\n"
            << "  help                     Show this message\n"
            << "  new | ucinewgame         Reset to the starting position\n"
            << "  position startpos [moves ...]\n"
            << "  position fen <fen six fields> [moves ...]\n"
            << "  setoption name <id> [value <x>]\n"
            << "  go <params>              Start a search (depth/movetime/time controls)\n"
            << "  move <uci>               Play a move manually\n"
            << "  board                    Show the current board\n"
            << "  history                  Show played moves\n"
            << "  status                   Show side to move and evaluation\n"
            << "  eval                     Print detailed static evaluation\n"
            << "  trace on|off             Toggle live search trace\n"
            << "  bench                    Run a short benchmark\n"
            << "  quit                     Exit\n";
}

}  // namespace

int main(int argc, char** argv) {
  bool uci_mode = false;
  bool bench_mode = false;
  bool trace_enabled = false;
  std::optional<std::string> log_path;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--uci") {
      uci_mode = true;
    } else if (arg == "--bench") {
      bench_mode = true;
    } else if (arg == "--trace") {
      trace_enabled = true;
    } else if (arg == "--log" && i + 1 < argc) {
      log_path = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: " << argv[0] << " [--uci] [--bench] [--trace] [--log <file>]\n";
      return EXIT_SUCCESS;
    }
  }

  Engine engine;

  if (uci_mode) {
    chessbot::uci::UciLoop loop(engine, std::cin, std::cout);
    loop.run();
    return EXIT_SUCCESS;
  }

  OptionRegistry options;
  std::ofstream log_stream;
  CliEvaluationLogger eval_logger(&log_stream);
  ScopedEvaluationSink eval_sink(&eval_logger);
  register_default_options(options);
  options.load_persistent_values();

  Engine::Options engine_opts = chessbot::uci::derive_engine_options(options, engine.options());
  engine.set_options(engine_opts);

  TimeManager time_manager;
  time_manager.set_config(chessbot::uci::derive_time_manager_config(options, engine_opts));

  if (log_path) {
    log_stream.open(*log_path);
    if (!log_stream.is_open()) {
      std::cerr << "error: could not open log file" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (bench_mode) {
    run_bench(engine, log_path ? &log_stream : nullptr);
    return EXIT_SUCCESS;
  }

  print_line(engine.name() + " ready", log_path ? &log_stream : nullptr);
  print_help();

  std::vector<std::string> move_history;
  std::string line;
  while (true) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, line)) {
      break;
    }
    if (log_path) {
      log_stream << "> " << line << '\n';
      log_stream.flush();
    }

    std::istringstream iss(line);
    std::string command;
    if (!(iss >> command)) {
      continue;
    }

    command = to_lower(command);

    if (command == "quit" || command == "exit") {
      break;
    }
    if (command == "help") {
      print_help();
      continue;
    }
    if (command == "new" || command == "ucinewgame") {
      engine.new_game();
      move_history.clear();
      print_line("Position reset.", log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "position") {
      std::string rest;
      std::getline(iss, rest);
      handle_position_command(engine, rest, move_history, log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "setoption") {
      std::string rest;
      std::getline(iss, rest);
      handle_setoption_command(rest, options, engine, time_manager, log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "go") {
      std::string rest;
      std::getline(iss, rest);
      const GoParseResult parsed = parse_go_command(rest, engine, time_manager, log_path ? &log_stream : nullptr);
      if (!parsed.ok) {
        print_line("error: " + (parsed.error.empty() ? std::string("invalid go command") : parsed.error),
                   log_path ? &log_stream : nullptr);
        continue;
      }

      CliObserver observer(log_path ? &log_stream : nullptr);
      SearchObserver* observer_ptr = trace_enabled ? static_cast<SearchObserver*>(&observer) : nullptr;
      const SearchResult result = engine.search(parsed.limits, observer_ptr);
      print_search_summary(result, log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "move") {
      std::string uci;
      if (!(iss >> uci)) {
        print_line("error: expected move in UCI format", log_path ? &log_stream : nullptr);
        continue;
      }
      if (!engine.play_move_uci(uci)) {
        print_line("error: illegal move", log_path ? &log_stream : nullptr);
      } else {
        move_history.push_back(uci);
        print_line("ok", log_path ? &log_stream : nullptr);
      }
      continue;
    }
    if (command == "board") {
      print_board_ascii(engine.board(), log_path ? &log_stream : nullptr);
      print_line("FEN: " + engine.board().to_fen(), log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "history") {
      print_move_history(move_history, log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "status") {
      const auto eval = engine.evaluate();
      std::ostringstream oss;
      oss << "Side to move: " << (engine.board().side_to_move() == Color::White ? "white" : "black")
          << " | eval: " << eval.score_cp << " cp";
      print_line(oss.str(), log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "eval") {
      const auto eval = engine.evaluate();
      const int tapered_psqt = eval.white_minus_black_cp - eval.material_balance_cp;
      std::ostringstream oss;
      oss << "Static evaluation (cp): " << eval.score_cp << '\n'
          << "  abs: " << eval.white_minus_black_cp << '\n'
          << "  material: " << eval.material_balance_cp << '\n'
          << "  psqt tapered: " << tapered_psqt << '\n'
          << "  psqt mg: " << eval.psqt_midgame_cp << '\n'
          << "  psqt eg: " << eval.psqt_endgame_cp << '\n'
          << "  tempo: " << eval.tempo_cp << '\n'
          << "  phase: " << eval.game_phase;
      print_line(oss.str(), log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "trace") {
      std::string mode;
      if (!(iss >> mode)) {
        print_line("error: expected 'on' or 'off'", log_path ? &log_stream : nullptr);
        continue;
      }
      const std::string lowered = to_lower(mode);
      if (lowered == "on") {
        trace_enabled = true;
      } else if (lowered == "off") {
        trace_enabled = false;
      } else {
        print_line("error: expected 'on' or 'off'", log_path ? &log_stream : nullptr);
        continue;
      }
      print_line(std::string("Trace ") + (trace_enabled ? "enabled" : "disabled"),
                 log_path ? &log_stream : nullptr);
      continue;
    }
    if (command == "bench") {
      run_bench(engine, log_path ? &log_stream : nullptr);
      move_history.clear();
      print_line("Position reset after bench.", log_path ? &log_stream : nullptr);
      continue;
    }

    print_line("error: unknown command", log_path ? &log_stream : nullptr);
  }

  return EXIT_SUCCESS;
}
