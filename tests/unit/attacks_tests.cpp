#include "test_framework.hpp"

#include "engine/attacks.hpp"

using namespace chessbot;

namespace {

Bitboard make_bitboard(std::initializer_list<Square> squares) {
  Bitboard board = 0ULL;
  for (Square square : squares) {
    board |= bit(square);
  }
  return board;
}

}  // namespace

CHESSBOT_TEST_CASE(knight_and_king_attacks_match_masks) {
  const Bitboard knight_mask = knight_attacks(Square::D4);
  const Bitboard expected_knight = make_bitboard({Square::B5, Square::B3, Square::C6, Square::C2,
                                                  Square::E6, Square::E2, Square::F5, Square::F3});
  CHESSBOT_CHECK(knight_mask == expected_knight);

  const Bitboard king_mask = king_attacks(Square::A1);
  const Bitboard expected_king = make_bitboard({Square::A2, Square::B2, Square::B1});
  CHESSBOT_CHECK(king_mask == expected_king);
}

CHESSBOT_TEST_CASE(pawn_pushes_respect_occupancy) {
  const Bitboard empty = 0ULL;
  const Bitboard white_single = pawn_single_push_targets(Color::White, Square::E2, empty);
  CHESSBOT_CHECK(white_single == make_bitboard({Square::E3}));
  const Bitboard white_double = pawn_double_push_targets(Color::White, Square::E2, empty);
  CHESSBOT_CHECK(white_double == make_bitboard({Square::E4}));

  Bitboard blockers = bit(Square::E3);
  CHESSBOT_CHECK(pawn_single_push_targets(Color::White, Square::E2, blockers) == 0ULL);
  CHESSBOT_CHECK(pawn_double_push_targets(Color::White, Square::E2, blockers) == 0ULL);

  const Bitboard black_single = pawn_single_push_targets(Color::Black, Square::D7, empty);
  CHESSBOT_CHECK(black_single == make_bitboard({Square::D6}));
  const Bitboard black_double = pawn_double_push_targets(Color::Black, Square::D7, empty);
  CHESSBOT_CHECK(black_double == make_bitboard({Square::D5}));

  const Bitboard white_attacks = pawn_attacks(Color::White, Square::E4);
  CHESSBOT_CHECK(white_attacks == make_bitboard({Square::D5, Square::F5}));

  const Bitboard black_attacks = pawn_attacks(Color::Black, Square::E5);
  CHESSBOT_CHECK(black_attacks == make_bitboard({Square::D4, Square::F4}));
}

CHESSBOT_TEST_CASE(sliding_attacks_stop_at_blockers) {
  Bitboard occupancy = make_bitboard({Square::D6, Square::G4, Square::D2, Square::B4});
  const Bitboard rook_mask = rook_attacks(Square::D4, occupancy);
  const Bitboard expected_rook = make_bitboard({Square::D5, Square::D6, Square::D3, Square::D2,
                                                Square::E4, Square::F4, Square::G4,
                                                Square::C4, Square::B4});
  CHESSBOT_CHECK(rook_mask == expected_rook);

  occupancy = make_bitboard({Square::F6, Square::B6, Square::F2, Square::B2});
  const Bitboard bishop_mask = bishop_attacks(Square::D4, occupancy);
  const Bitboard expected_bishop = make_bitboard({Square::E5, Square::F6, Square::C5, Square::B6,
                                                  Square::E3, Square::F2, Square::C3, Square::B2});
  CHESSBOT_CHECK(bishop_mask == expected_bishop);

  const Bitboard queen_mask = queen_attacks(Square::D4, occupancy);
  CHESSBOT_CHECK(queen_mask == (rook_attacks(Square::D4, occupancy) | bishop_mask));
}

