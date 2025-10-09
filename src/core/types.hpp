#pragma once

#include <cstdint>

namespace chessbot {

constexpr std::uint8_t kBoardDimension = 8;
constexpr std::uint8_t kBoardSquareCount = kBoardDimension * kBoardDimension;

enum class Color : std::uint8_t {
  White = 0,
  Black = 1,
  None  = 2
};

constexpr Color opposite(Color color) {
  return color == Color::White ? Color::Black
       : color == Color::Black ? Color::White
       : Color::None;
}

enum class PieceType : std::uint8_t {
  Pawn = 0,
  Knight,
  Bishop,
  Rook,
  Queen,
  King,
  None
};

enum class Square : std::uint8_t {
  A1 = 0, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  None = 64
};

inline constexpr bool is_valid(Square square) {
  return square != Square::None;
}

inline constexpr std::uint8_t square_index(Square square) {
  return static_cast<std::uint8_t>(square);
}

inline constexpr std::uint8_t file_of(Square square) {
  return is_valid(square) ? static_cast<std::uint8_t>(square_index(square) % kBoardDimension) : kBoardDimension;
}

inline constexpr std::uint8_t rank_of(Square square) {
  return is_valid(square) ? static_cast<std::uint8_t>(square_index(square) / kBoardDimension) : kBoardDimension;
}

inline constexpr Square make_square(std::uint8_t file, std::uint8_t rank) {
  return (file < kBoardDimension && rank < kBoardDimension)
             ? static_cast<Square>(rank * kBoardDimension + file)
             : Square::None;
}

enum class Piece : std::uint8_t {
  None = 0,
  WhitePawn,
  WhiteKnight,
  WhiteBishop,
  WhiteRook,
  WhiteQueen,
  WhiteKing,
  BlackPawn,
  BlackKnight,
  BlackBishop,
  BlackRook,
  BlackQueen,
  BlackKing
};

inline constexpr bool is_valid(PieceType type) {
  return type != PieceType::None;
}

inline constexpr bool is_valid(Piece piece) {
  return piece != Piece::None;
}

inline constexpr Piece make_piece(Color color, PieceType type) {
  if (color == Color::None || type == PieceType::None) {
    return Piece::None;
  }

  const std::uint8_t color_offset = (color == Color::Black) ? 6 : 0;
  return static_cast<Piece>(1 + color_offset + static_cast<std::uint8_t>(type));
}

inline constexpr Color color_of(Piece piece) {
  if (piece == Piece::None) {
    return Color::None;
  }
  return static_cast<std::uint8_t>(piece) <= 6 ? Color::White : Color::Black;
}

inline constexpr PieceType type_of(Piece piece) {
  if (piece == Piece::None) {
    return PieceType::None;
  }
  const std::uint8_t index = static_cast<std::uint8_t>(piece) - 1;
  return static_cast<PieceType>(index % 6);
}

inline constexpr bool same_color(Piece lhs, Piece rhs) {
  const Color lhs_color = color_of(lhs);
  const Color rhs_color = color_of(rhs);
  return lhs_color != Color::None && lhs_color == rhs_color;
}

inline constexpr bool opposite_color(Piece lhs, Piece rhs) {
  const Color lhs_color = color_of(lhs);
  const Color rhs_color = color_of(rhs);
  return lhs_color != Color::None && rhs_color != Color::None && lhs_color != rhs_color;
}

}  // namespace chessbot
