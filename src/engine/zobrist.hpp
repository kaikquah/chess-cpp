#pragma once

#include <array>
#include <cstdint>

#include "core/types.hpp"

namespace chessbot {

class Zobrist {
public:
  Zobrist();

  [[nodiscard]] std::uint64_t piece_square(Piece piece, Square square) const;
  [[nodiscard]] std::uint64_t castling(int rights_index) const;
  [[nodiscard]] std::uint64_t en_passant(std::uint8_t file) const;
  [[nodiscard]] std::uint64_t side_to_move() const;

private:
  std::array<std::array<std::uint64_t, static_cast<std::size_t>(Square::None)>, 13> piece_keys_{};
  std::array<std::uint64_t, 16> castling_keys_{};
  std::array<std::uint64_t, kBoardDimension> en_passant_keys_{};
  std::uint64_t side_to_move_key_{};
};

const Zobrist& zobrist();

}  // namespace chessbot
