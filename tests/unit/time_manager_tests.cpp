#include "test_framework.hpp"

#include "engine/time_manager.hpp"

#include <iostream>
#include <vector>

using namespace chessbot;

namespace {

TimeManagerConfig make_config(int safety, int overhead, double reserve_ratio = 0.0,
                              std::uint64_t min_reserve = 0, std::uint64_t min_alloc = 1) {
  TimeManagerConfig cfg;
  cfg.safety_margin_ms = safety;
  cfg.move_overhead_ms = overhead;
  cfg.reserve_ratio = reserve_ratio;
  cfg.minimum_reserve_ms = min_reserve;
  cfg.minimum_allocation_ms = min_alloc;
  return cfg;
}

}  // namespace

CHESSBOT_TEST_CASE(time_manager_applies_overhead_and_margin_to_movetime) {
  TimeManager manager;
  manager.set_config(make_config(/*safety=*/10, /*overhead=*/15));

  TimeManagerRequest request;
  request.side_to_move = Color::White;
  request.move_time_ms = 100ULL;

  const TimeBudget budget = manager.compute(request);
  CHESSBOT_CHECK(budget.use_time);
  CHESSBOT_CHECK(budget.hard_limit_ms == 85ULL);
  CHESSBOT_CHECK(budget.soft_limit_ms == 75ULL);
}

CHESSBOT_TEST_CASE(time_manager_handles_increment_only_clock) {
  TimeManager manager;
  manager.set_config(make_config(/*safety=*/50, /*overhead=*/10, /*reserve_ratio=*/0.05,
                                 /*min_reserve=*/50, /*min_alloc=*/10));

  TimeManagerRequest request;
  request.side_to_move = Color::White;
  request.white_time_ms = 0ULL;
  request.white_increment_ms = 1000ULL;

  const TimeBudget budget = manager.compute(request);
  CHESSBOT_CHECK(budget.use_time);
  CHESSBOT_CHECK(budget.hard_limit_ms <= 990ULL);
  CHESSBOT_CHECK(budget.hard_limit_ms >= 500ULL);
  CHESSBOT_CHECK(budget.soft_limit_ms <= budget.hard_limit_ms);
}

CHESSBOT_TEST_CASE(time_manager_reserves_overhead_from_remaining_time) {
  TimeManager manager;
  manager.set_config(make_config(/*safety=*/0, /*overhead=*/10, /*reserve_ratio=*/0.0,
                                 /*min_reserve=*/0, /*min_alloc=*/1));

  TimeManagerRequest request;
  request.side_to_move = Color::White;
  request.white_time_ms = 30ULL;
  request.moves_to_go = 1;

  const TimeBudget budget = manager.compute(request);
  CHESSBOT_CHECK(budget.use_time);
  CHESSBOT_CHECK(budget.hard_limit_ms == 20ULL);
  CHESSBOT_CHECK(budget.soft_limit_ms == 20ULL);
}

CHESSBOT_TEST_CASE(time_manager_scales_overhead_when_time_is_tiny) {
  TimeManager manager;
  manager.set_config(make_config(/*safety=*/0, /*overhead=*/50, /*reserve_ratio=*/0.0,
                                 /*min_reserve=*/0, /*min_alloc=*/1));

  TimeManagerRequest request;
  request.side_to_move = Color::White;
  request.white_time_ms = 40ULL;
  request.moves_to_go = 1;

  const TimeBudget budget = manager.compute(request);
  CHESSBOT_CHECK(budget.use_time);
  CHESSBOT_CHECK(budget.hard_limit_ms == 20ULL);
  CHESSBOT_CHECK(budget.soft_limit_ms == 20ULL);
}

CHESSBOT_TEST_CASE(time_manager_regression_matrix) {
  struct Scenario {
    const char* name;
    TimeManagerConfig config;
    TimeManagerRequest request;
    TimeBudget expected;
  };

  std::vector<Scenario> scenarios;

  {
    TimeManagerRequest request;
    request.side_to_move = Color::White;
    request.white_time_ms = 120000ULL;
    request.white_increment_ms = 2000ULL;
    request.moves_to_go = 40;

    TimeBudget expected;
    expected.use_time = true;
    expected.hard_limit_ms = 3995ULL;
    expected.soft_limit_ms = 3975ULL;

    scenarios.push_back({"classical_increment", make_config(/*safety=*/20, /*overhead=*/5,
                                                           /*reserve_ratio=*/0.10,
                                                           /*min_reserve=*/50, /*min_alloc=*/15),
                         request, expected});
  }

  {
    TimeManagerRequest request;
    request.side_to_move = Color::White;
    request.move_time_ms = 40ULL;

    TimeBudget expected;
    expected.use_time = true;
    expected.hard_limit_ms = 20ULL;
    expected.soft_limit_ms = 20ULL;

    scenarios.push_back({"movetime_overhead_clamp",
                         make_config(/*safety=*/0, /*overhead=*/50, /*reserve_ratio=*/0.0,
                                     /*min_reserve=*/0, /*min_alloc=*/1),
                         request, expected});
  }

  {
    TimeManagerRequest request;
    request.side_to_move = Color::White;
    request.white_time_ms = 0ULL;
    request.white_increment_ms = 5000ULL;

    TimeBudget expected;
    expected.use_time = true;
    expected.hard_limit_ms = 2651ULL;
    expected.soft_limit_ms = 2641ULL;

    scenarios.push_back({"increment_only_respects_minimums",
                         make_config(/*safety=*/10, /*overhead=*/15, /*reserve_ratio=*/0.0,
                                     /*min_reserve=*/0, /*min_alloc=*/10),
                         request, expected});
  }

  {
    TimeManagerRequest request;
    request.side_to_move = Color::Black;
    request.black_time_ms = 1000ULL;
    request.moves_to_go = 1;

    TimeBudget expected;
    expected.use_time = true;
    expected.hard_limit_ms = 500ULL;
    expected.soft_limit_ms = 450ULL;

    scenarios.push_back({"reserve_ratio_caps_allocation",
                         make_config(/*safety=*/50, /*overhead=*/0, /*reserve_ratio=*/0.5,
                                     /*min_reserve=*/100, /*min_alloc=*/10),
                         request, expected});
  }

  for (const auto& scenario : scenarios) {
    TimeManager manager;
    manager.set_config(scenario.config);
    const TimeBudget budget = manager.compute(scenario.request);

    if (budget.use_time != scenario.expected.use_time) {
      std::cerr << "scenario=" << scenario.name << " expected use_time="
                << scenario.expected.use_time << " actual=" << budget.use_time << '\n';
    }
    CHESSBOT_CHECK(budget.use_time == scenario.expected.use_time);

    if (budget.hard_limit_ms != scenario.expected.hard_limit_ms) {
      std::cerr << "scenario=" << scenario.name << " expected hard="
                << scenario.expected.hard_limit_ms << " actual=" << budget.hard_limit_ms
                << '\n';
    }
    CHESSBOT_CHECK(budget.hard_limit_ms == scenario.expected.hard_limit_ms);

    if (budget.soft_limit_ms != scenario.expected.soft_limit_ms) {
      std::cerr << "scenario=" << scenario.name << " expected soft="
                << scenario.expected.soft_limit_ms << " actual=" << budget.soft_limit_ms
                << '\n';
    }
    CHESSBOT_CHECK(budget.soft_limit_ms == scenario.expected.soft_limit_ms);
  }
}
