#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace chessbot::test {

struct TestFailure : public std::exception {
  TestFailure(std::string expression, std::string file, int line)
      : expression_(std::move(expression)), file_(std::move(file)), line_(line) {
    message_ = expression_ + " failed at " + file_ + ':' + std::to_string(line_);
  }

  [[nodiscard]] const char* what() const noexcept override {
    return message_.c_str();
  }

  std::string expression_;
  std::string file_;
  int line_;
  std::string message_;
};

using TestFn = void (*)();

class Registry {
public:
  static Registry& instance() {
    static Registry registry;
    return registry;
  }

  void add(std::string name, TestFn fn) {
    tests_.emplace_back(std::move(name), fn);
  }

  [[nodiscard]] int run_all() const {
    int failures = 0;
    for (const auto& [name, fn] : tests_) {
      try {
        fn();
        std::cout << "[PASS] " << name << '\n';
      } catch (const TestFailure& failure) {
        ++failures;
        std::cout << "[FAIL] " << name << " - " << failure.what() << '\n';
      }
    }
    std::cout << tests_.size() - failures << "/" << tests_.size() << " tests passed" << std::endl;
    return failures == 0 ? 0 : 1;
  }

private:
  std::vector<std::pair<std::string, TestFn>> tests_;
};

struct Registrar {
  Registrar(const char* name, TestFn fn) {
    Registry::instance().add(name, fn);
  }
};

inline void check(bool condition, const char* expression, const char* file, int line) {
  if (!condition) {
    throw TestFailure(expression, file, line);
  }
}

}  // namespace chessbot::test

#define CHESSBOT_TEST_CASE(name)                                                  \
  void name();                                                                    \
  static ::chessbot::test::Registrar registrar_##name(#name, &name);              \
  void name()

#define CHESSBOT_CHECK(expression)                                                \
  ::chessbot::test::check((expression), #expression, __FILE__, __LINE__)

#define CHESSBOT_REQUIRE(expression)                                              \
  ::chessbot::test::check((expression), #expression, __FILE__, __LINE__)

inline int run_all_tests() {
  return ::chessbot::test::Registry::instance().run_all();
}
