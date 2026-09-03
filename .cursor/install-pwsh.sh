#!/usr/bin/env bash
# Idempotent PowerShell 7 installer for the Opal Cloud Agent environment.
#
# Opal is a Windows/Windhawk product; its build, install, and live tests need
# Windows. On a Linux Cloud Agent the usable development surface is PowerShell:
# editing the .ps1/.psm1 tooling, assembling the single-file mod, and running
# the cross-platform static validations. This script provisions that runtime.
set -euo pipefail

PWSH_VERSION="7.6.5"
INSTALL_DIR="/opt/microsoft/powershell/7"
LINK="/usr/local/bin/pwsh"

if command -v pwsh >/dev/null 2>&1; then
    echo "pwsh already installed: $(pwsh --version)"
    exit 0
fi

arch="$(uname -m)"
case "$arch" in
    x86_64) pkg_arch="x64" ;;
    aarch64|arm64) pkg_arch="arm64" ;;
    *) echo "Unsupported architecture: $arch" >&2; exit 1 ;;
esac

tarball="powershell-${PWSH_VERSION}-linux-${pkg_arch}.tar.gz"
url="https://github.com/PowerShell/PowerShell/releases/download/v${PWSH_VERSION}/${tarball}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "Downloading PowerShell ${PWSH_VERSION} (${pkg_arch})..."
curl -fsSL -o "${tmp}/${tarball}" "$url"

echo "Installing to ${INSTALL_DIR}..."
sudo mkdir -p "$INSTALL_DIR"
sudo tar zxf "${tmp}/${tarball}" -C "$INSTALL_DIR"
sudo chmod +x "${INSTALL_DIR}/pwsh"
sudo ln -sf "${INSTALL_DIR}/pwsh" "$LINK"

echo "Installed: $(pwsh --version)"
