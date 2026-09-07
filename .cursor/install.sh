#!/usr/bin/env bash
# Opal Cloud Agent environment bootstrap.
#
# Provisions two capabilities:
#   1. PowerShell 7 + PSScriptAnalyzer  -> cross-platform static gate
#      (Test-OpalStatic.ps1).
#   2. clang-20/lld-20/mingw-w64 + Windhawk headers/engine lib -> real Linux
#      cross-build of the product DLL (Build-OpalSuite.Linux.ps1), so C++
#      changes can be compiled and validated on Linux.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bash "$here/install-pwsh.sh"
bash "$here/install-buildchain.sh"
