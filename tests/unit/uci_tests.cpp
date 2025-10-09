#include "test_framework.hpp"

#include <cstdio>
#include <sstream>
#include <string>

#include "engine/engine.hpp"
#include "uci/uci.hpp"

using namespace chessbot;

CHESSBOT_TEST_CASE(uci_basic_handshake_and_go) {
  Engine engine;

  std::istringstream input("uci\nisready\nposition startpos\ngo depth 1\nquit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("uciok") != std::string::npos);
  CHESSBOT_CHECK(out.find("readyok") != std::string::npos);
  CHESSBOT_CHECK(out.find("bestmove") != std::string::npos);
  CHESSBOT_CHECK(out.find("option name TimeSafetyMargin") != std::string::npos);
}

CHESSBOT_TEST_CASE(uci_go_with_time_controls) {
  Engine engine;

  std::istringstream input("uci\ndebug on\nposition startpos\ngo wtime 60000 btime 60000 winc 1000 binc 1000 movestogo 20\nquit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("bestmove") != std::string::npos);
  CHESSBOT_CHECK(out.find("info depth") != std::string::npos);
  CHESSBOT_CHECK(out.find("info string time budget") != std::string::npos);
}

CHESSBOT_TEST_CASE(uci_setoption_and_ponder_flow) {
  Engine engine;

  std::istringstream input(
      "uci\n"
      "setoption name EvalTempoBonus value 20\n"
      "setoption name Ponder value true\n"
      "setoption name UnknownOption value 1\n"
      "isready\n"
      "position startpos\n"
      "go ponder depth 1\n"
      "ponderhit\n"
      "quit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("option name Hash") != std::string::npos);
  CHESSBOT_CHECK(out.find("info string unknown option 'UnknownOption'") != std::string::npos);
  CHESSBOT_CHECK(out.find("info depth") != std::string::npos);
  CHESSBOT_CHECK(out.find("bestmove") != std::string::npos);
}

CHESSBOT_TEST_CASE(uci_debug_command) {
  Engine engine;

  std::istringstream input("uci\ndebug on\ndebug off\nquit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("info string debug on") != std::string::npos);
  CHESSBOT_CHECK(out.find("info string debug off") != std::string::npos);
}

CHESSBOT_TEST_CASE(uci_movetime_uses_time_manager) {
  Engine engine;

  std::istringstream input(
      "uci\n"
      "setoption name MoveOverhead value 25\n"
      "setoption name TimeSafetyMargin value 10\n"
      "debug on\n"
      "position startpos\n"
      "go movetime 100\n"
      "quit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("info string time budget soft=65 hard=75") != std::string::npos);
}

CHESSBOT_TEST_CASE(uci_increment_only_time_budget) {
  Engine engine;

  std::istringstream input(
      "uci\n"
      "debug on\n"
      "position startpos\n"
      "go wtime 0 winc 1000 movestogo 30\n"
      "quit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  const auto marker = out.find("info string time budget");
  CHESSBOT_CHECK(marker != std::string::npos);
  int soft = -1;
  int hard = -1;
  const int scanned = std::sscanf(out.c_str() + marker, "info string time budget soft=%d hard=%d", &soft, &hard);
  CHESSBOT_CHECK(scanned == 2);
  CHESSBOT_CHECK(soft > 0);
  CHESSBOT_CHECK(hard > 0);
}

CHESSBOT_TEST_CASE(uci_eval_logging_breakdown) {
  Engine engine;

  std::istringstream input(
      "uci\n"
      "setoption name Eval Log value true\n"
      "position startpos\n"
      "eval\n"
      "quit\n");
  std::ostringstream output;

  uci::UciLoop loop(engine, input, output);
  loop.run();

  const std::string out = output.str();
  CHESSBOT_CHECK(out.find("info string eval breakdown") != std::string::npos);
  CHESSBOT_CHECK(out.find(" tempo=") != std::string::npos);
}
