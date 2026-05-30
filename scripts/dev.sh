#!/bin/bash
set -euo pipefail

# ============================================================================
# scripts/dev.sh
# Developer workflow script for the FlexUI HarmonyOS playground.
#
# Usage:
#   ./scripts/dev.sh <command> [options]
#
# Commands:
#   build   Build the playground hap
#   test    Run the C++ unit tests (host)
#   install Install the hap onto a connected device via hdc
#   start   Launch the app on the connected device via hdc
#   auto    Build + install + start in one step
#
# Options:
#   --mode <debug|release>   Build mode, default: debug
#   --device <id>            HDC device serial (default: first found)
#   -h, --help               Show this help
#
# Environment:
#   DEVECO_HOME   DevEco Studio install root
#                 (default: /Applications/DevEco-Studio.app/Contents)
#
# Signing config must be committed in playground/harmony/build-profile.json5.
#
# Examples:
#   ./scripts/dev.sh auto
#   ./scripts/dev.sh build --mode release
#   ./scripts/dev.sh install --device 127.0.0.1:5555
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=common/_common.sh
source "${SCRIPT_DIR}/common/_common.sh"

# -------------------- Config --------------------
BUILD_MODE="debug"
DEVICE_ID=""

PLAYGROUND_DIR="${AGENUI_ROOT}/playground/harmony"
BUNDLE_NAME="com.withai.aitools"

# -------------------- Retry wrapper --------------------
# hvigorw and hdc occasionally fail on first invocation (daemon cold-start,
# device-connection settle). Retry once before propagating failure.
run_with_retry() {
    local desc="$1"; shift
    if "$@"; then
        return 0
    fi
    warn "${desc} failed, retrying once..."
    "$@"
}

# -------------------- DevEco toolchain --------------------
setup_deveco() {
    DEVECO_HOME="${DEVECO_HOME:-/Applications/DevEco-Studio.app/Contents}"
    if [[ ! -d "$DEVECO_HOME" ]]; then
        error "DevEco Studio not found: ${DEVECO_HOME} (override via DEVECO_HOME)"
    fi
    export DEVECO_SDK_HOME="${DEVECO_HOME}/sdk"
    export PATH="${DEVECO_HOME}/tools/hvigor/bin:${DEVECO_HOME}/tools/ohpm/bin:${DEVECO_HOME}/tools/node/bin:${PATH}"
    command -v hvigorw >/dev/null 2>&1 || error "hvigorw not found; verify DevEco Studio installation"
    command -v hdc    >/dev/null 2>&1 || error "hdc not found; verify DevEco Studio installation"
}

# -------------------- Commands --------------------
cmd_build() {
    setup_deveco
    info "Building playground (mode=${BUILD_MODE})"

    cd "$PLAYGROUND_DIR"

    # Skip ohpm install when oh_modules already exist (avoids network on repeat builds)
    if [[ ! -d "${PLAYGROUND_DIR}/entry/oh_modules" ]]; then
        run_with_retry "ohpm install" ohpm install
    else
        info "oh_modules present, skipping ohpm install"
    fi

    run_with_retry "hvigorw assembleHap" \
        hvigorw assembleHap \
            --parallel \
            -p product=default \
            -p "buildMode=${BUILD_MODE}"

    # Prefer signed HAP; fall back to any .hap
    HAP_PATH="$(find "${PLAYGROUND_DIR}/entry/build" -name "*-signed.hap" 2>/dev/null | head -1)"
    if [[ -z "$HAP_PATH" ]]; then
        HAP_PATH="$(find "${PLAYGROUND_DIR}/entry/build" -name "*.hap" 2>/dev/null | head -1)"
    fi
    if [[ -z "$HAP_PATH" ]]; then
        error "Build failed: no .hap found under ${PLAYGROUND_DIR}/entry/build"
    fi
    export HAP_PATH
    success "Build done: ${HAP_PATH}"
}

cmd_test() {
    info "Running C++ unit tests"
    "${SCRIPT_DIR}/../tests/cpp/ci/run_tests.sh"
}

_pick_device() {
    if [[ -n "$DEVICE_ID" ]]; then
        echo "$DEVICE_ID"
        return
    fi
    local dev
    dev="$(hdc list targets 2>/dev/null | grep -v '^$' | head -1 || true)"
    if [[ -z "$dev" ]]; then
        error "No HDC device found. Connect a device or pass --device <id>"
    fi
    echo "$dev"
}

cmd_install() {
    setup_deveco
    local device
    device="$(_pick_device)"

    # If HAP_PATH not set (called standalone), find the signed build
    if [[ -z "${HAP_PATH:-}" ]]; then
        HAP_PATH="$(find "${PLAYGROUND_DIR}/entry/build" -name "*-signed.hap" 2>/dev/null | head -1 || true)"
        if [[ -z "$HAP_PATH" ]]; then
            HAP_PATH="$(find "${PLAYGROUND_DIR}/entry/build" -name "*.hap" 2>/dev/null | head -1 || true)"
        fi
        if [[ -z "$HAP_PATH" ]]; then
            error "No .hap found. Run './scripts/dev.sh build' first."
        fi
    fi

    info "Installing ${HAP_PATH} → device [${device}]"
    run_with_retry "hdc install" hdc -t "$device" install "$HAP_PATH"
    success "Install done"
}

cmd_start() {
    setup_deveco
    local device
    device="$(_pick_device)"

    info "Starting ${BUNDLE_NAME} on device [${device}]"
    run_with_retry "hdc aa start" hdc -t "$device" shell aa start -a EntryAbility -b "${BUNDLE_NAME}"
    success "App launched"
}

# -------------------- Argument parsing --------------------
show_help() {
    sed -n '6,27p' "$0" | sed 's/^# \?//'
    exit 0
}

COMMAND="${1:-}"
shift || true

while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode)     BUILD_MODE="$2"; shift 2 ;;
        --device)   DEVICE_ID="$2"; shift 2 ;;
        -h|--help)  show_help ;;
        *) error "Unknown argument: $1" ;;
    esac
done

case "$BUILD_MODE" in
    debug|release) ;;
    *) error "Invalid --mode: ${BUILD_MODE}" ;;
esac

case "$COMMAND" in
    build)   HAP_PATH=""; cmd_build ;;
    test)    cmd_test ;;
    install) cmd_install ;;
    start)   cmd_start ;;
    auto)
        HAP_PATH=""
        cmd_build
        cmd_install
        cmd_start
        ;;
    ""|--help|-h) show_help ;;
    *) error "Unknown command: ${COMMAND}. Run with --help." ;;
esac
