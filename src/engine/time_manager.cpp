#include "engine/time_manager.hpp"

#include <algorithm>

namespace chessbot {

namespace {

std::uint64_t apply_margin(std::uint64_t value, int margin_ms) {
  if (margin_ms <= 0 || value <= static_cast<std::uint64_t>(margin_ms)) {
    return value;
  }
  return value - static_cast<std::uint64_t>(margin_ms);
}

std::uint64_t apply_overhead(std::uint64_t allocation, int overhead_ms) {
  if (overhead_ms <= 0) {
    return allocation;
  }

  const std::uint64_t overhead = static_cast<std::uint64_t>(overhead_ms);
  if (allocation <= overhead) {
    const std::uint64_t half = allocation / 2;
    return std::max<std::uint64_t>(1ULL, half);
  }

  return allocation - overhead;
}

}  // namespace

void TimeManager::set_config(const TimeManagerConfig& config) {
  TimeManagerConfig adjusted = config;
  adjusted.reserve_ratio = std::clamp(adjusted.reserve_ratio, 0.0, 0.5);
  adjusted.safety_margin_ms = std::max(adjusted.safety_margin_ms, 0);
  adjusted.move_overhead_ms = std::max(adjusted.move_overhead_ms, 0);
  adjusted.minimum_allocation_ms = std::max<std::uint64_t>(adjusted.minimum_allocation_ms, 1);
  adjusted.minimum_reserve_ms = std::max<std::uint64_t>(adjusted.minimum_reserve_ms, 0);
  config_ = adjusted;
}

TimeBudget TimeManager::compute(const TimeManagerRequest& request) const {
  TimeBudget budget{};

  if (request.infinite || request.ponder) {
    return budget;
  }

  if (request.move_time_ms && !request.infinite) {
    const std::uint64_t raw = std::max<std::uint64_t>(1ULL, *request.move_time_ms);
    std::uint64_t hard_limit = apply_overhead(raw, config_.move_overhead_ms);
    hard_limit = std::min(hard_limit, raw);
    if (hard_limit == 0) {
      hard_limit = 1;
    }

    std::uint64_t soft_limit = apply_margin(hard_limit, config_.safety_margin_ms);
    if (soft_limit == 0 || soft_limit > hard_limit) {
      soft_limit = hard_limit;
    }

    budget.use_time = true;
    budget.hard_limit_ms = hard_limit;
    budget.soft_limit_ms = soft_limit;
    return budget;
  }

  const bool has_time_value = request.side_to_move == Color::White
                                  ? request.white_time_ms.has_value()
                                  : request.black_time_ms.has_value();
  const bool has_increment_value = request.side_to_move == Color::White
                                       ? request.white_increment_ms.has_value()
                                       : request.black_increment_ms.has_value();

  if (!has_time_value && !has_increment_value) {
    return budget;
  }

  const std::uint64_t remaining = request.side_to_move == Color::White
                                      ? request.white_time_ms.value_or(0)
                                      : request.black_time_ms.value_or(0);
  const std::uint64_t increment = request.side_to_move == Color::White
                                      ? request.white_increment_ms.value_or(0)
                                      : request.black_increment_ms.value_or(0);

  if (remaining == 0 && increment == 0) {
    const std::uint64_t fallback = std::max<std::uint64_t>(1ULL, config_.minimum_allocation_ms);
    std::uint64_t hard_limit = apply_overhead(fallback, config_.move_overhead_ms);
    if (hard_limit == 0) {
      hard_limit = 1;
    }
    std::uint64_t soft_limit = apply_margin(hard_limit, config_.safety_margin_ms);
    if (soft_limit == 0 || soft_limit > hard_limit) {
      soft_limit = hard_limit;
    }
    budget.use_time = true;
    budget.hard_limit_ms = hard_limit;
    budget.soft_limit_ms = soft_limit;
    return budget;
  }

  const std::uint64_t available_base = (remaining == 0 && increment > 0) ? increment : remaining;
  const int moves_hint = std::max(1, request.moves_to_go.value_or(30));
  std::uint64_t allocation = available_base / static_cast<std::uint64_t>(moves_hint);

  if (increment > 0) {
    allocation += increment / 2;
  }

  allocation = std::max<std::uint64_t>(allocation, config_.minimum_allocation_ms);

  if (remaining > 0) {
    const std::uint64_t reserve_from_ratio = static_cast<std::uint64_t>(remaining * config_.reserve_ratio);
    const std::uint64_t reserve = std::max(config_.minimum_reserve_ms, reserve_from_ratio);

    if (remaining > reserve && allocation > remaining - reserve) {
      allocation = remaining - reserve;
    }

    allocation = std::min(allocation, remaining);
  } else {
    if (increment > 0) {
      allocation = std::min(allocation, increment);
    }
  }

  if (allocation == 0) {
    allocation = 1;
  }

  std::uint64_t hard_limit = apply_overhead(allocation, config_.move_overhead_ms);
  hard_limit = std::min(hard_limit, allocation);
  if (hard_limit == 0) {
    hard_limit = 1;
  }

  std::uint64_t soft_limit = apply_margin(hard_limit, config_.safety_margin_ms);
  if (soft_limit == 0 || soft_limit > hard_limit) {
    soft_limit = hard_limit;
  }

  budget.use_time = true;
  budget.hard_limit_ms = hard_limit;
  budget.soft_limit_ms = soft_limit;
  return budget;
}

}  // namespace chessbot
