#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "core/bitboard.hpp"
#include "core/types.hpp"
#include "engine/move.hpp"

namespace chessbot {

struct GameState {
  std::uint8_t castling_rights = 0;
  std::uint8_t en_passant_file = 8;
  std::uint16_t halfmove_clock = 0;
  std::uint16_t fullmove_number = 1;
  Square king_square_white = Square::None;
  Square king_square_black = Square::None;
  std::uint64_t hash = 0ULL;
};

struct MoveState {
  GameState game_state{};
  Piece captured_piece = Piece::None;
  Square captured_square = Square::None;
  Color side_to_move = Color::White;
};

class Board {
public:
  Board();

  void clear();
  void set_start_position();
  void set_from_fen(const std::string& fen);

  [[nodiscard]] std::string to_fen() const;

  [[nodiscard]] Color side_to_move() const { return side_to_move_; }
  [[nodiscard]] Bitboard occupancy(Color color) const;
  [[nodiscard]] Bitboard occupancy_all() const { return occupancies_[2]; }
  [[nodiscard]] Piece piece_at(Square square) const;
  [[nodiscard]] Bitboard pieces(Color color, PieceType type) const;
  [[nodiscard]] std::uint8_t castling_rights() const { return state_.castling_rights; }
  [[nodiscard]] std::uint8_t en_passant_file() const { return state_.en_passant_file; }
  [[nodiscard]] std::uint16_t halfmove_clock() const { return state_.halfmove_clock; }
  [[nodiscard]] std::uint16_t fullmove_number() const { return state_.fullmove_number; }
  [[nodiscard]] std::uint64_t hash() const { return state_.hash; }
  [[nodiscard]] Square king_square(Color color) const {
    return color == Color::White ? state_.king_square_white : state_.king_square_black;
  }

  void set_piece(Square square, Piece piece);
  void remove_piece(Square square);

  void set_side_to_move(Color color);
  void set_castling_rights(std::uint8_t rights);
  void set_en_passant_file(std::uint8_t file);  // 0..7 or 8 meaning none
  void set_halfmove_clock(std::uint16_t clock);

  [[nodiscard]] const GameState& state() const { return state_; }
  void set_state(const GameState& state);

  bool make_move(const Move& move, MoveState& move_state);
  void unmake_move(const Move& move, const MoveState& move_state);
  [[nodiscard]] bool is_square_attacked(Square square, Color attacker) const;
  [[nodiscard]] bool has_insufficient_material() const;

  void generate_pseudo_legal_moves(MoveList& moves, MoveGenerationType type) const;
  void generate_legal_moves(MoveList& moves, MoveGenerationType type);

private:
  void update_hash();

  void add_piece_internal(Square square, Piece piece, bool update_hash = true);
  void remove_piece_internal(Square square, Piece piece, bool update_hash = true);
  void move_piece_internal(Square from, Square to, Piece piece, bool update_hash = true);

  Color side_to_move_ = Color::White;
  std::array<Piece, kBoardSquareCount> squares_{};
  std::array<Bitboard, 2> color_bitboards_{};
  std::array<std::array<Bitboard, 6>, 2> piece_bitboards_{};
  std::array<Bitboard, 3> occupancies_{};  // white, black, all
  GameState state_{};
};

}  // namespace chessbot
