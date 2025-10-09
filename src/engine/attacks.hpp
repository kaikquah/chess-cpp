#pragma once

#include "core/bitboard.hpp"

namespace chessbot {

Bitboard knight_attacks(Square square);
Bitboard king_attacks(Square square);
Bitboard pawn_attacks(Color color, Square square);
Bitboard pawn_single_push_targets(Color color, Square square, Bitboard occupancy);
Bitboard pawn_double_push_targets(Color color, Square square, Bitboard occupancy);
Bitboard pawn_push_targets(Color color, Square square, Bitboard occupancy);
Bitboard bishop_attacks(Square square, Bitboard occupancy);
Bitboard rook_attacks(Square square, Bitboard occupancy);
Bitboard queen_attacks(Square square, Bitboard occupancy);

}  // namespace chessbot

