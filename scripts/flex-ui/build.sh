#!/usr/bin/env bash
# FlexUI host build + test runner.
# Default: ASan + UBSan. Pass --tsan-only to flip to TSan.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/tests/flex-ui/build"
MODE="asan"

for arg in "$@"; do
  case "$arg" in
    --tsan-only) MODE="tsan" ;;
    --no-san)    MODE="none" ;;
    --coverage)  MODE="coverage" ;;
    *)           echo "Unknown arg: $arg"; exit 2 ;;
  esac
done

CMAKE_FLAGS=()
case "$MODE" in
  asan)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=ON  -DFLEXUI_TESTS_ENABLE_TSAN=OFF) ;;
  tsan)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=ON ) ;;
  none)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=OFF) ;;
  coverage) CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=OFF -DFLEXUI_TESTS_ENABLE_COVERAGE=ON) ;;
esac

mkdir -p "$BUILD_DIR"
cmake -S "${ROOT}/tests/flex-ui" -B "$BUILD_DIR" "${CMAKE_FLAGS[@]}"
cmake --build "$BUILD_DIR" -j
ctest --test-dir "$BUILD_DIR" --output-on-failure
