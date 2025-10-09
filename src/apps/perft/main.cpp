#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "engine/board.hpp"
#include "engine/perft.hpp"

namespace {

void print_usage(const char* program) {
  std::cout << "Usage: " << program << " <depth> [FEN]\n";
  std::cout << "  depth : positive integer\n";
  std::cout << "  FEN   : optional position; defaults to start position\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  bool divide = false;
  int depth = -1;
  std::vector<std::string> fen_tokens;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--divide") {
      divide = true;
      continue;
    }
    if (depth < 0) {
      try {
        depth = std::stoi(arg);
      } catch (const std::exception&) {
        std::cerr << "Invalid depth argument\n";
        print_usage(argv[0]);
        return 1;
      }
      continue;
    }
    fen_tokens.push_back(arg);
  }

  if (depth == -1) {
    print_usage(argv[0]);
    return 1;
  }

  std::string fen;
  if (!fen_tokens.empty()) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < fen_tokens.size(); ++i) {
      if (i != 0) {
        oss << ' ';
      }
      oss << fen_tokens[i];
    }
    fen = oss.str();
  }

  chessbot::Board board;
  try {
    if (fen.empty()) {
      board.set_start_position();
    } else {
      board.set_from_fen(fen);
    }
  } catch (const std::exception& ex) {
    std::cerr << "Failed to parse FEN: " << ex.what() << '\n';
    return 1;
  }

  const auto start = std::chrono::steady_clock::now();
  const std::uint64_t nodes = chessbot::perft(board, depth);
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Depth: " << depth << '\n';
  if (!fen.empty()) {
    std::cout << "FEN:   " << fen << '\n';
  }
  std::cout << "Nodes: " << nodes << '\n';
  std::cout << "Time:  " << elapsed.count() << " ms\n";

  if (divide && depth > 0) {
    chessbot::MoveList moves;
    board.generate_legal_moves(moves, chessbot::MoveGenerationType::All);
    std::uint64_t total = 0;
    for (const auto& move : moves) {
      chessbot::MoveState state;
      if (board.make_move(move, state)) {
        const auto child_nodes = chessbot::perft(board, depth - 1);
        board.unmake_move(move, state);
        total += child_nodes;
        std::cout << move.to_uci() << ": " << child_nodes << '\n';
      }
    }
    std::cout << "Total: " << total << '\n';
  }

  return 0;
}
