# Opal 4.4 ABBA cost receipt

The published CPU/RAM table in README is still the 4.0 ABBA run
(`package-cost-opal-4-unified-valid-settled-abba2.json`).

Re-measure on this PC after installing the current Opal DLL (Explorer must load
the rebuilt `local@opal` 4.4.0 package). Requires administrator for Windhawk
HKLM scenario switching:

```powershell
pwsh -NoProfile -File .\Measure-OpalPackageCost.ps1 `
  -Label opal-4.4-20260902-abba `
  -AbbaRounds 2 -RandomSeed 3 `
  -Scenario @('Stock','FullSuite') `
  -Samples 2 -SampleSeconds 8 -SettleSeconds 6
```

Write the JSON under `measurements\` and cite it from README. Do not invent
numbers. This command restarts Explorer between legs on purpose.
