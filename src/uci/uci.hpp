#pragma once

#include <atomic>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <thread>

#include "engine/engine.hpp"
#include "engine/time_manager.hpp"
#include "uci/options.hpp"

namespace chessbot::uci {

class LoopObserver;

class UciLoop : public chessbot::EvaluationLogSink {
public:
  UciLoop(Engine& engine, std::istream& in, std::ostream& out);
  ~UciLoop();

  void run();
  void request_stop();

private:
  friend class LoopObserver;
  void handle_command(const std::string& line);
  void cmd_uci();
  void cmd_isready();
  void cmd_ucinewgame();
  void cmd_position(const std::string& rest);
  void cmd_go(const std::string& rest);
  void cmd_setoption(const std::string& rest);
  void cmd_debug(const std::string& rest);
  void cmd_register(const std::string& rest);
  void cmd_stop();
  void cmd_quit();
  void cmd_ponderhit();

  void start_search(const SearchLimits& limits);
  void stop_active_search();
  void join_finished_search();
  void report_search_result(const SearchResult& result);
  void emit_info_iteration(const SearchIterationInfo& info, int multipv);
  void publish_bestmove(const SearchResult& result);
  void flush_deferred_result();
  void apply_options();
  void on_evaluation_log(const chessbot::Board& board,
                         const chessbot::EvaluationResult& result) override;

  Engine& engine_;
  std::istream& in_;
  std::ostream& out_;
  OptionRegistry options_;
  TimeManager time_manager_{};
  std::atomic<bool> running_{false};
  std::thread search_thread_{};
  std::atomic<bool> search_active_{false};
  std::mutex io_mutex_;
  std::mutex result_mutex_;
  bool debug_mode_ = false;
  bool pondering_enabled_ = false;
  std::atomic<bool> pondering_{false};
  std::atomic<bool> awaiting_ponderhit_{false};
  std::unique_ptr<SearchObserver> observer_;
  std::optional<SearchResult> deferred_result_;
};

}  // namespace chessbot::uci
