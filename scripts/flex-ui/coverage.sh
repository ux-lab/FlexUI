#!/usr/bin/env bash
# scripts/flex-ui/coverage.sh — measure line coverage on flex-ui/common/ and flex-ui/core/
#
# Requires: CMake 3.18+, Apple LLVM (xcrun llvm-cov gcov), Python 3
# Usage:    ./scripts/flex-ui/coverage.sh [--threshold N]
#
# Exits 1 if aggregate line coverage is below THRESHOLD (default 80%).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/tests/flex-ui/build-cov"
THRESHOLD="${FLEX_COV_THRESHOLD:-80}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --threshold) THRESHOLD="$2"; shift 2 ;;
    *) echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
done

# ── Configure (only if needed) ───────────────────────────────────────────────
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cmake -S "${REPO_ROOT}/tests/flex-ui" \
        -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DFLEXUI_TESTS_ENABLE_COVERAGE=ON \
        -Wno-dev
fi

# ── Build ─────────────────────────────────────────────────────────────────────
cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

# ── Run (clean gcda first to avoid stale merge data) ─────────────────────────
find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true
find "${BUILD_DIR}" -name "*.gcov" -delete 2>/dev/null || true
"${BUILD_DIR}/unit/flexui_unit_tests"

# ── Collect coverage data from common and core ───────────────────────────────
ALL_COV_OUTPUT=""

# common
COMMON_GCDA_DIR="${BUILD_DIR}/flex-ui-build/common/CMakeFiles/flexui_common.dir/src"
if [[ -d "${COMMON_GCDA_DIR}" ]]; then
  pushd "${COMMON_GCDA_DIR}" > /dev/null
  xcrun llvm-cov gcov -b ./*.gcda > /dev/null 2>&1 || true
  COV_PART="$(xcrun llvm-cov gcov -b ./*.gcda 2>/dev/null || true)"
  ALL_COV_OUTPUT+="${COV_PART}"$'\n'
  popd > /dev/null
fi

# core subsystems
for subsystem in vdom reconciler layout js-engine; do
  # find the CMakeFiles/flexui_core_*.dir/src directory
  CORE_GCDA_DIR="${BUILD_DIR}/flex-ui-build/core/${subsystem}/CMakeFiles"
  if [[ ! -d "${CORE_GCDA_DIR}" ]]; then
    continue
  fi
  for lib_dir in "${CORE_GCDA_DIR}"/flexui_core_*.dir; do
    SRC_DIR="${lib_dir}/src"
    if [[ ! -d "${SRC_DIR}" ]]; then
      # some subsystems compile .cc directly in lib_dir (no src/ subdir)
      SRC_DIR="${lib_dir}"
    fi
    shopt -s nullglob
    gcda_files=("${SRC_DIR}"/*.gcda)
    shopt -u nullglob
    if [[ ${#gcda_files[@]} -eq 0 ]]; then
      continue
    fi
    pushd "${SRC_DIR}" > /dev/null
    xcrun llvm-cov gcov -b ./*.gcda > /dev/null 2>&1 || true
    COV_PART="$(xcrun llvm-cov gcov -b ./*.gcda 2>/dev/null || true)"
    ALL_COV_OUTPUT+="${COV_PART}"$'\n'
    popd > /dev/null
  done
done

# ── Parse and report ──────────────────────────────────────────────────────────
python3 - "${THRESHOLD}" <<PYEOF
import sys, re

raw = """${ALL_COV_OUTPUT}"""
threshold = float(sys.argv[1])

files = []
file_name = None
for line in raw.splitlines():
    m = re.search(r"File '(.*/(?:src|core)/([^']+))'", line)
    if m:
        file_name = m.group(2)
        continue
    m2 = re.search(r'Lines executed:([0-9.]+)% of ([0-9]+)', line)
    if m2 and file_name:
        pct        = float(m2.group(1))
        lines      = int(m2.group(2))
        exec_lines = round(pct * lines / 100)
        files.append((pct, exec_lines, lines, file_name))
        file_name  = None

total_lines = sum(l for _, _, l, _ in files)
total_exec  = sum(e for _, e, _, _ in files)
aggregate   = total_exec * 100 / total_lines if total_lines else 0

print("Per-file coverage (flex-ui/common/src + flex-ui/core/*/src):")
for pct, ex, tot, fn in sorted(files):
    tag = "OK " if pct >= 80 else "LOW"
    print(f"  {tag}  {pct:5.1f}%  ({ex:3d}/{tot:3d})  {fn}")
print()
print(f"Aggregate : {aggregate:.1f}%  ({total_exec}/{total_lines} lines)")
print(f"Threshold : {threshold}%")

if aggregate < threshold:
    print(f"FAIL: {aggregate:.1f}% < {threshold}% — coverage gate not met")
    sys.exit(1)
print(f"PASS: {aggregate:.1f}% >= {threshold}% — coverage gate met")
PYEOF
