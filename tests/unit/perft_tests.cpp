#include "test_framework.hpp"

#include "engine/board.hpp"
#include "engine/perft.hpp"

#include <array>
#include <cstdint>

using namespace chessbot;

CHESSBOT_TEST_CASE(perft_start_position_baseline) {
  Board board;
  board.set_start_position();

  CHESSBOT_CHECK(perft(board, 1) == 20);
  CHESSBOT_CHECK(perft(board, 2) == 400);
  CHESSBOT_CHECK(perft(board, 3) == 8902);
  CHESSBOT_CHECK(perft(board, 4) == 197281);
  CHESSBOT_CHECK(perft(board, 5) == 4865609);
}

CHESSBOT_TEST_CASE(perft_kiwipete_reference) {
  Board board;
  board.set_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  CHESSBOT_CHECK(perft(board, 1) == 48);
  CHESSBOT_CHECK(perft(board, 2) == 2039);
  CHESSBOT_CHECK(perft(board, 3) == 97862);
  CHESSBOT_CHECK(perft(board, 4) == 4085603);
}

CHESSBOT_TEST_CASE(perft_chessprogramming_board3) {
  Board board;
  board.set_from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
  CHESSBOT_CHECK(perft(board, 1) == 14);
  CHESSBOT_CHECK(perft(board, 2) == 191);
  CHESSBOT_CHECK(perft(board, 3) == 2812);
  CHESSBOT_CHECK(perft(board, 4) == 43238);
  CHESSBOT_CHECK(perft(board, 5) == 674624);
}

CHESSBOT_TEST_CASE(perft_chessprogramming_board4) {
  Board board;
  board.set_from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
  CHESSBOT_CHECK(perft(board, 1) == 6);
  CHESSBOT_CHECK(perft(board, 2) == 264);
  CHESSBOT_CHECK(perft(board, 3) == 9467);
  CHESSBOT_CHECK(perft(board, 4) == 422333);
}

CHESSBOT_TEST_CASE(perft_chessprogramming_board5) {
  Board board;
  board.set_from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
  CHESSBOT_CHECK(perft(board, 1) == 44);
  CHESSBOT_CHECK(perft(board, 2) == 1486);
  CHESSBOT_CHECK(perft(board, 3) == 62379);
  CHESSBOT_CHECK(perft(board, 4) == 2103487);
}

CHESSBOT_TEST_CASE(perft_chessprogramming_board6) {
  Board board;
  board.set_from_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
  CHESSBOT_CHECK(perft(board, 1) == 46);
  CHESSBOT_CHECK(perft(board, 2) == 2079);
  CHESSBOT_CHECK(perft(board, 3) == 89890);
}

CHESSBOT_TEST_CASE(perft_en_passant_and_underpromotion) {
  Board board;
  board.set_from_fen("8/P7/8/4k3/8/4K3/7p/8 w - - 0 1");
  CHESSBOT_CHECK(perft(board, 1) == 9);
  CHESSBOT_CHECK(perft(board, 2) == 90);
  board.set_from_fen("4k3/1P6/8/8/8/8/6p1/4K3 w - - 0 1");
  CHESSBOT_CHECK(perft(board, 1) == 8);
  CHESSBOT_CHECK(perft(board, 2) == 59);
}

CHESSBOT_TEST_CASE(perft_endgame_mate_depth6) {
  Board board;
  board.set_from_fen("8/8/8/8/8/5k2/8/4K3 w - - 0 1");
  CHESSBOT_CHECK(perft(board, 6) == 26222ULL);
}

CHESSBOT_TEST_CASE(perft_en_passant_depth6) {
  Board board;
  board.set_from_fen("8/3k4/8/3Pp3/8/8/8/4K3 w - e6 0 1");
  CHESSBOT_CHECK(perft(board, 6) == 111613ULL);
}
