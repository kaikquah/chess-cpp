#pragma once

#include <cstdint>
#include <optional>

#include "core/types.hpp"

namespace chessbot {

struct TimeManagerConfig {
  int safety_margin_ms = 50;        // Grace period before hard cutoff
  int move_overhead_ms = 10;        // Reserved per-move overhead (GUI latency)
  double reserve_ratio = 0.05;      // Fraction of remaining time to keep in reserve
  std::uint64_t minimum_reserve_ms = 50;  // Absolute minimum reserve
  std::uint64_t minimum_allocation_ms = 10;  // Minimum thinking time when clocks exist
};

struct TimeManagerRequest {
  Color side_to_move = Color::White;
  bool ponder = false;
  bool infinite = false;
  std::optional<std::uint64_t> move_time_ms;
  std::optional<std::uint64_t> white_time_ms;
  std::optional<std::uint64_t> black_time_ms;
  std::optional<std::uint64_t> white_increment_ms;
  std::optional<std::uint64_t> black_increment_ms;
  std::optional<int> moves_to_go;
};

struct TimeBudget {
  bool use_time = false;
  std::uint64_t soft_limit_ms = 0;  // Preferred stop time (search may finish earlier)
  std::uint64_t hard_limit_ms = 0;  // Absolute cutoff enforced by search
};

class TimeManager {
public:
  TimeManager() = default;

  void set_config(const TimeManagerConfig& config);
  [[nodiscard]] TimeBudget compute(const TimeManagerRequest& request) const;

private:
  TimeManagerConfig config_{};
};

}  // namespace chessbot

