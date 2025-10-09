#include "test_framework.hpp"

#include "engine/move.hpp"

using namespace chessbot;

CHESSBOT_TEST_CASE(move_encodes_core_fields) {
  Move move{Square::E2, Square::E4, PieceType::Pawn};
  CHESSBOT_CHECK(move.from() == Square::E2);
  CHESSBOT_CHECK(move.to() == Square::E4);
  CHESSBOT_CHECK(move.moving_piece() == PieceType::Pawn);
  CHESSBOT_CHECK(!move.is_capture());
  CHESSBOT_CHECK(!move.is_promotion());
  CHESSBOT_CHECK(move.raw() != 0);
}

CHESSBOT_TEST_CASE(move_handles_promotion_and_flags) {
  Move move{Square::E7, Square::E8, PieceType::Pawn, PieceType::Rook, PieceType::Queen,
            MoveFlag::Capture | MoveFlag::Promotion};
  CHESSBOT_CHECK(move.from() == Square::E7);
  CHESSBOT_CHECK(move.to() == Square::E8);
  CHESSBOT_CHECK(move.moving_piece() == PieceType::Pawn);
  CHESSBOT_CHECK(move.captured_piece() == PieceType::Rook);
  CHESSBOT_CHECK(move.promotion_piece() == PieceType::Queen);
  CHESSBOT_CHECK(move.is_capture());
  CHESSBOT_CHECK(move.is_promotion());
  const std::string uci = move.to_uci();
  CHESSBOT_CHECK(uci.size() == 5);
  CHESSBOT_CHECK(uci[0] == 'e');
  CHESSBOT_CHECK(uci[1] == '7');
  CHESSBOT_CHECK(uci[2] == 'e');
  CHESSBOT_CHECK(uci[3] == '8');
  CHESSBOT_CHECK(uci[4] == 'q');
  CHESSBOT_CHECK(uci == "e7e8q");
}

CHESSBOT_TEST_CASE(move_list_collects_moves) {
  MoveList moves;
  moves.reserve(4);
  moves.add(Move{Square::B1, Square::C3, PieceType::Knight});
  moves.add(Move{Square::G1, Square::H3, PieceType::Knight});
  CHESSBOT_CHECK(moves.size() == 2);
  CHESSBOT_CHECK(moves[0].to() == Square::C3);
  CHESSBOT_CHECK(moves[1].from() == Square::G1);
}

CHESSBOT_TEST_CASE(move_rejects_invalid_input) {
  Move invalid_from{Square::None, Square::E4, PieceType::Pawn};
  CHESSBOT_CHECK(invalid_from.is_null());

  Move invalid_promotion{Square::E7, Square::E8, PieceType::Pawn, PieceType::None, PieceType::Pawn};
  CHESSBOT_CHECK(invalid_promotion.is_null());

}
