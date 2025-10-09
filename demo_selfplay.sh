#!/bin/bash
# ChessBot self-play demo

ENGINE="./build/src/chessbot"

# Commands to feed the engine
cat <<EOF | $ENGINE
position startpos
go depth 5
EOF
