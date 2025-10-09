#include "engine/attacks.hpp"

#include <array>
#include <utility>

namespace chessbot {

namespace {

constexpr bool on_board(int file, int rank) {
  return file >= 0 && file < static_cast<int>(kBoardDimension) && rank >= 0 && rank < static_cast<int>(kBoardDimension);
}

constexpr std::array<std::pair<int, int>, 8> kKnightOffsets{{
    {1, 2},  {2, 1},  {-1, 2}, {-2, 1},
    {1, -2}, {2, -1}, {-1, -2}, {-2, -1}}};

constexpr std::array<std::pair<int, int>, 8> kKingOffsets{{
    {1, 0},  {1, 1},  {0, 1},  {-1, 1},
    {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};

constexpr Bitboard compute_attacks(int square_index, const std::array<std::pair<int, int>, 8>& offsets) {
  const int file = square_index % kBoardDimension;
  const int rank = square_index / kBoardDimension;
  Bitboard mask = 0ULL;
  for (const auto& [df, dr] : offsets) {
    const int nf = file + df;
    const int nr = rank + dr;
    if (on_board(nf, nr)) {
      mask |= bit(static_cast<std::uint8_t>(nr * kBoardDimension + nf));
    }
  }
  return mask;
}

constexpr std::array<Bitboard, kBoardSquareCount> init_knight_attacks() {
  std::array<Bitboard, kBoardSquareCount> attacks{};
  for (int sq = 0; sq < kBoardSquareCount; ++sq) {
    attacks[sq] = compute_attacks(sq, kKnightOffsets);
  }
  return attacks;
}

constexpr std::array<Bitboard, kBoardSquareCount> init_king_attacks() {
  std::array<Bitboard, kBoardSquareCount> attacks{};
  for (int sq = 0; sq < kBoardSquareCount; ++sq) {
    attacks[sq] = compute_attacks(sq, kKingOffsets);
  }
  return attacks;
}

constexpr std::array<Bitboard, kBoardSquareCount> init_white_pawn_attacks() {
  std::array<Bitboard, kBoardSquareCount> attacks{};
  for (int sq = 0; sq < kBoardSquareCount; ++sq) {
    Bitboard from = bit(static_cast<std::uint8_t>(sq));
    attacks[sq] = north_east(from) | north_west(from);
  }
  return attacks;
}

constexpr std::array<Bitboard, kBoardSquareCount> init_black_pawn_attacks() {
  std::array<Bitboard, kBoardSquareCount> attacks{};
  for (int sq = 0; sq < kBoardSquareCount; ++sq) {
    Bitboard from = bit(static_cast<std::uint8_t>(sq));
    attacks[sq] = south_east(from) | south_west(from);
  }
  return attacks;
}

constexpr auto kKnightAttacks = init_knight_attacks();
constexpr auto kKingAttacks = init_king_attacks();
constexpr auto kWhitePawnAttacks = init_white_pawn_attacks();
constexpr auto kBlackPawnAttacks = init_black_pawn_attacks();

}  // namespace

Bitboard knight_attacks(Square square) {
  if (!is_valid(square)) {
    return 0ULL;
  }
  return kKnightAttacks[square_index(square)];
}

Bitboard king_attacks(Square square) {
  if (!is_valid(square)) {
    return 0ULL;
  }
  return kKingAttacks[square_index(square)];
}

Bitboard pawn_attacks(Color color, Square square) {
  if (!is_valid(square)) {
    return 0ULL;
  }
  return color == Color::White ? kWhitePawnAttacks[square_index(square)]
                               : kBlackPawnAttacks[square_index(square)];
}

Bitboard pawn_single_push_targets(Color color, Square square, Bitboard occupancy) {
  if (!is_valid(square)) {
    return 0ULL;
  }

  const Bitboard from = bit(square);
  Bitboard single = 0ULL;
  if (color == Color::White) {
    single = north_one(from);
  } else {
    single = south_one(from);
  }
  if ((single & occupancy) != 0ULL) {
    return 0ULL;
  }
  return single;
}

Bitboard pawn_double_push_targets(Color color, Square square, Bitboard occupancy) {
  if (!is_valid(square)) {
    return 0ULL;
  }

  const int rank = rank_of(square);
  const bool on_start_rank = (color == Color::White && rank == 1) || (color == Color::Black && rank == 6);
  if (!on_start_rank) {
    return 0ULL;
  }

  Bitboard single = pawn_single_push_targets(color, square, occupancy);
  if (single == 0ULL) {
    return 0ULL;
  }

  Bitboard double_push = 0ULL;
  if (color == Color::White) {
    double_push = north_one(single);
  } else {
    double_push = south_one(single);
  }
  if ((double_push & occupancy) != 0ULL) {
    return 0ULL;
  }
  return double_push;
}

Bitboard pawn_push_targets(Color color, Square square, Bitboard occupancy) {
  return pawn_single_push_targets(color, square, occupancy) |
         pawn_double_push_targets(color, square, occupancy);
}

Bitboard bishop_attacks(Square square, Bitboard occupancy) {
  if (!is_valid(square)) {
    return 0ULL;
  }

  Bitboard attacks = 0ULL;
  const int file = file_of(square);
  const int rank = rank_of(square);

  for (int nf = file + 1, nr = rank + 1; on_board(nf, nr); ++nf, ++nr) {
    const Square target = make_square(nf, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nf = file - 1, nr = rank + 1; on_board(nf, nr); --nf, ++nr) {
    const Square target = make_square(nf, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nf = file + 1, nr = rank - 1; on_board(nf, nr); ++nf, --nr) {
    const Square target = make_square(nf, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nf = file - 1, nr = rank - 1; on_board(nf, nr); --nf, --nr) {
    const Square target = make_square(nf, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  return attacks;
}

Bitboard rook_attacks(Square square, Bitboard occupancy) {
  if (!is_valid(square)) {
    return 0ULL;
  }

  Bitboard attacks = 0ULL;
  const int file = file_of(square);
  const int rank = rank_of(square);

  for (int nr = rank + 1; nr < static_cast<int>(kBoardDimension); ++nr) {
    const Square target = make_square(file, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nr = rank - 1; nr >= 0; --nr) {
    const Square target = make_square(file, nr);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nf = file + 1; nf < static_cast<int>(kBoardDimension); ++nf) {
    const Square target = make_square(nf, rank);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  for (int nf = file - 1; nf >= 0; --nf) {
    const Square target = make_square(nf, rank);
    attacks |= bit(target);
    if (get_bit(occupancy, target)) {
      break;
    }
  }

  return attacks;
}

Bitboard queen_attacks(Square square, Bitboard occupancy) {
  return bishop_attacks(square, occupancy) | rook_attacks(square, occupancy);
}

}  // namespace chessbot
