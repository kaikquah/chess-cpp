#include "board.hpp"

#include <array>
#include <cassert>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "engine/attacks.hpp"
#include "engine/zobrist.hpp"

namespace chessbot {

namespace {

constexpr std::array<char, 13> kPieceToChar{
    '.',
    'P', 'N', 'B', 'R', 'Q', 'K',
    'p', 'n', 'b', 'r', 'q', 'k'};

Piece char_to_piece(char c) {
  switch (c) {
    case 'P': return Piece::WhitePawn;
    case 'N': return Piece::WhiteKnight;
    case 'B': return Piece::WhiteBishop;
    case 'R': return Piece::WhiteRook;
    case 'Q': return Piece::WhiteQueen;
    case 'K': return Piece::WhiteKing;
    case 'p': return Piece::BlackPawn;
    case 'n': return Piece::BlackKnight;
    case 'b': return Piece::BlackBishop;
    case 'r': return Piece::BlackRook;
    case 'q': return Piece::BlackQueen;
    case 'k': return Piece::BlackKing;
    default:  return Piece::None;
  }
}

constexpr std::uint8_t kCastleWhiteKing = 0x1;
constexpr std::uint8_t kCastleWhiteQueen = 0x2;
constexpr std::uint8_t kCastleBlackKing = 0x4;
constexpr std::uint8_t kCastleBlackQueen = 0x8;

constexpr std::array<PieceType, 4> kPromotionPieces{
    PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};

constexpr Bitboard kLightSquares = 0x55AA55AA55AA55AAULL;

}  // namespace

Board::Board() {
  clear();
}

void Board::clear() {
  squares_.fill(Piece::None);
  color_bitboards_.fill(0);
  for (auto& arr : piece_bitboards_) {
    arr.fill(0);
  }
  occupancies_.fill(0);
  side_to_move_ = Color::White;
  state_ = {};
  update_hash();
}

void Board::set_piece(Square square, Piece piece) {
  if (!is_valid(square)) {
    return;
  }

  const Piece current = squares_[square_index(square)];
  if (current == piece) {
    return;
  }

  if (current != Piece::None) {
    remove_piece_internal(square, current);
  }

  if (piece != Piece::None) {
    add_piece_internal(square, piece);
  }
}

void Board::remove_piece(Square square) {
  if (!is_valid(square)) {
    return;
  }
  const Piece piece = piece_at(square);
  if (piece == Piece::None) {
    return;
  }
  remove_piece_internal(square, piece);
}

Bitboard Board::occupancy(Color color) const {
  if (color == Color::None) {
    return occupancies_[2];
  }
  return occupancies_[static_cast<int>(color)];
}

Piece Board::piece_at(Square square) const {
  return is_valid(square) ? squares_[square_index(square)] : Piece::None;
}

Bitboard Board::pieces(Color color, PieceType type) const {
  if (color == Color::None || !is_valid(type)) {
    return 0ULL;
  }
  return piece_bitboards_[static_cast<int>(color)][static_cast<int>(type)];
}

void Board::set_side_to_move(Color color) {
  if (color == Color::None || side_to_move_ == color) {
    return;
  }
  side_to_move_ = color;
  state_.hash ^= zobrist().side_to_move();
}

void Board::set_castling_rights(std::uint8_t rights) {
  const std::uint8_t masked = rights & 0xF;
  if (state_.castling_rights == masked) {
    return;
  }
  const auto& zob = zobrist();
  state_.hash ^= zob.castling(state_.castling_rights);
  state_.castling_rights = masked;
  state_.hash ^= zob.castling(state_.castling_rights);
}

void Board::set_en_passant_file(std::uint8_t file) {
  const std::uint8_t new_file = (file < kBoardDimension) ? file : 8;
  if (state_.en_passant_file == new_file) {
    return;
  }
  const auto& zob = zobrist();
  if (state_.en_passant_file < kBoardDimension) {
    state_.hash ^= zob.en_passant(state_.en_passant_file);
  }
  state_.en_passant_file = new_file;
  if (state_.en_passant_file < kBoardDimension) {
    state_.hash ^= zob.en_passant(state_.en_passant_file);
  }
}

void Board::set_halfmove_clock(std::uint16_t clock) {
  state_.halfmove_clock = clock;
}

void Board::set_state(const GameState& state) {
  state_ = state;
  update_hash();
}

void Board::add_piece_internal(Square square, Piece piece, bool update_hash) {
  const auto& zob = zobrist();
  const int idx = square_index(square);
  squares_[idx] = piece;

  const Color color = color_of(piece);
  const PieceType type = type_of(piece);
  const int color_idx = static_cast<int>(color);
  const int type_idx = static_cast<int>(type);
  const Bitboard mask = bit(square);

  color_bitboards_[color_idx] |= mask;
  piece_bitboards_[color_idx][type_idx] |= mask;
  occupancies_[color_idx] = color_bitboards_[color_idx];
  occupancies_[2] = occupancies_[0] | occupancies_[1];

  if (piece == Piece::WhiteKing) {
    state_.king_square_white = square;
  } else if (piece == Piece::BlackKing) {
    state_.king_square_black = square;
  }

  if (update_hash) {
    state_.hash ^= zob.piece_square(piece, square);
  }
}

void Board::remove_piece_internal(Square square, Piece piece, bool update_hash) {
  const auto& zob = zobrist();
  const int idx = square_index(square);
  squares_[idx] = Piece::None;

  const Color color = color_of(piece);
  const PieceType type = type_of(piece);
  const int color_idx = static_cast<int>(color);
  const int type_idx = static_cast<int>(type);
  const Bitboard mask = bit(square);

  color_bitboards_[color_idx] &= ~mask;
  piece_bitboards_[color_idx][type_idx] &= ~mask;
  occupancies_[color_idx] = color_bitboards_[color_idx];
  occupancies_[2] = occupancies_[0] | occupancies_[1];

  if (piece == Piece::WhiteKing) {
    state_.king_square_white = Square::None;
  } else if (piece == Piece::BlackKing) {
    state_.king_square_black = Square::None;
  }

  if (update_hash) {
    state_.hash ^= zob.piece_square(piece, square);
  }
}

void Board::move_piece_internal(Square from, Square to, Piece piece, bool update_hash) {
  const auto& zob = zobrist();
  const int from_idx = square_index(from);
  const int to_idx = square_index(to);
  squares_[from_idx] = Piece::None;
  squares_[to_idx] = piece;

  const Color color = color_of(piece);
  const PieceType type = type_of(piece);
  const int color_idx = static_cast<int>(color);
  const int type_idx = static_cast<int>(type);
  const Bitboard from_mask = bit(from);
  const Bitboard to_mask = bit(to);

  color_bitboards_[color_idx] &= ~from_mask;
  color_bitboards_[color_idx] |= to_mask;
  piece_bitboards_[color_idx][type_idx] &= ~from_mask;
  piece_bitboards_[color_idx][type_idx] |= to_mask;
  occupancies_[color_idx] = color_bitboards_[color_idx];
  occupancies_[2] = occupancies_[0] | occupancies_[1];

  if (piece == Piece::WhiteKing) {
    state_.king_square_white = to;
  } else if (piece == Piece::BlackKing) {
    state_.king_square_black = to;
  }

  if (update_hash) {
    state_.hash ^= zob.piece_square(piece, from);
    state_.hash ^= zob.piece_square(piece, to);
  }
}

void Board::set_start_position() {
  set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::set_from_fen(const std::string& fen) {
  clear();

  std::istringstream ss(fen);
  std::string placement;
  ss >> placement;
  if (placement.empty()) {
    throw std::invalid_argument("FEN missing placement");
  }

  std::string active_color;
  ss >> active_color;
  if (active_color != "w" && active_color != "b") {
    throw std::invalid_argument("Invalid active color in FEN");
  }

  std::string castling;
  if (!(ss >> castling)) {
    throw std::invalid_argument("FEN missing castling field");
  }
  std::string en_passant;
  if (!(ss >> en_passant)) {
    throw std::invalid_argument("FEN missing en passant field");
  }
  std::string halfmove_str;
  if (!(ss >> halfmove_str)) {
    throw std::invalid_argument("FEN missing halfmove clock field");
  }
  std::string fullmove_str;
  if (!(ss >> fullmove_str)) {
    throw std::invalid_argument("FEN missing fullmove number field");
  }

  auto placement_parts = std::array<std::string, 8>{};
  {
    std::istringstream placement_ss(placement);
    std::string rank_str;
    int rank = 7;
    while (std::getline(placement_ss, rank_str, '/')) {
      if (rank < 0) {
        throw std::invalid_argument("Too many ranks in FEN");
      }
      placement_parts[rank--] = rank_str;
    }
    if (rank != -1) {
      throw std::invalid_argument("Not enough ranks in FEN");
    }
  }

  int white_king_count = 0;
  int black_king_count = 0;

  for (int rank = 7; rank >= 0; --rank) {
    const std::string& row = placement_parts[rank];
    int file = 0;
    for (char c : row) {
      if (std::isdigit(static_cast<unsigned char>(c))) {
        const int digit = c - '0';
        if (digit < 1 || digit > 8 || file + digit > 8) {
          throw std::invalid_argument("Invalid digit in FEN rank");
        }
        file += digit;
      } else {
        const Piece piece = char_to_piece(c);
        if (piece == Piece::None) {
          throw std::invalid_argument("Invalid piece char in FEN");
        }
        if (file >= 8) {
          throw std::invalid_argument("File out of range while parsing FEN");
        }
        set_piece(make_square(file, rank), piece);
        if (piece == Piece::WhiteKing) {
          ++white_king_count;
        } else if (piece == Piece::BlackKing) {
          ++black_king_count;
        }
        ++file;
      }
    }
    if (file != 8) {
      throw std::invalid_argument("Rank does not sum to 8 squares");
    }
  }

  set_side_to_move(active_color == "w" ? Color::White : Color::Black);

  if (white_king_count != 1 || black_king_count != 1) {
    throw std::invalid_argument("FEN must contain exactly one king per side");
  }

  std::uint8_t rights = 0;
  if (castling != "-") {
    for (char c : castling) {
      switch (c) {
        case 'K': rights |= 0x1; break;
        case 'Q': rights |= 0x2; break;
        case 'k': rights |= 0x4; break;
        case 'q': rights |= 0x8; break;
        default: throw std::invalid_argument("Invalid castling char");
      }
    }
  }
  set_castling_rights(rights);

  if (en_passant == "-") {
    set_en_passant_file(8);
  } else {
    if (en_passant.size() != 2 || en_passant[0] < 'a' || en_passant[0] > 'h' ||
        (en_passant[1] != '3' && en_passant[1] != '6')) {
      throw std::invalid_argument("Invalid en passant square");
    }
    const char expected_rank = (active_color == "w") ? '6' : '3';
    if (en_passant[1] != expected_rank) {
      throw std::invalid_argument("En passant rank inconsistent with side to move");
    }
    set_en_passant_file(static_cast<std::uint8_t>(en_passant[0] - 'a'));
  }

  try {
    const int value = std::stoi(halfmove_str);
    if (value < 0 || value > std::numeric_limits<std::uint16_t>::max()) {
      throw std::invalid_argument("Invalid halfmove clock value in FEN");
    }
    state_.halfmove_clock = static_cast<std::uint16_t>(value);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument("Invalid halfmove clock value in FEN");
  } catch (const std::out_of_range&) {
    throw std::invalid_argument("Halfmove clock value out of range in FEN");
  }

  try {
    const int value = std::stoi(fullmove_str);
    if (value <= 0 || value > std::numeric_limits<std::uint16_t>::max()) {
      throw std::invalid_argument("Invalid fullmove number value in FEN");
    }
    state_.fullmove_number = static_cast<std::uint16_t>(value);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument("Invalid fullmove number value in FEN");
  } catch (const std::out_of_range&) {
    throw std::invalid_argument("Fullmove number value out of range in FEN");
  }

  state_.hash = 0ULL;
  update_hash();

}

std::string Board::to_fen() const {
  std::ostringstream oss;

  for (int rank = 7; rank >= 0; --rank) {
    int empty = 0;
    for (int file = 0; file < 8; ++file) {
      const Square square = make_square(file, rank);
      const Piece piece = piece_at(square);
      if (piece == Piece::None) {
        ++empty;
      } else {
        if (empty > 0) {
          oss << empty;
          empty = 0;
        }
        oss << kPieceToChar[static_cast<std::uint8_t>(piece)];
      }
    }
    if (empty > 0) {
      oss << empty;
    }
    if (rank > 0) {
      oss << '/';
    }
  }

  oss << ' ' << (side_to_move_ == Color::White ? 'w' : 'b');

  if (state_.castling_rights == 0) {
    oss << " -";
  } else {
    oss << ' ';
    if (state_.castling_rights & 0x1) oss << 'K';
    if (state_.castling_rights & 0x2) oss << 'Q';
    if (state_.castling_rights & 0x4) oss << 'k';
    if (state_.castling_rights & 0x8) oss << 'q';
  }

  if (state_.en_passant_file >= 8) {
    oss << " -";
  } else {
    const char file_char = static_cast<char>('a' + state_.en_passant_file);
    const char rank_char = side_to_move_ == Color::White ? '6' : '3';
    oss << ' ' << file_char << rank_char;
  }

  oss << ' ' << state_.halfmove_clock << ' ' << state_.fullmove_number;

  return oss.str();
}

bool Board::make_move(const Move& move, MoveState& move_state) {
  const Color us = side_to_move_;
  if (us == Color::None || move.is_null()) {
    return false;
  }

  const Square from = move.from();
  const Square to = move.to();
  if (!is_valid(from) || !is_valid(to)) {
    return false;
  }

  const Piece moving_piece = piece_at(from);
  if (moving_piece == Piece::None || color_of(moving_piece) != us) {
    return false;
  }

  const PieceType moving_type = type_of(moving_piece);
  const Color them = opposite(us);

  const bool is_promotion = move.is_promotion();
  const bool is_double_push = has_flag(move.flags(), MoveFlag::DoublePawnPush);
  const bool is_en_passant = has_flag(move.flags(), MoveFlag::EnPassant);
  const bool is_kingside_castle = has_flag(move.flags(), MoveFlag::KingCastle);
  const bool is_queenside_castle = has_flag(move.flags(), MoveFlag::QueenCastle);

  if (is_promotion) {
    const int promotion_rank = rank_of(to);
    if (moving_type != PieceType::Pawn ||
        (us == Color::White && promotion_rank != 7) ||
        (us == Color::Black && promotion_rank != 0)) {
      return false;
    }
  }

  if (is_double_push) {
    const int from_rank = rank_of(from);
    const int to_rank = rank_of(to);
    if (moving_type != PieceType::Pawn ||
        (us == Color::White && (from_rank != 1 || to_rank != 3)) ||
        (us == Color::Black && (from_rank != 6 || to_rank != 4))) {
      return false;
    }
  }

  Piece captured_piece = Piece::None;
  Square capture_square = to;

  if (is_en_passant) {
    if (moving_type != PieceType::Pawn) {
      return false;
    }
    const int expected_rank = (us == Color::White) ? 5 : 2;
    if (rank_of(to) != expected_rank || state_.en_passant_file >= kBoardDimension ||
        state_.en_passant_file != file_of(to)) {
      return false;
    }
    capture_square = make_square(state_.en_passant_file, (us == Color::White) ? 4 : 3);
    if (!is_valid(capture_square)) {
      return false;
    }
    captured_piece = piece_at(capture_square);
    if (captured_piece != make_piece(them, PieceType::Pawn)) {
      return false;
    }
  } else {
    captured_piece = piece_at(to);
    if (captured_piece != Piece::None && color_of(captured_piece) == us) {
      return false;
    }
  }

  if ((is_kingside_castle || is_queenside_castle) && moving_type != PieceType::King) {
    return false;
  }

  if (is_kingside_castle) {
    const std::uint8_t required = (us == Color::White) ? kCastleWhiteKing : kCastleBlackKing;
    if ((state_.castling_rights & required) == 0) {
      return false;
    }
    const Square rook_from = (us == Color::White) ? Square::H1 : Square::H8;
    if (piece_at(rook_from) != make_piece(us, PieceType::Rook) || captured_piece != Piece::None) {
      return false;
    }
  }
  if (is_queenside_castle) {
    const std::uint8_t required = (us == Color::White) ? kCastleWhiteQueen : kCastleBlackQueen;
    if ((state_.castling_rights & required) == 0) {
      return false;
    }
    const Square rook_from = (us == Color::White) ? Square::A1 : Square::A8;
    if (piece_at(rook_from) != make_piece(us, PieceType::Rook) || captured_piece != Piece::None) {
      return false;
    }
  }

  move_state.game_state = state_;
  move_state.side_to_move = us;
  move_state.captured_piece = captured_piece;
  move_state.captured_square = captured_piece == Piece::None ? Square::None : capture_square;

  std::uint8_t new_castling = state_.castling_rights;
  if (moving_piece == Piece::WhiteKing) {
    new_castling &= ~(kCastleWhiteKing | kCastleWhiteQueen);
  } else if (moving_piece == Piece::BlackKing) {
    new_castling &= ~(kCastleBlackKing | kCastleBlackQueen);
  } else if (moving_piece == Piece::WhiteRook) {
    if (from == Square::H1) new_castling &= ~kCastleWhiteKing;
    if (from == Square::A1) new_castling &= ~kCastleWhiteQueen;
  } else if (moving_piece == Piece::BlackRook) {
    if (from == Square::H8) new_castling &= ~kCastleBlackKing;
    if (from == Square::A8) new_castling &= ~kCastleBlackQueen;
  }

  if (captured_piece == Piece::WhiteRook) {
    if (capture_square == Square::H1) new_castling &= ~kCastleWhiteKing;
    if (capture_square == Square::A1) new_castling &= ~kCastleWhiteQueen;
  } else if (captured_piece == Piece::BlackRook) {
    if (capture_square == Square::H8) new_castling &= ~kCastleBlackKing;
    if (capture_square == Square::A8) new_castling &= ~kCastleBlackQueen;
  }

  if (captured_piece != Piece::None) {
    remove_piece_internal(capture_square, captured_piece);
  }

  move_piece_internal(from, to, moving_piece);

  if (is_promotion) {
    const Piece promoted = make_piece(us, move.promotion_piece());
    remove_piece_internal(to, moving_piece);
    add_piece_internal(to, promoted);
  }

  if (is_kingside_castle) {
    const Square rook_from = (us == Color::White) ? Square::H1 : Square::H8;
    const Square rook_to = (us == Color::White) ? Square::F1 : Square::F8;
    if (Piece rook = piece_at(rook_from); rook != Piece::None) {
      move_piece_internal(rook_from, rook_to, rook);
    }
  } else if (is_queenside_castle) {
    const Square rook_from = (us == Color::White) ? Square::A1 : Square::A8;
    const Square rook_to = (us == Color::White) ? Square::D1 : Square::D8;
    if (Piece rook = piece_at(rook_from); rook != Piece::None) {
      move_piece_internal(rook_from, rook_to, rook);
    }
  }

  const std::uint8_t new_en_passant = (is_double_push && !is_en_passant)
                                          ? static_cast<std::uint8_t>(file_of(from))
                                          : 8;
  set_en_passant_file(new_en_passant);
  set_castling_rights(new_castling);

  if (moving_type == PieceType::Pawn || captured_piece != Piece::None) {
    state_.halfmove_clock = 0;
  } else if (state_.halfmove_clock < std::numeric_limits<std::uint16_t>::max()) {
    state_.halfmove_clock = static_cast<std::uint16_t>(state_.halfmove_clock + 1);
  }

  if (us == Color::Black) {
    ++state_.fullmove_number;
  }

  set_side_to_move(them);
  return true;
}

void Board::unmake_move(const Move& move, const MoveState& move_state) {
  const Color us = move_state.side_to_move;
  const Square from = move.from();
  const Square to = move.to();

  if (has_flag(move.flags(), MoveFlag::KingCastle)) {
    const Square rook_from = us == Color::White ? Square::F1 : Square::F8;
    const Square rook_to = us == Color::White ? Square::H1 : Square::H8;
    if (Piece rook = piece_at(rook_from); rook != Piece::None) {
      move_piece_internal(rook_from, rook_to, rook, false);
    }
  } else if (has_flag(move.flags(), MoveFlag::QueenCastle)) {
    const Square rook_from = us == Color::White ? Square::D1 : Square::D8;
    const Square rook_to = us == Color::White ? Square::A1 : Square::A8;
    if (Piece rook = piece_at(rook_from); rook != Piece::None) {
      move_piece_internal(rook_from, rook_to, rook, false);
    }
  }

  Piece piece_on_to = piece_at(to);
  if (move.is_promotion()) {
    if (piece_on_to != Piece::None) {
      remove_piece_internal(to, piece_on_to, false);
    }
    piece_on_to = make_piece(us, move.moving_piece());
    add_piece_internal(to, piece_on_to, false);
  }

  if (piece_on_to != Piece::None) {
    move_piece_internal(to, from, piece_on_to, false);
  }

  if (move_state.captured_piece != Piece::None) {
    add_piece_internal(move_state.captured_square, move_state.captured_piece, false);
  }

  state_ = move_state.game_state;
  side_to_move_ = move_state.side_to_move;
}

bool Board::is_square_attacked(Square square, Color attacker) const {
  if (!is_valid(square) || attacker == Color::None) {
    return false;
  }

  const Bitboard pawns = pieces(attacker, PieceType::Pawn);
  if ((pawn_attacks(opposite(attacker), square) & pawns) != 0ULL) {
    return true;
  }

  const Bitboard knights = pieces(attacker, PieceType::Knight);
  if ((knight_attacks(square) & knights) != 0ULL) {
    return true;
  }

  const Bitboard kings = pieces(attacker, PieceType::King);
  if ((king_attacks(square) & kings) != 0ULL) {
    return true;
  }

  const Bitboard occupancy_all = occupancy(Color::None);
  const Bitboard bishops = pieces(attacker, PieceType::Bishop);
  const Bitboard rooks = pieces(attacker, PieceType::Rook);
  const Bitboard queens = pieces(attacker, PieceType::Queen);

  if ((bishop_attacks(square, occupancy_all) & (bishops | queens)) != 0ULL) {
    return true;
  }
  if ((rook_attacks(square, occupancy_all) & (rooks | queens)) != 0ULL) {
    return true;
  }

  return false;
}

bool Board::has_insufficient_material() const {
  const auto has_major_material = [&](Color color) {
    return pieces(color, PieceType::Pawn) != 0ULL ||
           pieces(color, PieceType::Rook) != 0ULL ||
           pieces(color, PieceType::Queen) != 0ULL;
  };

  if (has_major_material(Color::White) || has_major_material(Color::Black)) {
    return false;
  }

  const Bitboard white_knights = pieces(Color::White, PieceType::Knight);
  const Bitboard black_knights = pieces(Color::Black, PieceType::Knight);
  const Bitboard white_bishops = pieces(Color::White, PieceType::Bishop);
  const Bitboard black_bishops = pieces(Color::Black, PieceType::Bishop);

  const int total_knights = popcount(white_knights) + popcount(black_knights);
  const int total_bishops = popcount(white_bishops) + popcount(black_bishops);

  if (total_knights == 0 && total_bishops == 0) {
    return true;
  }
  if (total_knights == 1 && total_bishops == 0) {
    return true;
  }
  if (total_knights == 0 && total_bishops == 1) {
    return true;
  }

  if (total_knights == 0 && total_bishops > 0) {
    Bitboard bishops = white_bishops | black_bishops;
    int light_count = 0;
    while (bishops) {
      const Square sq = lsb(bishops);
      if ((bit(sq) & kLightSquares) != 0ULL) {
        ++light_count;
      }
      bishops &= bishops - 1ULL;
    }
    const int dark_count = total_bishops - light_count;
    if (light_count == 0 || dark_count == 0) {
      return true;
    }
  }

  return false;
}

void Board::generate_pseudo_legal_moves(MoveList& moves, MoveGenerationType type) const {
  const Color us = side_to_move_;
  if (us == Color::None) {
    return;
  }
  const Color them = opposite(us);
  const Bitboard occupancy_them = occupancy(them);
  const Bitboard occupancy_all = occupancy(Color::None);
  const bool include_quiet = type != MoveGenerationType::Captures;
  const bool include_captures = type != MoveGenerationType::Quiet;

  auto add_move = [&](Square from_sq, Square to_sq, PieceType moving, PieceType captured,
                      MoveFlag flags, PieceType promotion) {
    const bool is_capture = captured != PieceType::None;
    if ((type == MoveGenerationType::Quiet && is_capture) ||
        (type == MoveGenerationType::Captures && !is_capture)) {
      return;
    }

    MoveFlag final_flags = flags;
    if (is_capture) {
      final_flags |= MoveFlag::Capture;
    }
    if (promotion != PieceType::None) {
      final_flags |= MoveFlag::Promotion;
    }
    moves.add(Move{from_sq, to_sq, moving, captured, promotion, final_flags});
  };

  Bitboard pawns = pieces(us, PieceType::Pawn);
  while (pawns) {
    const Square from_sq = pop_lsb(pawns);

    if (include_quiet) {
      Bitboard singles = pawn_single_push_targets(us, from_sq, occupancy_all);
      while (singles) {
        const Square to_sq = pop_lsb(singles);
        const bool promotion_rank = rank_of(to_sq) == (us == Color::White ? 7 : 0);
        if (promotion_rank) {
          for (PieceType promo : kPromotionPieces) {
            add_move(from_sq, to_sq, PieceType::Pawn, PieceType::None, MoveFlag::None, promo);
          }
        } else {
          add_move(from_sq, to_sq, PieceType::Pawn, PieceType::None, MoveFlag::None, PieceType::None);
        }
      }

      Bitboard doubles = pawn_double_push_targets(us, from_sq, occupancy_all);
      while (doubles) {
        const Square to_sq = pop_lsb(doubles);
        add_move(from_sq, to_sq, PieceType::Pawn, PieceType::None, MoveFlag::DoublePawnPush, PieceType::None);
      }
    }

    if (include_captures) {
      Bitboard captures = pawn_attacks(us, from_sq) & occupancy_them;
      while (captures) {
        const Square to_sq = pop_lsb(captures);
        const bool promotion_rank = rank_of(to_sq) == (us == Color::White ? 7 : 0);
        const PieceType captured_type = type_of(piece_at(to_sq));
        if (promotion_rank) {
          for (PieceType promo : kPromotionPieces) {
            add_move(from_sq, to_sq, PieceType::Pawn, captured_type, MoveFlag::None, promo);
          }
        } else {
          add_move(from_sq, to_sq, PieceType::Pawn, captured_type, MoveFlag::None, PieceType::None);
        }
      }

      if (state_.en_passant_file < kBoardDimension) {
        const int ep_rank = us == Color::White ? 5 : 2;
        const Square ep_square = make_square(state_.en_passant_file, ep_rank);
        if (is_valid(ep_square) && get_bit(pawn_attacks(us, from_sq), ep_square)) {
          const Square capture_square = make_square(state_.en_passant_file, us == Color::White ? 4 : 3);
          if (is_valid(capture_square) && piece_at(capture_square) == make_piece(them, PieceType::Pawn)) {
            add_move(from_sq, ep_square, PieceType::Pawn, PieceType::Pawn, MoveFlag::EnPassant, PieceType::None);
          }
        }
      }
    }
  }

  Bitboard knights = pieces(us, PieceType::Knight);
  while (knights) {
    const Square from_sq = pop_lsb(knights);
    const Bitboard attacks = knight_attacks(from_sq);
    if (include_captures) {
      Bitboard targets = attacks & occupancy_them;
      while (targets) {
        const Square to_sq = pop_lsb(targets);
        add_move(from_sq, to_sq, PieceType::Knight, type_of(piece_at(to_sq)), MoveFlag::None, PieceType::None);
      }
    }
    if (include_quiet) {
      Bitboard targets = attacks & ~occupancy_all;
      while (targets) {
        const Square to_sq = pop_lsb(targets);
        add_move(from_sq, to_sq, PieceType::Knight, PieceType::None, MoveFlag::None, PieceType::None);
      }
    }
  }

  auto generate_sliding = [&](PieceType type) {
    Bitboard pieces_bb = pieces(us, type);
    while (pieces_bb) {
      const Square from_sq = pop_lsb(pieces_bb);
      Bitboard attacks = 0ULL;
      if (type == PieceType::Bishop) {
        attacks = bishop_attacks(from_sq, occupancy_all);
      } else if (type == PieceType::Rook) {
        attacks = rook_attacks(from_sq, occupancy_all);
      } else {
        attacks = queen_attacks(from_sq, occupancy_all);
      }

      if (include_captures) {
        Bitboard targets = attacks & occupancy_them;
        while (targets) {
          const Square to_sq = pop_lsb(targets);
          add_move(from_sq, to_sq, type, type_of(piece_at(to_sq)), MoveFlag::None, PieceType::None);
        }
      }
      if (include_quiet) {
        Bitboard targets = attacks & ~occupancy_all;
        while (targets) {
          const Square to_sq = pop_lsb(targets);
          add_move(from_sq, to_sq, type, PieceType::None, MoveFlag::None, PieceType::None);
        }
      }
    }
  };

  generate_sliding(PieceType::Bishop);
  generate_sliding(PieceType::Rook);
  generate_sliding(PieceType::Queen);

  Bitboard kings = pieces(us, PieceType::King);
  if (kings) {
    const Square from_sq = pop_lsb(kings);
    const Bitboard attacks = king_attacks(from_sq);
    if (include_captures) {
      Bitboard targets = attacks & occupancy_them;
      while (targets) {
        const Square to_sq = pop_lsb(targets);
        add_move(from_sq, to_sq, PieceType::King, type_of(piece_at(to_sq)), MoveFlag::None, PieceType::None);
      }
    }
    if (include_quiet) {
      Bitboard targets = attacks & ~occupancy_all;
      while (targets) {
        const Square to_sq = pop_lsb(targets);
        add_move(from_sq, to_sq, PieceType::King, PieceType::None, MoveFlag::None, PieceType::None);
      }

      const bool can_castle_kingside = (us == Color::White && (state_.castling_rights & kCastleWhiteKing)) ||
                                       (us == Color::Black && (state_.castling_rights & kCastleBlackKing));
      if (can_castle_kingside) {
        const Square f_sq = us == Color::White ? Square::F1 : Square::F8;
        const Square g_sq = us == Color::White ? Square::G1 : Square::G8;
        const Square rook_sq = us == Color::White ? Square::H1 : Square::H8;
        if (piece_at(f_sq) == Piece::None && piece_at(g_sq) == Piece::None &&
            piece_at(rook_sq) == make_piece(us, PieceType::Rook) &&
            !is_square_attacked(from_sq, them) && !is_square_attacked(f_sq, them) &&
            !is_square_attacked(g_sq, them)) {
          add_move(from_sq, g_sq, PieceType::King, PieceType::None, MoveFlag::KingCastle, PieceType::None);
        }
      }

      const bool can_castle_queenside = (us == Color::White && (state_.castling_rights & kCastleWhiteQueen)) ||
                                        (us == Color::Black && (state_.castling_rights & kCastleBlackQueen));
      if (can_castle_queenside) {
        const Square d_sq = us == Color::White ? Square::D1 : Square::D8;
        const Square c_sq = us == Color::White ? Square::C1 : Square::C8;
        const Square b_sq = us == Color::White ? Square::B1 : Square::B8;
        const Square rook_sq = us == Color::White ? Square::A1 : Square::A8;
        if (piece_at(d_sq) == Piece::None && piece_at(c_sq) == Piece::None && piece_at(b_sq) == Piece::None &&
            piece_at(rook_sq) == make_piece(us, PieceType::Rook) &&
            !is_square_attacked(from_sq, them) && !is_square_attacked(d_sq, them) &&
            !is_square_attacked(c_sq, them)) {
          add_move(from_sq, c_sq, PieceType::King, PieceType::None, MoveFlag::QueenCastle, PieceType::None);
        }
      }
    }
  }
}

void Board::generate_legal_moves(MoveList& moves, MoveGenerationType type) {
  MoveList pseudo;
  pseudo.reserve(64);
  generate_pseudo_legal_moves(pseudo, type);

  for (const Move& move : pseudo) {
    MoveState move_state;
    const bool made = make_move(move, move_state);
    assert(made && "Pseudo-legal move rejected during make_move");
    if (!made) {
      continue;
    }

    const Color us = opposite(side_to_move_);
    const Square king_sq = king_square(us);
    const bool illegal = !is_valid(king_sq) || is_square_attacked(king_sq, side_to_move_);

    unmake_move(move, move_state);

    if (!illegal) {
      moves.add(move);
    }
  }
}

void Board::update_hash() {
  std::uint64_t hash = 0ULL;
  const auto& zob = zobrist();

  for (int sq = 0; sq < kBoardSquareCount; ++sq) {
    const Piece piece = squares_[sq];
    if (piece != Piece::None) {
      hash ^= zob.piece_square(piece, static_cast<Square>(sq));
    }
  }

  hash ^= zob.castling(state_.castling_rights);

  if (state_.en_passant_file < kBoardDimension) {
    hash ^= zob.en_passant(state_.en_passant_file);
  }

  if (side_to_move_ == Color::Black) {
    hash ^= zob.side_to_move();
  }

  state_.hash = hash;
}

}  // namespace chessbot
