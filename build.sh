#!/usr/bin/env bash
set -e

if command -v apt >/dev/null 2>&1; then
  sudo apt update
  sudo apt install -y build-essential cmake
elif command -v brew >/dev/null 2>&1; then
  brew install cmake || true
elif command -v pacman >/dev/null 2>&1; then
  sudo pacman -Sy --needed base-devel cmake
else
  echo "Install C++ compiler and CMake manually"
fi

mkdir -p build
cd build
cmake ..
cmake --build .
