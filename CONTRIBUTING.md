# Contributing to Opal

Opal is free software under GPL-3.0-or-later. You may fork it, modify it, and
share your changes under the same license.

## Contribute a change

1. Fork the repository and create a focused branch.
2. Edit the canonical sources under `mod/visual-clones` or the supporting
   PowerShell and native companion code.
3. Run `./Test-OpalSuite.ps1 -StaticOnly`.
4. Run `./Build-OpalSuite.ps1` for changes that affect the shipped package.
5. Open a pull request that explains the behavior change and verification.

Public visibility does not grant direct write access to the main branch.
Maintainers explicitly approve collaborators who need that access.
