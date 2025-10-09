#include "test_framework.hpp"

#include "engine/board.hpp"

#include <string>

using namespace chessbot;

namespace {

const Move* find_move(const MoveList& moves, const std::string& uci) {
  for (const auto& move : moves) {
    if (move.to_uci() == uci) {
      return &move;
    }
  }
  return nullptr;
}

}  // namespace

CHESSBOT_TEST_CASE(start_position_move_counts) {
  Board board;
  board.set_start_position();

  MoveList pseudo_moves;
  board.generate_pseudo_legal_moves(pseudo_moves, MoveGenerationType::All);
  CHESSBOT_CHECK(pseudo_moves.size() == 20);

  MoveList capture_moves;
  board.generate_pseudo_legal_moves(capture_moves, MoveGenerationType::Captures);
  CHESSBOT_CHECK(capture_moves.size() == 0);

  MoveList legal_moves;
  board.generate_legal_moves(legal_moves, MoveGenerationType::All);
  CHESSBOT_CHECK(legal_moves.size() == 20);
}

CHESSBOT_TEST_CASE(en_passant_is_generated_and_applied) {
  Board board;
  board.set_from_fen("8/8/8/3pP3/8/8/8/8 w - d6 0 1");

  MoveList moves;
  board.generate_pseudo_legal_moves(moves, MoveGenerationType::Captures);
  const Move* ep_move = find_move(moves, "e5d6");
  CHESSBOT_REQUIRE(ep_move != nullptr);

  MoveState state;
  CHESSBOT_REQUIRE(board.make_move(*ep_move, state));
  CHESSBOT_CHECK(board.piece_at(Square::D6) == Piece::WhitePawn);
  CHESSBOT_CHECK(board.piece_at(Square::D5) == Piece::None);
  CHESSBOT_CHECK(board.halfmove_clock() == 0);

  board.unmake_move(*ep_move, state);
  CHESSBOT_CHECK(board.piece_at(Square::E5) == Piece::WhitePawn);
  CHESSBOT_CHECK(board.piece_at(Square::D5) == Piece::BlackPawn);
  CHESSBOT_CHECK(board.to_fen() == "8/8/8/3pP3/8/8/8/8 w - d6 0 1");
}

CHESSBOT_TEST_CASE(castling_moves_require_clear_path_and_safety) {
  Board open_castle;
  open_castle.set_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  MoveList pseudo;
  open_castle.generate_pseudo_legal_moves(pseudo, MoveGenerationType::All);
  CHESSBOT_CHECK(find_move(pseudo, "e1g1") != nullptr);
  CHESSBOT_CHECK(find_move(pseudo, "e1c1") != nullptr);

  MoveList legal;
  open_castle.generate_legal_moves(legal, MoveGenerationType::All);
  CHESSBOT_CHECK(find_move(legal, "e1g1") != nullptr);
  CHESSBOT_CHECK(find_move(legal, "e1c1") != nullptr);

  Board attacked_castle;
  attacked_castle.set_from_fen("r3kr1r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  MoveList attacked_legal;
  attacked_castle.generate_legal_moves(attacked_legal, MoveGenerationType::All);
  CHESSBOT_CHECK(find_move(attacked_legal, "e1g1") == nullptr);
  CHESSBOT_CHECK(find_move(attacked_legal, "e1c1") != nullptr);
}

CHESSBOT_TEST_CASE(legal_moves_filter_pinned_piece) {
  Board board;
  board.set_from_fen("4r2k/8/8/8/8/8/4R3/4K3 w - - 0 1");

  MoveList pseudo;
  board.generate_pseudo_legal_moves(pseudo, MoveGenerationType::All);
  CHESSBOT_CHECK(find_move(pseudo, "e2f2") != nullptr);

  MoveList legal;
  board.generate_legal_moves(legal, MoveGenerationType::All);
  CHESSBOT_CHECK(find_move(legal, "e2f2") == nullptr);
}

CHESSBOT_TEST_CASE(make_and_unmake_restore_state) {
  Board board;
  board.set_start_position();
  const std::string initial_fen = board.to_fen();
  const auto initial_hash = board.hash();

  MoveList legal;
  board.generate_legal_moves(legal, MoveGenerationType::All);
  const Move* move = find_move(legal, "e2e4");
  CHESSBOT_REQUIRE(move != nullptr);

  MoveState state;
  CHESSBOT_REQUIRE(board.make_move(*move, state));
  CHESSBOT_CHECK(board.side_to_move() == Color::Black);
  CHESSBOT_CHECK(board.piece_at(Square::E4) == Piece::WhitePawn);
  CHESSBOT_CHECK(board.halfmove_clock() == 0);
  CHESSBOT_CHECK(board.fullmove_number() == 1);

  board.unmake_move(*move, state);
  CHESSBOT_CHECK(board.side_to_move() == Color::White);
  CHESSBOT_CHECK(board.to_fen() == initial_fen);
  CHESSBOT_CHECK(board.hash() == initial_hash);
}

CHESSBOT_TEST_CASE(invalid_en_passant_does_not_mutate_board) {
  Board board;
  board.set_from_fen("8/8/8/4P3/8/8/8/8 w - d6 0 1");
  const std::string snapshot = board.to_fen();

  Move invalid_ep{Square::E5, Square::D6, PieceType::Pawn, PieceType::Pawn, PieceType::None,
                  MoveFlag::EnPassant | MoveFlag::Capture};
  MoveState state;
  CHESSBOT_CHECK(!board.make_move(invalid_ep, state));
  CHESSBOT_CHECK(board.to_fen() == snapshot);
  CHESSBOT_CHECK(board.side_to_move() == Color::White);
}

CHESSBOT_TEST_CASE(castling_without_rook_is_rejected) {
  Board board;
  board.set_from_fen("4k3/8/8/8/8/8/8/4K3 w K - 0 1");
  const std::string snapshot = board.to_fen();

  Move castle{Square::E1, Square::G1, PieceType::King, PieceType::None, PieceType::None,
              MoveFlag::KingCastle};
  MoveState state;
  CHESSBOT_CHECK(!board.make_move(castle, state));
  CHESSBOT_CHECK(board.to_fen() == snapshot);
  CHESSBOT_CHECK(board.castling_rights() == 0x1);
}

CHESSBOT_TEST_CASE(castling_without_rights_is_rejected) {
  Board board;
  board.set_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

  Move castle{Square::E1, Square::G1, PieceType::King, PieceType::None, PieceType::None,
              MoveFlag::KingCastle};
  MoveState state;
  CHESSBOT_CHECK(!board.make_move(castle, state));
  CHESSBOT_CHECK(board.castling_rights() == 0);
}
