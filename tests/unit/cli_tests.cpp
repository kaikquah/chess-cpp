#include "test_framework.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace chessbot;

namespace {

namespace fs = std::filesystem;

std::string shell_quote(const std::string& path) {
  std::string quoted;
  quoted.push_back('\'');
  for (char ch : path) {
    if (ch == '\'') {
      quoted += "'\"'\"'";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

std::string run_cli_script(const std::string& script_name) {
  const fs::path exe_path = fs::weakly_canonical(fs::path(CHESSBOT_BINARY_DIR) / "src" / "chessbot");
  CHESSBOT_CHECK(fs::exists(exe_path));

  const fs::path script_path = fs::weakly_canonical(fs::path(CHESSBOT_SOURCE_DIR) /
                                                   "tests" / "cli_scenarios" / script_name);
  CHESSBOT_CHECK(fs::exists(script_path));

  const fs::path output_path = fs::temp_directory_path() /
                               ("chessbot_cli_" + script_name + ".log");

  std::ostringstream command;
  command << shell_quote(exe_path.string()) << " < "
          << shell_quote(script_path.string()) << " > "
          << shell_quote(output_path.string()) << " 2>&1";

  const int rc = std::system(command.str().c_str());
  CHESSBOT_CHECK(rc == 0);

  std::ifstream stream(output_path);
  CHESSBOT_CHECK(stream.good());
  std::stringstream buffer;
  buffer << stream.rdbuf();
  stream.close();
  std::error_code ec;
  fs::remove(output_path, ec);
  return buffer.str();
}

}  // namespace

CHESSBOT_TEST_CASE(cli_basic_script_executes) {
  const std::string output = run_cli_script("basic_script.txt");
  CHESSBOT_CHECK(output.find("ok") != std::string::npos);
  CHESSBOT_CHECK(output.find("Side to move: white") != std::string::npos);
  CHESSBOT_CHECK(output.find("bestmove") != std::string::npos);
}

CHESSBOT_TEST_CASE(cli_time_controls_trace) {
  const std::string output = run_cli_script("time_controls.txt");
  CHESSBOT_CHECK(output.find("info string option 'Debug Log' set to 'true'") != std::string::npos);
  CHESSBOT_CHECK(output.find("info string time budget") != std::string::npos);
  CHESSBOT_CHECK(output.find("Trace enabled") != std::string::npos);
  CHESSBOT_CHECK(output.find("bestmove") != std::string::npos);
}
