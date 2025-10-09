#include "test_framework.hpp"

#include "core/bitboard.hpp"
#include "core/types.hpp"

#include <array>

using namespace chessbot;

CHESSBOT_TEST_CASE(bit_helper_creates_expected_bitboards) {
  Bitboard board = bit(Square::A1);
  CHESSBOT_CHECK(board == 0x1ULL);
  CHESSBOT_CHECK(get_bit(board, Square::A1));
  CHESSBOT_CHECK(!get_bit(board, Square::B1));

  Bitboard board_h8 = bit(Square::H8);
  CHESSBOT_CHECK(board_h8 == (1ULL << 63));
  CHESSBOT_CHECK(get_bit(board_h8, Square::H8));
}

CHESSBOT_TEST_CASE(north_and_south_shifts_behave) {
  Bitboard start = bit(Square::E2);
  CHESSBOT_CHECK(north_one(start) == bit(Square::E3));
  CHESSBOT_CHECK(north_one(bit(Square::E7)) == bit(Square::E8));
  CHESSBOT_CHECK(north_one(bit(Square::E8)) == 0);

  Bitboard south_from_e7 = south_one(bit(Square::E7));
  CHESSBOT_CHECK(south_from_e7 == bit(Square::E6));
  CHESSBOT_CHECK(south_one(bit(Square::E1)) == 0);
}

CHESSBOT_TEST_CASE(horizontal_shifts_mask_edges) {
  Bitboard rank = bit(Square::A4) | bit(Square::H4);
  CHESSBOT_CHECK(east_one(rank) == bit(Square::B4));
  CHESSBOT_CHECK(west_one(rank) == bit(Square::G4));

  Bitboard middle = bit(Square::D5);
  CHESSBOT_CHECK(east_one(middle) == bit(Square::E5));
  CHESSBOT_CHECK(west_one(middle) == bit(Square::C5));
}

CHESSBOT_TEST_CASE(diagonal_shifts_respect_board_edges) {
  Bitboard start = bit(Square::C3);
  CHESSBOT_CHECK(north_east(start) == bit(Square::D4));
  CHESSBOT_CHECK(north_west(start) == bit(Square::B4));
  CHESSBOT_CHECK(south_east(start) == bit(Square::D2));
  CHESSBOT_CHECK(south_west(start) == bit(Square::B2));

  Bitboard corners = bit(Square::A1) | bit(Square::H8);
  CHESSBOT_CHECK(north_west(corners) == 0);
  CHESSBOT_CHECK(south_east(corners) == 0);
}

CHESSBOT_TEST_CASE(file_and_rank_masks_match_expectations) {
  CHESSBOT_CHECK(file_mask(0) == kFileA);
  CHESSBOT_CHECK(file_mask(7) == kFileH);
  CHESSBOT_CHECK(rank_mask(0) == kRank1);
  CHESSBOT_CHECK(rank_mask(7) == kRank8);

  CHESSBOT_CHECK(file_mask(8) == 0);
  CHESSBOT_CHECK(rank_mask(8) == 0);
}

CHESSBOT_TEST_CASE(pop_lsb_returns_first_square_and_clears_bit) {
  Bitboard board = bit(Square::B2) | bit(Square::E4);
  const Square first = pop_lsb(board);
  CHESSBOT_CHECK(first == Square::B2);
  CHESSBOT_CHECK(board == bit(Square::E4));

  const Square second = pop_lsb(board);
  CHESSBOT_CHECK(second == Square::E4);
  CHESSBOT_CHECK(board == 0);

  CHESSBOT_CHECK(pop_lsb(board) == Square::None);
}

CHESSBOT_TEST_CASE(lsb_and_popcount_are_consistent) {
  std::array<Square, 4> squares{Square::A1, Square::C3, Square::H8, Square::F5};
  Bitboard board = 0;
  for (Square sq : squares) {
    board = set_bit(board, sq);
  }

  CHESSBOT_CHECK(popcount(board) == static_cast<int>(squares.size()));
  CHESSBOT_CHECK(lsb(board) == Square::A1);
  CHESSBOT_CHECK(multiple_bits(board));

  Bitboard single = bit(Square::D4);
  CHESSBOT_CHECK(!multiple_bits(single));
  CHESSBOT_CHECK(lsb(single) == Square::D4);
}

CHESSBOT_TEST_CASE(square_helpers_produce_expected_indices) {
  CHESSBOT_CHECK(file_of(Square::A1) == 0);
  CHESSBOT_CHECK(rank_of(Square::A1) == 0);
  CHESSBOT_CHECK(file_of(Square::H8) == 7);
  CHESSBOT_CHECK(rank_of(Square::H8) == 7);

  CHESSBOT_CHECK(make_square(2, 5) == Square::C6);
  CHESSBOT_CHECK(make_square(8, 0) == Square::None);
}

CHESSBOT_TEST_CASE(piece_helpers_map_between_types_and_colors) {
  CHESSBOT_CHECK(make_piece(Color::White, PieceType::Knight) == Piece::WhiteKnight);
  CHESSBOT_CHECK(make_piece(Color::Black, PieceType::Queen) == Piece::BlackQueen);
  CHESSBOT_CHECK(make_piece(Color::None, PieceType::Queen) == Piece::None);

  CHESSBOT_CHECK(color_of(Piece::WhitePawn) == Color::White);
  CHESSBOT_CHECK(color_of(Piece::BlackKing) == Color::Black);
  CHESSBOT_CHECK(color_of(Piece::None) == Color::None);

  CHESSBOT_CHECK(type_of(Piece::WhiteBishop) == PieceType::Bishop);
  CHESSBOT_CHECK(type_of(Piece::BlackRook) == PieceType::Rook);
  CHESSBOT_CHECK(type_of(Piece::None) == PieceType::None);

  CHESSBOT_CHECK(same_color(Piece::WhiteBishop, Piece::WhiteQueen));
  CHESSBOT_CHECK(!same_color(Piece::WhiteBishop, Piece::BlackQueen));
  CHESSBOT_CHECK(opposite_color(Piece::WhiteKnight, Piece::BlackKnight));
  CHESSBOT_CHECK(!opposite_color(Piece::WhiteKnight, Piece::WhitePawn));
}
