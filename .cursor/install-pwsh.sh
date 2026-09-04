#!/usr/bin/env bash
# Idempotent PowerShell 7 installer for the Opal Cloud Agent environment.
#
# Opal is a Windows/Windhawk product; its build, install, and live tests need
# Windows. On a Linux Cloud Agent the usable development surface is PowerShell:
# editing the .ps1/.psm1 tooling, assembling the single-file mod, and running
# the cross-platform static gate (Test-OpalStatic.ps1). This script provisions
# that runtime plus PSScriptAnalyzer for the gate's lint step.
set -euo pipefail

PWSH_VERSION="7.6.5"
INSTALL_DIR="/opt/microsoft/powershell/7"
LINK="/usr/local/bin/pwsh"

# Official SHA256 checksums for the pinned PowerShell release tarballs.
declare -A PWSH_SHA256=(
    [x64]="b34ab3b19acac1d3d4d0d3cfdb02acf62f457b0b6a962ff008132033f7566844"
    [arm64]="ed4084f215d8bce2edd23aa7cb1f1e7b0818e41363a635a22065d2701b6141df"
)

ensure_pwsh() {
    if command -v pwsh >/dev/null 2>&1; then
        echo "pwsh already installed: $(pwsh --version)"
        return 0
    fi

    local arch pkg_arch
    arch="$(uname -m)"
    case "$arch" in
        x86_64) pkg_arch="x64" ;;
        aarch64 | arm64) pkg_arch="arm64" ;;
        *) echo "Unsupported architecture: $arch" >&2; exit 1 ;;
    esac

    local expected="${PWSH_SHA256[$pkg_arch]:-}"
    if [[ -z "$expected" ]]; then
        echo "No pinned checksum for architecture: $pkg_arch" >&2
        exit 1
    fi

    local tarball="powershell-${PWSH_VERSION}-linux-${pkg_arch}.tar.gz"
    local url="https://github.com/PowerShell/PowerShell/releases/download/v${PWSH_VERSION}/${tarball}"
    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN

    echo "Downloading PowerShell ${PWSH_VERSION} (${pkg_arch})..."
    curl -fsSL -o "${tmp}/${tarball}" "$url"

    echo "Verifying checksum..."
    echo "${expected}  ${tmp}/${tarball}" | sha256sum -c -

    echo "Installing to ${INSTALL_DIR}..."
    sudo mkdir -p "$INSTALL_DIR"
    sudo tar zxf "${tmp}/${tarball}" -C "$INSTALL_DIR"
    sudo chmod +x "${INSTALL_DIR}/pwsh"
    sudo ln -sf "${INSTALL_DIR}/pwsh" "$LINK"

    echo "Installed: $(pwsh --version)"
}

ensure_analyzer() {
    # PSScriptAnalyzer powers the lint step of Test-OpalStatic.ps1. Install it
    # idempotently for the current user so the gate runs fully in the VM.
    pwsh -NoProfile -Command '
        if (-not (Get-Module -ListAvailable -Name PSScriptAnalyzer)) {
            Set-PSRepository -Name PSGallery -InstallationPolicy Trusted
            Install-Module PSScriptAnalyzer -Scope CurrentUser -Force
            Write-Host ("Installed PSScriptAnalyzer {0}" -f (Get-Module -ListAvailable PSScriptAnalyzer | Select-Object -First 1).Version)
        } else {
            Write-Host ("PSScriptAnalyzer already available: {0}" -f (Get-Module -ListAvailable PSScriptAnalyzer | Select-Object -First 1).Version)
        }'
}

ensure_pwsh
ensure_analyzer
