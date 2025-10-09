#pragma once

#include <cstdint>

#include "engine/board.hpp"

namespace chessbot {

std::uint64_t perft(Board& board, int depth);

}  // namespace chessbot

