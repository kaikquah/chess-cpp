set(TEST_SOURCES
    unit/main.cpp
    unit/bitboard_tests.cpp
    unit/board_tests.cpp
    unit/move_tests.cpp
    unit/attacks_tests.cpp
    unit/movegen_tests.cpp
    unit/perft_tests.cpp
    unit/evaluation_tests.cpp
    unit/search_tests.cpp
    unit/time_manager_tests.cpp
    unit/cli_tests.cpp
    unit/uci_tests.cpp
)

add_executable(chessbot_tests ${TEST_SOURCES})

set_target_properties(chessbot_tests PROPERTIES OUTPUT_NAME chessbot-tests)

target_include_directories(chessbot_tests
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(chessbot_tests
    PRIVATE
      chessbot::engine
)

target_compile_definitions(chessbot_tests
    PRIVATE
      CHESSBOT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
      CHESSBOT_BINARY_DIR="${CMAKE_BINARY_DIR}"
)

add_test(NAME chessbot-unit-tests COMMAND chessbot-tests)
set_tests_properties(chessbot-unit-tests PROPERTIES TIMEOUT 120)
