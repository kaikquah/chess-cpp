#include "zobrist.hpp"

#include <random>

namespace chessbot {

namespace {

std::uint64_t random_u64(std::mt19937_64& rng) {
  std::uniform_int_distribution<std::uint64_t> dist;
  return dist(rng);
}

}  // namespace

Zobrist::Zobrist() {
  std::mt19937_64 rng(0xC5EED1234ABCDEFULL);

  for (auto& piece_array : piece_keys_) {
    for (auto& key : piece_array) {
      key = random_u64(rng);
    }
  }

  for (auto& key : castling_keys_) {
    key = random_u64(rng);
  }

  for (auto& key : en_passant_keys_) {
    key = random_u64(rng);
  }

  side_to_move_key_ = random_u64(rng);
}

std::uint64_t Zobrist::piece_square(Piece piece, Square square) const {
  if (!is_valid(piece) || !is_valid(square)) {
    return 0ULL;
  }
  return piece_keys_[static_cast<std::uint8_t>(piece)][square_index(square)];
}

std::uint64_t Zobrist::castling(int rights_index) const {
  return castling_keys_[rights_index & 0xF];
}

std::uint64_t Zobrist::en_passant(std::uint8_t file) const {
  return file < kBoardDimension ? en_passant_keys_[file] : 0ULL;
}

std::uint64_t Zobrist::side_to_move() const {
  return side_to_move_key_;
}

const Zobrist& zobrist() {
  static Zobrist instance;
  return instance;
}

}  // namespace chessbot
