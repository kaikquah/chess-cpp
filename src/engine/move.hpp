#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/types.hpp"

namespace chessbot {

enum class MoveFlag : std::uint8_t {
  None           = 0,
  Capture        = 1 << 0,
  DoublePawnPush = 1 << 1,
  EnPassant      = 1 << 2,
  KingCastle     = 1 << 3,
  QueenCastle    = 1 << 4,
  Promotion      = 1 << 5
};

constexpr MoveFlag operator|(MoveFlag lhs, MoveFlag rhs) {
  return static_cast<MoveFlag>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr MoveFlag operator&(MoveFlag lhs, MoveFlag rhs) {
  return static_cast<MoveFlag>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr MoveFlag& operator|=(MoveFlag& lhs, MoveFlag rhs) {
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool has_flag(MoveFlag flags, MoveFlag test) {
  return static_cast<std::uint8_t>(flags & test) != 0;
}

class Move {
public:
  constexpr Move() = default;

  constexpr Move(Square from, Square to, PieceType moving, PieceType captured = PieceType::None,
                 PieceType promotion = PieceType::None, MoveFlag flags = MoveFlag::None)
      : data_(encode(from, to, moving, captured, promotion, flags)) {}

  static constexpr Move from_raw(std::uint32_t raw) {
    Move move;
    move.data_ = raw;
    return move;
  }

  [[nodiscard]] constexpr Square from() const {
    return static_cast<Square>((data_ >> kFromShift) & kSquareMask);
  }

  [[nodiscard]] constexpr Square to() const {
    return static_cast<Square>((data_ >> kToShift) & kSquareMask);
  }

  [[nodiscard]] constexpr PieceType moving_piece() const {
    return decode_piece((data_ >> kMovingShift) & kPieceMask);
  }

  [[nodiscard]] constexpr PieceType captured_piece() const {
    return decode_piece((data_ >> kCapturedShift) & kPieceMask);
  }

  [[nodiscard]] constexpr PieceType promotion_piece() const {
    return decode_piece((data_ >> kPromotionShift) & kPieceMask);
  }

  [[nodiscard]] constexpr MoveFlag flags() const {
    return static_cast<MoveFlag>((data_ >> kFlagShift) & kFlagMask);
  }

  [[nodiscard]] constexpr bool is_null() const { return data_ == 0; }
  [[nodiscard]] constexpr bool is_capture() const { return captured_piece() != PieceType::None; }
  [[nodiscard]] constexpr bool is_promotion() const {
    return has_flag(flags(), MoveFlag::Promotion) || promotion_piece() != PieceType::None;
  }

  [[nodiscard]] std::string to_uci() const;
  [[nodiscard]] constexpr std::uint32_t raw() const { return data_; }

private:
  static constexpr std::uint32_t kSquareMask = 0x3F;
  static constexpr std::uint32_t kPieceMask = 0x7;
  static constexpr std::uint32_t kFlagMask = 0x3F;
  static constexpr std::uint32_t kFromShift = 0;
  static constexpr std::uint32_t kToShift = 6;
  static constexpr std::uint32_t kMovingShift = 12;
  static constexpr std::uint32_t kCapturedShift = 15;
  static constexpr std::uint32_t kPromotionShift = 18;
  static constexpr std::uint32_t kFlagShift = 21;
  static constexpr std::uint32_t kEncodedPieceNone = 7;

  static constexpr bool is_valid_promotion(PieceType piece) {
    switch (piece) {
      case PieceType::Knight:
      case PieceType::Bishop:
      case PieceType::Rook:
      case PieceType::Queen:
      case PieceType::None:
        return true;
      default:
        return false;
    }
  }

  static constexpr std::uint32_t encode(Square from, Square to, PieceType moving, PieceType captured,
                                        PieceType promotion, MoveFlag flags) {
    const bool squares_valid = is_valid(from) && is_valid(to);
    const bool pieces_valid = moving != PieceType::None;
    const bool promotion_valid = is_valid_promotion(promotion);
    if (!squares_valid || !pieces_valid || !promotion_valid) {
      return 0;
    }

    return (static_cast<std::uint32_t>(square_index(from)) << kFromShift) |
           (static_cast<std::uint32_t>(square_index(to)) << kToShift) |
           (encode_piece(moving) << kMovingShift) |
           (encode_piece(captured) << kCapturedShift) |
           (encode_piece(promotion) << kPromotionShift) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(flags)) << kFlagShift);
  }

  static constexpr std::uint32_t encode_piece(PieceType piece) {
    return piece == PieceType::None ? kEncodedPieceNone : static_cast<std::uint32_t>(piece);
  }

  static constexpr PieceType decode_piece(std::uint32_t value) {
    return value == kEncodedPieceNone ? PieceType::None : static_cast<PieceType>(value);
  }

  std::uint32_t data_ = 0;
};

class MoveList {
public:
  using Container = std::vector<Move>;
  using iterator = Container::iterator;
  using const_iterator = Container::const_iterator;

  void add(Move move) { moves_.push_back(move); }
  void clear() { moves_.clear(); }
  [[nodiscard]] std::size_t size() const { return moves_.size(); }
  void reserve(std::size_t count) { moves_.reserve(count); }

  [[nodiscard]] const Move& operator[](std::size_t index) const { return moves_[index]; }
  [[nodiscard]] Move& operator[](std::size_t index) { return moves_[index]; }

  iterator begin() { return moves_.begin(); }
  iterator end() { return moves_.end(); }
  [[nodiscard]] const_iterator begin() const { return moves_.begin(); }
  [[nodiscard]] const_iterator end() const { return moves_.end(); }

private:
  Container moves_{};
};

enum class MoveGenerationType {
  All,
  Quiet,
  Captures
};

}  // namespace chessbot
