#!/usr/bin/env bash
# FlexUI HarmonyOS HAR build script.
# Builds libflexui_napi.so via CMake + Harmony toolchain, then assembles the HAR.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODE="${1:-debug}"
OUT="${ROOT}/dist/flex-ui/harmony/${MODE}"

if [[ -z "${OHOS_SDK_HOME:-}" ]]; then
  echo "OHOS_SDK_HOME not set; see CLAUDE.md for HarmonyOS setup"
  exit 2
fi

mkdir -p "$OUT"

# 1) Build C++ shared library (libflexui_napi.so) via CMake + Harmony toolchain.
BUILD_DIR="${OUT}/cmake"
cmake -S "${ROOT}/flex-ui" -B "${BUILD_DIR}" \
  -DFLEXUI_OHOS=ON \
  -DCMAKE_TOOLCHAIN_FILE="${OHOS_SDK_HOME}/native/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_PLATFORM=OHOS \
  -DCMAKE_BUILD_TYPE="$([ "$MODE" = "release" ] && echo Release || echo Debug)"
cmake --build "${BUILD_DIR}" -j --target flexui_napi

# 2) Assemble the HAR using hvigorw.
HARMONY_PROJECT="${ROOT}/playground/harmony"
pushd "${HARMONY_PROJECT}"
"${HARMONY_PROJECT}/hvigorw" assembleHar --mode module \
  -p product=default -p buildMode="$MODE" || true
popd

# 3) Copy artifacts to OUT.
cp "${BUILD_DIR}/platforms/harmony/libflexui_napi.so" "$OUT/" 2>/dev/null || true
find "${ROOT}/flex-ui/platforms/harmony/card/ohos-module/build" \
  -name "*.har" -exec cp {} "$OUT/" \; 2>/dev/null || true

echo "FlexUI Harmony HAR ready: $OUT"
