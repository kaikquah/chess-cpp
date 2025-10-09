cmake_minimum_required(VERSION 3.20)

project(chessbot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHESSBOT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CHESSBOT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(CHESSBOT_BUILD_TESTS "Build unit and regression tests" ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(CHESSBOT_ENABLE_ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(CHESSBOT_ENABLE_UBSAN)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()

add_subdirectory(src)

if(CHESSBOT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
