@{
    # Shared PSScriptAnalyzer configuration for Opal's cross-platform static gate
    # (Test-OpalStatic.ps1) and CI. Reports every severity; the gate itself
    # decides which severities block (Error by default).
    IncludeDefaultRules = $true
    Severity            = @('Error', 'Warning', 'Information')

    # Rules excluded because the behavior is deliberate in this codebase:
    ExcludeRules        = @(
        # The assemble/measure/installer scripts use Write-Host intentionally
        # for human-facing console output.
        'PSAvoidUsingWriteHost',
        # File encodings (UTF-8 without BOM, etc.) are chosen on purpose for the
        # Windhawk single-file source and receipts.
        'PSUseBOMForUnicodeEncodedFile',
        # Internal installer/rollback helpers (Set-RegistryValue, Restart-*,
        # Start-*) deliberately do not implement ShouldProcess; -WhatIf support
        # is provided at the top-level script boundary instead.
        'PSUseShouldProcessForStateChangingFunctions',
        # A few internal helpers use plural nouns (Clear-*RestartMarkers) that
        # read naturally and are only called within their own scripts.
        'PSUseSingularNouns'
    )
}
