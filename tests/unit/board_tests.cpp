#include "test_framework.hpp"

#include "engine/board.hpp"

#include <array>

using namespace chessbot;

namespace {

struct FenRoundTripCase {
  std::string fen;
  Color expected_to_move;
  std::uint8_t castling;
};

const std::array<FenRoundTripCase, 3> kFenCases{{
    {"8/8/8/8/8/8/8/8 w - - 0 1", Color::White, 0},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", Color::White, 0xF},
    {"r3k2r/pppp1ppp/2n2n2/1B2p3/3P4/2N2N2/PPPBPPPP/R3K2R b KQkq - 5 12", Color::Black, 0xF},
}};

}  // namespace

CHESSBOT_TEST_CASE(empty_board_round_trip) {
  Board board;
  board.set_from_fen(kFenCases[0].fen);
  CHESSBOT_CHECK(board.to_fen() == "8/8/8/8/8/8/8/8 w - - 0 1");
  CHESSBOT_CHECK(board.side_to_move() == Color::White);
  CHESSBOT_CHECK(board.castling_rights() == 0);
  CHESSBOT_CHECK(board.halfmove_clock() == 0);
  CHESSBOT_CHECK(board.occupancy_all() == 0);
}

CHESSBOT_TEST_CASE(start_position_round_trip) {
  Board board;
  board.set_start_position();
  CHESSBOT_CHECK(board.to_fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  for (Square sq = Square::A2; sq <= Square::H2; sq = static_cast<Square>(square_index(sq) + 1)) {
    CHESSBOT_CHECK(board.piece_at(sq) == Piece::WhitePawn);
  }
  CHESSBOT_CHECK(board.side_to_move() == Color::White);
  CHESSBOT_CHECK(board.pieces(Color::White, PieceType::Knight) == (bit(Square::B1) | bit(Square::G1)));
  CHESSBOT_CHECK(popcount(board.occupancy(Color::White)) == 16);
  CHESSBOT_CHECK(popcount(board.occupancy(Color::Black)) == 16);
}

CHESSBOT_TEST_CASE(fen_parsing_various_positions) {
  for (const auto& test_case : kFenCases) {
    Board board;
    board.set_from_fen(test_case.fen);
    CHESSBOT_CHECK(board.side_to_move() == test_case.expected_to_move);
    CHESSBOT_CHECK(board.castling_rights() == test_case.castling);
    CHESSBOT_CHECK(board.to_fen().substr(0, test_case.fen.find(' ')) == test_case.fen.substr(0, test_case.fen.find(' ')));
    CHESSBOT_CHECK(board.hash() != 0ULL || board.occupancy_all() == 0);
  }
}

CHESSBOT_TEST_CASE(en_passant_file_is_respected) {
  Board board;
  board.set_from_fen("8/8/8/8/8/8/8/8 w - a6 0 1");
  CHESSBOT_CHECK(board.en_passant_file() == 0);
  CHESSBOT_CHECK(board.to_fen().find(" a6 ") != std::string::npos);

  board.set_from_fen("8/8/8/8/8/8/8/8 b - h3 0 1");
  CHESSBOT_CHECK(board.en_passant_file() == 7);
  CHESSBOT_CHECK(board.to_fen().find(" h3 ") != std::string::npos);
}

CHESSBOT_TEST_CASE(hash_changes_when_state_changes) {
  Board board;
  board.set_start_position();
  const auto start_hash = board.hash();

  board.set_side_to_move(Color::Black);
  CHESSBOT_CHECK(board.hash() != start_hash);

  board.set_castling_rights(0);
  const auto no_castle_hash = board.hash();
  CHESSBOT_CHECK(no_castle_hash != start_hash);

  board.set_en_passant_file(4);
  CHESSBOT_CHECK(board.hash() != no_castle_hash);

  board.set_piece(Square::E4, Piece::WhitePawn);
  CHESSBOT_CHECK(board.hash() != 0ULL);
}

CHESSBOT_TEST_CASE(invalid_fen_inputs_throw) {
  Board board;
  auto expect_invalid = [&](std::string_view fen) {
    bool threw = false;
    try {
      board.set_from_fen(std::string(fen));
    } catch (const std::invalid_argument&) {
      threw = true;
    }
    CHESSBOT_CHECK(threw);
  };

  expect_invalid("invalid");
  expect_invalid("8/8/8/8/8/8/8/9 w - - 0 1");  // rank digit overruns board
  expect_invalid("8/8/8/8/8/8/8/8 w - e3 0 1");  // en passant rank inconsistent with side to move
  expect_invalid("8/8/8/8/8/8/8/8 w - - 65536 1");  // halfmove clock overflow
  expect_invalid("8/8/8/8/8/8/8/8 w - - 0 0");      // fullmove number invalid
}
