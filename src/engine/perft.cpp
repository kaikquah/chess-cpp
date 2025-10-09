#include "engine/perft.hpp"

#include "engine/move.hpp"

namespace chessbot {

std::uint64_t perft(Board& board, int depth) {
  if (depth <= 0) {
    return 1;
  }

  MoveList moves;
  moves.reserve(64);
  board.generate_legal_moves(moves, MoveGenerationType::All);

  if (depth == 1) {
    return static_cast<std::uint64_t>(moves.size());
  }

  std::uint64_t nodes = 0;
  for (const Move& move : moves) {
    MoveState state;
    if (board.make_move(move, state)) {
      nodes += perft(board, depth - 1);
      board.unmake_move(move, state);
    }
  }
  return nodes;
}

}  // namespace chessbot

