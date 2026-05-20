#!/usr/bin/env bash
set -e
HOST=${1:-127.0.0.1}
PORT=${2:-8080}
if [ ! -f build/simpledb_app ]; then
  ./build.sh
fi
./build/simpledb_app "$HOST" "$PORT"
