#include "engine/evaluation.hpp"

#include <array>
#include <algorithm>
#include <atomic>

#include "core/bitboard.hpp"
#include "engine/board.hpp"

namespace chessbot {

namespace {

using Table = std::array<int, kBoardSquareCount>;

constexpr int kDefaultTempoBonus = 10;
constexpr int kMaxGamePhase = 24;

constexpr std::array<int, 6> kPieceValues = {100, 320, 330, 500, 900, 0};
constexpr std::array<int, 6> kGamePhaseIncrements = {0, 1, 1, 2, 4, 0};

// clang-format off
constexpr Table kMgPawnTable = {
    0,   0,   0,   0,   0,   0,   0,   0,
   98, 134,  61,  95,  68, 126,  34, -11,
   -6,   7,  26,  31,  65,  56,  25, -20,
  -14,  13,   6,  21,  23,  12,  17, -23,
  -27,  -2,  -5,  12,  17,   6,  10, -25,
  -26,  -4,  -4, -10,   3,   3,  33, -12,
  -35,  -1, -20, -23, -15,  24,  38, -22,
    0,   0,   0,   0,   0,   0,   0,   0,
};

constexpr Table kMgKnightTable = {
  -167,  -89,  -34,  -49,   61,  -97,  -15, -107,
   -73,  -41,   72,   36,   23,   62,    7,  -17,
   -47,   60,   37,   65,   84,  129,   73,   44,
    -9,   17,   19,   53,   37,   69,   18,   22,
   -13,    4,   16,   13,   28,   19,   21,   -8,
   -23,   -9,   12,   10,   19,   17,   25,  -16,
   -29,  -53,  -12,   -3,   -1,   18,  -14,  -19,
  -105,  -21,  -58,  -33,  -17,  -28,  -19,  -23,
};

constexpr Table kMgBishopTable = {
   -29,    4,  -82,  -37,  -25,  -42,    7,   -8,
   -26,   16,  -18,  -13,   30,   59,   18,  -47,
   -16,   37,   43,   40,   35,   50,   37,   -2,
    -4,    5,   19,   50,   37,   37,    7,   -2,
    -6,   13,   13,   26,   34,   12,   10,    4,
     0,   15,   15,   15,   14,   27,   18,   10,
     4,   15,   16,    0,    7,   21,   33,    1,
   -33,   -3,  -14,  -21,  -13,  -12,  -39,  -21,
};

constexpr Table kMgRookTable = {
    32,   42,   32,   51,   63,    9,   31,   43,
    27,   32,   58,   62,   80,   67,   26,   44,
    -5,   19,   26,   36,   17,   45,   61,   16,
   -24,  -11,    7,   26,   24,   35,   -8,  -20,
   -36,  -26,  -12,   -1,    9,   -7,    6,  -23,
   -45,  -25,  -16,  -17,    3,    0,   -5,  -33,
   -44,  -16,  -20,   -9,   -1,   11,   -6,  -71,
   -19,  -13,    1,   17,   16,    7,  -37,  -26,
};

constexpr Table kMgQueenTable = {
   -28,    0,   29,   12,   59,   44,   43,   45,
   -24,  -39,   -5,    1,  -16,   57,   28,   54,
   -13,  -17,    7,    8,   29,   56,   47,   57,
   -27,  -27,  -16,  -16,   -1,   17,   -2,    1,
    -9,  -26,   -9,  -10,   -2,   -4,    3,   -3,
   -14,    2,  -11,   -2,   -5,    2,   14,    5,
   -35,   -8,   11,    2,    8,   15,   -3,    1,
    -1,  -18,   -9,   10,  -15,  -25,  -31,  -50,
};

constexpr Table kMgKingTable = {
   -65,   23,   16,  -15,  -56,  -34,    2,   13,
    29,   -1,  -20,   -7,   -8,   -4,  -38,  -29,
    -9,   24,    2,  -16,  -20,    6,   22,  -22,
   -17,  -20,  -12,  -27,  -30,  -25,  -14,  -36,
   -49,   -1,  -27,  -39,  -46,  -44,  -33,  -51,
   -14,  -14,  -22,  -46,  -44,  -30,  -15,  -27,
     1,    7,   -8,  -64,  -43,  -16,    9,    8,
   -15,   36,   12,  -54,    8,  -28,   24,   14,
};

constexpr Table kEgPawnTable = {
     0,    0,    0,    0,    0,    0,    0,    0,
   178,  173,  158,  134,  147,  132,  165,  187,
    94,  100,   85,   67,   56,   53,   82,   84,
    32,   24,   13,    5,   -2,    4,   17,   17,
    13,    9,   -3,   -7,   -7,   -8,    3,   -1,
     4,    7,   -6,    1,    0,   -5,   -1,   -8,
    13,    8,    8,   10,   13,    0,    2,   -7,
     0,    0,    0,    0,    0,    0,    0,    0,
};

constexpr Table kEgKnightTable = {
   -58,  -38,  -13,  -28,  -31,  -27,  -63,  -99,
   -25,   -8,  -25,   -2,   -9,  -25,  -24,  -52,
   -24,  -20,   10,    9,   -1,   -9,  -19,  -41,
   -17,    3,   22,   22,   22,   11,    8,  -18,
   -18,   -6,   16,   25,   16,   17,    4,  -18,
   -23,   -3,   -1,   15,   10,   -3,  -20,  -22,
   -42,  -20,  -10,   -5,   -2,  -20,  -23,  -44,
   -29,  -51,  -23,  -15,  -22,  -18,  -50,  -64,
};

constexpr Table kEgBishopTable = {
   -14,  -21,  -11,   -8,   -7,   -9,  -17,  -24,
    -8,   -4,    7,  -12,   -3,  -13,   -4,  -14,
     2,   -8,    0,   -1,   -2,    6,    0,    4,
    -3,    9,   12,    9,   14,   10,    3,    2,
    -6,    3,   13,   19,    7,   10,   -3,   -9,
   -12,   -3,    8,   10,   13,    3,   -7,  -15,
   -14,  -18,   -7,   -1,    4,   -9,  -15,  -27,
   -23,   -9,  -23,   -5,   -9,  -16,   -5,  -17,
};

constexpr Table kEgRookTable = {
    13,   10,   18,   15,   12,   12,    8,    5,
    11,   13,   13,   11,   -3,    3,    8,    3,
     7,    7,    7,    5,    4,   -3,   -5,   -3,
     4,    3,   13,    1,    2,    1,   -1,    2,
     3,    5,    8,    4,   -5,   -6,   -8,  -11,
    -4,    0,   -5,   -1,   -7,  -12,   -8,  -16,
    -6,   -6,    0,    2,   -9,   -9,  -11,   -3,
    -9,    2,    3,   -1,   -5,  -13,    4,  -20,
};

constexpr Table kEgQueenTable = {
    -9,   22,   22,   27,   27,   19,   10,   20,
   -17,   20,   32,   41,   58,   25,   30,    0,
   -20,    6,    9,   49,   47,   35,   19,    9,
     3,   22,   24,   45,   57,   40,   57,   36,
   -18,   28,   19,   47,   31,   34,   39,   23,
   -16,  -27,   15,    6,    9,   17,   10,    5,
   -22,  -23,  -30,  -16,  -16,  -23,  -36,  -32,
   -33,  -28,  -22,  -43,   -5,  -32,  -20,  -41,
};

constexpr Table kEgKingTable = {
   -74,  -35,  -18,  -18,  -11,   15,    4,  -17,
   -12,   17,   14,   17,   17,   38,   23,   11,
    10,   17,   23,   15,   20,   45,   44,   13,
    -8,   22,   24,   27,   26,   33,   26,    3,
   -18,   -4,   21,   24,   27,   23,    9,  -11,
   -19,   -3,   11,   21,   23,   16,    7,   -9,
   -27,  -11,    4,   13,   14,    4,   -5,  -17,
   -53,  -34,  -21,  -11,  -28,  -14,  -24,  -43,
};
// clang-format on

constexpr Square flip_square(Square square) {
  return static_cast<Square>(square_index(square) ^ 56);
}

struct PrecomputedTables {
  std::array<std::array<std::array<int, kBoardSquareCount>, 6>, 2> mg{};
  std::array<std::array<std::array<int, kBoardSquareCount>, 6>, 2> eg{};

  constexpr PrecomputedTables() : mg{}, eg{} {
    for (int sq = 0; sq < kBoardSquareCount; ++sq) {
      const Square square = static_cast<Square>(sq);
      const Square flipped = flip_square(square);

      // White tables mirror the reference tables vertically; black uses direct indices.
      const auto idx = square_index(square);
      const auto flipped_idx = square_index(flipped);

      mg[0][static_cast<int>(PieceType::Pawn)][sq] = kMgPawnTable[idx];
      mg[0][static_cast<int>(PieceType::Knight)][sq] = kMgKnightTable[idx];
      mg[0][static_cast<int>(PieceType::Bishop)][sq] = kMgBishopTable[idx];
      mg[0][static_cast<int>(PieceType::Rook)][sq] = kMgRookTable[idx];
      mg[0][static_cast<int>(PieceType::Queen)][sq] = kMgQueenTable[idx];
      mg[0][static_cast<int>(PieceType::King)][sq] = kMgKingTable[idx];

      mg[1][static_cast<int>(PieceType::Pawn)][sq] = kMgPawnTable[flipped_idx];
      mg[1][static_cast<int>(PieceType::Knight)][sq] = kMgKnightTable[flipped_idx];
      mg[1][static_cast<int>(PieceType::Bishop)][sq] = kMgBishopTable[flipped_idx];
      mg[1][static_cast<int>(PieceType::Rook)][sq] = kMgRookTable[flipped_idx];
      mg[1][static_cast<int>(PieceType::Queen)][sq] = kMgQueenTable[flipped_idx];
      mg[1][static_cast<int>(PieceType::King)][sq] = kMgKingTable[flipped_idx];

      eg[0][static_cast<int>(PieceType::Pawn)][sq] = kEgPawnTable[idx];
      eg[0][static_cast<int>(PieceType::Knight)][sq] = kEgKnightTable[idx];
      eg[0][static_cast<int>(PieceType::Bishop)][sq] = kEgBishopTable[idx];
      eg[0][static_cast<int>(PieceType::Rook)][sq] = kEgRookTable[idx];
      eg[0][static_cast<int>(PieceType::Queen)][sq] = kEgQueenTable[idx];
      eg[0][static_cast<int>(PieceType::King)][sq] = kEgKingTable[idx];

      eg[1][static_cast<int>(PieceType::Pawn)][sq] = kEgPawnTable[flipped_idx];
      eg[1][static_cast<int>(PieceType::Knight)][sq] = kEgKnightTable[flipped_idx];
      eg[1][static_cast<int>(PieceType::Bishop)][sq] = kEgBishopTable[flipped_idx];
      eg[1][static_cast<int>(PieceType::Rook)][sq] = kEgRookTable[flipped_idx];
      eg[1][static_cast<int>(PieceType::Queen)][sq] = kEgQueenTable[flipped_idx];
      eg[1][static_cast<int>(PieceType::King)][sq] = kEgKingTable[flipped_idx];
    }
  }
};

constexpr PrecomputedTables precomputed_tables;
std::atomic<int> g_tempo_bonus{kDefaultTempoBonus};
std::atomic<bool> g_logging_enabled{false};
std::atomic<EvaluationLogSink*> g_log_sink{nullptr};

}  // namespace

EvaluationConfig evaluation_config() {
  EvaluationConfig config;
  config.tempo_bonus = g_tempo_bonus.load(std::memory_order_relaxed);
  config.enable_logging = g_logging_enabled.load(std::memory_order_relaxed);
  return config;
}

void set_evaluation_config(const EvaluationConfig& config) {
  g_tempo_bonus.store(config.tempo_bonus, std::memory_order_relaxed);
  g_logging_enabled.store(config.enable_logging, std::memory_order_relaxed);
}

void set_evaluation_log_sink(EvaluationLogSink* sink) {
  g_log_sink.store(sink, std::memory_order_release);
}

EvaluationResult evaluate(const Board& board) {
  int material[2] = {0, 0};
  int mg_psqt[2] = {0, 0};
  int eg_psqt[2] = {0, 0};
  int game_phase = 0;

  for (int piece = static_cast<int>(PieceType::Pawn);
       piece <= static_cast<int>(PieceType::King); ++piece) {
    const PieceType type = static_cast<PieceType>(piece);
    if (type == PieceType::None) {
      continue;
    }

    Bitboard white_bb = board.pieces(Color::White, type);
    while (white_bb) {
      const Square sq = pop_lsb(white_bb);
      const int idx = square_index(sq);
      material[0] += kPieceValues[piece];
      mg_psqt[0] += precomputed_tables.mg[0][piece][idx];
      eg_psqt[0] += precomputed_tables.eg[0][piece][idx];
      game_phase += kGamePhaseIncrements[piece];
    }

    Bitboard black_bb = board.pieces(Color::Black, type);
    while (black_bb) {
      const Square sq = pop_lsb(black_bb);
      const int idx = square_index(sq);
      material[1] += kPieceValues[piece];
      mg_psqt[1] += precomputed_tables.mg[1][piece][idx];
      eg_psqt[1] += precomputed_tables.eg[1][piece][idx];
      game_phase += kGamePhaseIncrements[piece];
    }
  }

  const int material_balance = material[0] - material[1];
  const int mg_balance = mg_psqt[0] - mg_psqt[1];
  const int eg_balance = eg_psqt[0] - eg_psqt[1];

  const int bounded_phase = std::min(game_phase, kMaxGamePhase);
  const int mg_weight = bounded_phase;
  const int eg_weight = kMaxGamePhase - bounded_phase;

  const int tapered_psqt =
      (mg_balance * mg_weight + eg_balance * eg_weight) / kMaxGamePhase;
  const int absolute_score = material_balance + tapered_psqt;
  EvaluationResult result{};
  const int side_factor = (board.side_to_move() == Color::White) ? 1 : -1;
  const int relative_score = absolute_score * side_factor;
  const int tempo_bonus = g_tempo_bonus.load(std::memory_order_relaxed);
  result.tempo_cp = tempo_bonus * side_factor;
  result.score_cp = relative_score + result.tempo_cp;
  result.white_minus_black_cp = absolute_score;
  result.material_balance_cp = material_balance;
  result.psqt_midgame_cp = mg_balance;
  result.psqt_endgame_cp = eg_balance;
  result.game_phase = bounded_phase;
  if (g_logging_enabled.load(std::memory_order_relaxed)) {
    if (EvaluationLogSink* sink = g_log_sink.load(std::memory_order_acquire)) {
      sink->on_evaluation_log(board, result);
    }
  }
  return result;
}

}  // namespace chessbot
