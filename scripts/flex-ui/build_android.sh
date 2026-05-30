#!/usr/bin/env bash
# FlexUI Android compile gate.
# Builds all cross-platform C++ targets for Android arm64-v8a.
# No JNI runtime, no Kotlin AAR — compile only for PoC gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ABI="${ABI:-arm64-v8a}"
OUT="${ROOT}/dist/flex-ui/android"
BUILD_DIR="${OUT}/cmake/${ABI}"

if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
  echo "ANDROID_NDK_HOME not set; set it to your NDK root (e.g. \$ANDROID_HOME/ndk/27.0.12077973)"
  exit 2
fi

mkdir -p "$BUILD_DIR"
cmake -S "${ROOT}/flex-ui" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=${ABI} \
  -DANDROID_PLATFORM=android-26 \
  -DFLEXUI_OHOS=OFF \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" -j --target flexui_common flexui_core_js_engine \
   flexui_core_vdom flexui_core_reconciler flexui_core_layout \
   flexui_core_bridge flexui_core_scope_manager flexui_core_commit_pipeline \
   flexui_core_plugin_host flexui_core_card_controller flexui_components flexui_api

echo "FlexUI Android libs built: ${BUILD_DIR}"
