#include "test_framework.hpp"

#include "engine/board.hpp"
#include "engine/engine.hpp"
#include "engine/search.hpp"

using namespace chessbot;

CHESSBOT_TEST_CASE(search_prefers_immediate_capture) {
  Board board;
  board.set_from_fen("4k3/8/4q3/8/4Q3/8/8/4K3 w - - 0 1");

  Search search;
  SearchLimits limits;
  limits.depth = 1;

  const auto result = search.search(board, limits, {board.hash()});

  CHESSBOT_REQUIRE(!result.principal_variation.empty());
  CHESSBOT_CHECK(result.principal_variation.front().to_uci() == "e4e6");
}

CHESSBOT_TEST_CASE(search_detects_root_stalemate) {
  Board board;
  board.set_from_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");

  Search search;
  SearchLimits limits;
  limits.depth = 4;

  const auto result = search.search(board, limits, {board.hash()});

  CHESSBOT_CHECK(result.is_stalemate);
  CHESSBOT_CHECK(result.principal_variation.empty());
  CHESSBOT_CHECK(result.depth_completed == 0);
}

CHESSBOT_TEST_CASE(search_honors_node_limit) {
  Board board;
  board.set_start_position();

  Search search;
  SearchLimits limits;
  limits.depth = 5;
  limits.node_limit = 50;

  const auto result = search.search(board, limits, {board.hash()});

  CHESSBOT_CHECK(result.nodes <= limits.node_limit);
  CHESSBOT_CHECK(!result.principal_variation.empty());
}

CHESSBOT_TEST_CASE(engine_handles_position_and_search) {
  Engine engine;
  CHESSBOT_REQUIRE(engine.set_start_position({"e2e4", "e7e5"}));

  SearchLimits limits;
  limits.depth = 2;
  const auto result = engine.search(limits);

  CHESSBOT_CHECK(result.depth_completed >= 1);
  CHESSBOT_CHECK(!result.principal_variation.empty());
}

CHESSBOT_TEST_CASE(search_respects_time_limit) {
  Board board;
  board.set_start_position();

  Search search;
  SearchLimits limits;
  limits.depth = 6;
  limits.time_limit_ms = 5;

  const auto result = search.search(board, limits);

  CHESSBOT_CHECK(result.elapsed_ms <= 200);
}

CHESSBOT_TEST_CASE(quiescence_allows_quiet_evasions_when_in_check) {
  Board board;
  board.set_from_fen("3r2k1/8/8/3Q4/8/8/8/6K1 w - - 0 1");

  Search search;
  SearchLimits limits;
  limits.depth = 1;

  const auto result = search.search(board, limits, {board.hash()});

  CHESSBOT_CHECK(!result.is_mate);
}
