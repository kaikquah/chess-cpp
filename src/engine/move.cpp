#include "engine/move.hpp"

#include <array>

namespace chessbot {

namespace {

constexpr std::array<char, 6> kPromotionLookup{'?', 'n', 'b', 'r', 'q', '?'};

char promotion_char(PieceType type) {
  if (type == PieceType::None) {
    return '\0';
  }
  const auto index = static_cast<std::uint8_t>(type);
  return index < kPromotionLookup.size() ? kPromotionLookup[index] : '?';
}

}  // namespace

std::string Move::to_uci() const {
  if (is_null()) {
    return "0000";
  }

  auto square_to_string = [](Square square) {
    std::string result(2, 'a');
    result[0] = static_cast<char>('a' + file_of(square));
    result[1] = static_cast<char>('1' + rank_of(square));
    return result;
  };

  std::string uci;
  uci.reserve(5);
  uci += square_to_string(from());
  uci += square_to_string(to());
  if (is_promotion()) {
    uci.push_back(promotion_char(promotion_piece()));
  }
  return uci;
}

}  // namespace chessbot
