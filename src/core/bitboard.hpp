#pragma once

#include <cstdint>

#include "core/types.hpp"

namespace chessbot {

using Bitboard = std::uint64_t;

constexpr Bitboard kFileA = 0x0101010101010101ULL;
constexpr Bitboard kFileH = 0x8080808080808080ULL;
constexpr Bitboard kRank1 = 0x00000000000000FFULL;
constexpr Bitboard kRank8 = 0xFF00000000000000ULL;

constexpr Bitboard bit(std::uint8_t square) {
  return Bitboard{1ULL} << square;
}

constexpr Bitboard bit(Square square) {
  return is_valid(square) ? bit(square_index(square)) : Bitboard{0};
}

constexpr bool get_bit(Bitboard board, std::uint8_t square) {
  return (board >> square) & 1ULL;
}

constexpr bool get_bit(Bitboard board, Square square) {
  return is_valid(square) && get_bit(board, square_index(square));
}

constexpr Bitboard set_bit(Bitboard board, Square square) {
  return board | bit(square);
}

constexpr Bitboard clear_bit(Bitboard board, Square square) {
  return board & ~bit(square);
}

constexpr Bitboard north_one(Bitboard board) {
  return board << 8;
}

constexpr Bitboard south_one(Bitboard board) {
  return board >> 8;
}

constexpr Bitboard east_one(Bitboard board) {
  return (board << 1) & ~kFileA;
}

constexpr Bitboard west_one(Bitboard board) {
  return (board >> 1) & ~kFileH;
}

constexpr Bitboard north_east(Bitboard board) {
  return (board << 9) & ~kFileA;
}

constexpr Bitboard north_west(Bitboard board) {
  return (board << 7) & ~kFileH;
}

constexpr Bitboard south_east(Bitboard board) {
  return (board >> 7) & ~kFileA;
}

constexpr Bitboard south_west(Bitboard board) {
  return (board >> 9) & ~kFileH;
}

constexpr Bitboard file_mask(std::uint8_t file) {
  return file < kBoardDimension ? (kFileA << file) : Bitboard{0};
}

constexpr Bitboard rank_mask(std::uint8_t rank) {
  return rank < kBoardDimension ? (kRank1 << (rank * kBoardDimension)) : Bitboard{0};
}

constexpr bool multiple_bits(Bitboard board) {
  return (board & (board - 1)) != 0;
}

Square pop_lsb(Bitboard& board);
Square lsb(Bitboard board);
int popcount(Bitboard board);

}  // namespace chessbot
