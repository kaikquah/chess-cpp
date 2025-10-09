set(ENGINE_SOURCES
    core/types.cpp
    core/bitboard.cpp
    engine/board.cpp
    engine/move.cpp
    engine/attacks.cpp
    engine/evaluation.cpp
    engine/search.cpp
    engine/time_manager.cpp
    engine/perft.cpp
    engine/zobrist.cpp
    engine/engine.cpp
    uci/options.cpp
    uci/uci.cpp
)

add_library(chessbot_engine STATIC ${ENGINE_SOURCES})
add_library(chessbot::engine ALIAS chessbot_engine)

target_include_directories(chessbot_engine
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(chessbot_engine PUBLIC cxx_std_20)

add_executable(chessbot apps/chessbot/main.cpp)

target_link_libraries(chessbot PRIVATE chessbot::engine)

add_executable(perft_tool apps/perft/main.cpp)
set_target_properties(perft_tool PROPERTIES OUTPUT_NAME chessbot-perft)
target_link_libraries(perft_tool PRIVATE chessbot::engine)
