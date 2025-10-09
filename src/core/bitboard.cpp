#include "core/bitboard.hpp"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace chessbot {

namespace {

inline int count_trailing_zeros(Bitboard board) {
  if (board == 0ULL) {
    return 64;
  }
#if defined(_MSC_VER)
  unsigned long index;
  _BitScanForward64(&index, board);
  return static_cast<int>(index);
#elif defined(__has_builtin)
#  if __has_builtin(__builtin_ctzll)
  return static_cast<int>(__builtin_ctzll(board));
#  endif
#elif defined(__GNUC__) || defined(__clang__)
  return static_cast<int>(__builtin_ctzll(board));
#endif
  int idx = 0;
  while ((board & 1ULL) == 0ULL) {
    board >>= 1;
    ++idx;
  }
  return idx;
}

inline int bit_popcount(Bitboard board) {
#if defined(_MSC_VER)
  return static_cast<int>(__popcnt64(board));
#elif defined(__has_builtin)
#  if __has_builtin(__builtin_popcountll)
  return static_cast<int>(__builtin_popcountll(board));
#  endif
#elif defined(__GNUC__) || defined(__clang__)
  return static_cast<int>(__builtin_popcountll(board));
#endif
  int count = 0;
  while (board != 0ULL) {
    board &= board - 1ULL;
    ++count;
  }
  return count;
}

}  // namespace

Square pop_lsb(Bitboard& board) {
  if (board == 0) {
    return Square::None;
  }

  const int index = count_trailing_zeros(board);
  board &= board - 1;
  return static_cast<Square>(index);
}

Square lsb(Bitboard board) {
  if (board == 0) {
    return Square::None;
  }
  return static_cast<Square>(count_trailing_zeros(board));
}

int popcount(Bitboard board) {
  return bit_popcount(board);
}

}  // namespace chessbot
