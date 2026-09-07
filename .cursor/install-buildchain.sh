#!/usr/bin/env bash
# Provision a Linux cross-build toolchain for the Opal Windhawk mod.
#
# Opal ships as a Windows DLL that Windhawk builds with its bundled
# clang(++)/lld (LLVM-MinGW, clang 20) plus cppwinrt-generated WinRT headers,
# windhawk_api.h/windhawk_utils.h, and the engine import lib windhawk.lib.
#
# We reproduce that on Linux by pairing a native clang-20 + lld-20 + mingw-w64
# with Windhawk's own headers and windhawk.lib, extracted from the official
# offline installer. Those headers are Microsoft/Windhawk licensed and large,
# so they are fetched into a git-ignored cache rather than committed.
#
# Idempotent: skips work that is already present.
set -euo pipefail

WINDHAWK_VERSION="1.7.3"
INSTALLER_SHA256="4d93016570f982326eebdfc9068e924ff21f6448534ea32672bb4c1d52a8193b"
INSTALLER_URL="https://github.com/ramensoftware/windhawk/releases/download/v${WINDHAWK_VERSION}/windhawk_setup_offline.exe"

TOOLCHAIN_DIR="${OPAL_WINDHAWK_TOOLCHAIN:-$HOME/.cache/opal-windhawk}"
STAMP="${TOOLCHAIN_DIR}/.windhawk-${WINDHAWK_VERSION}.stamp"

install_apt_tools() {
    local missing=()
    command -v clang-20 >/dev/null 2>&1 || missing+=("clang-20")
    command -v ld.lld-20 >/dev/null 2>&1 || missing+=("lld-20")
    command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || missing+=("mingw-w64")
    command -v 7z >/dev/null 2>&1 || missing+=("p7zip-full")
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo "Installing apt packages: ${missing[*]}"
        sudo apt-get update -qq
        sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "${missing[@]}"
    else
        echo "Cross-build apt packages already present."
    fi
}

fetch_windhawk_headers() {
    if [[ -f "$STAMP" ]] \
        && [[ -f "${TOOLCHAIN_DIR}/Compiler/include/windhawk_api.h" ]] \
        && [[ -f "${TOOLCHAIN_DIR}/Engine/64/windhawk.lib" ]]; then
        echo "Windhawk toolchain already staged at ${TOOLCHAIN_DIR}."
        return 0
    fi

    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN

    echo "Downloading Windhawk ${WINDHAWK_VERSION} offline installer..."
    curl -fsSL -o "${tmp}/wh.exe" "$INSTALLER_URL"

    echo "Verifying installer checksum..."
    echo "${INSTALLER_SHA256}  ${tmp}/wh.exe" | sha256sum -c -

    echo "Extracting compiler headers and engine import library..."
    7z x -y -o"${tmp}/x" "${tmp}/wh.exe" >/dev/null

    rm -rf "${TOOLCHAIN_DIR}/Compiler" "${TOOLCHAIN_DIR}/Engine"
    mkdir -p "${TOOLCHAIN_DIR}/Engine/64"
    cp -r "${tmp}/x/Compiler" "${TOOLCHAIN_DIR}/Compiler"
    cp "${tmp}/x/Engine/"*/64/windhawk.lib "${TOOLCHAIN_DIR}/Engine/64/windhawk.lib"

    touch "$STAMP"
    echo "Staged Windhawk toolchain to ${TOOLCHAIN_DIR}."
}

install_apt_tools
fetch_windhawk_headers
echo "Opal Linux build toolchain ready:"
echo "  clang: $(clang-20 --version | head -1)"
echo "  toolchain dir: ${TOOLCHAIN_DIR}"
