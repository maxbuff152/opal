# Shared sampling helpers. Working sets stay per process: shared pages make
# their sum unsuitable as a package physical-memory total.
function Get-OpalMeasuredProcessSnapshot {
    param([Parameter(Mandatory)][int]$SessionId)
    @(Get-Process -Name explorer,Maxwell.Shell.Core,windhawk -ErrorAction SilentlyContinue |
        Where-Object SessionId -eq $SessionId | ForEach-Object {
            $_.Refresh()
            [pscustomobject]@{
                Name = $_.ProcessName
                Pid = $_.Id
                StartedAtUtc = $_.StartTime.ToUniversalTime().ToString('o')
                CpuSeconds = $_.CPU
                PrivateMB = $_.PrivateMemorySize64 / 1MB
                WorkingSetMB = $_.WorkingSet64 / 1MB
            }
        })
}

function Compare-OpalMeasuredProcessSnapshot {
    param(
        [Parameter(Mandatory)][object[]]$Before,
        [Parameter(Mandatory)][object[]]$After,
        [Parameter(Mandatory)][double]$ElapsedSeconds
    )
    if (-not [double]::IsFinite($ElapsedSeconds) -or $ElapsedSeconds -le 0) {
        throw 'Invalid measured-process sample duration.'
    }
    if ($Before.Count -eq 0 -or $Before.Count -ne $After.Count) {
        throw 'Measured process membership changed during the sample.'
    }
    $rows = @(foreach ($last in $After) {
        $first = @($Before | Where-Object { $_.Pid -eq $last.Pid })
        if ($first.Count -ne 1 -or
            @($After | Where-Object { $_.Pid -eq $last.Pid }).Count -ne 1 -or
            $first[0].StartedAtUtc -ne $last.StartedAtUtc -or
            $first[0].Name -ne $last.Name) {
            throw 'Measured process identity changed during the sample.'
        }
        foreach ($value in @($first[0].CpuSeconds, $last.CpuSeconds, $last.PrivateMB, $last.WorkingSetMB)) {
            if ($null -eq $value -or $value -is [bool] -or $value -is [string] -or
                -not [double]::IsFinite([double]$value) -or [double]$value -lt 0) {
                throw 'Invalid measured-process counter.'
            }
        }
        $cpu = $last.CpuSeconds - $first[0].CpuSeconds
        if ($cpu -lt 0) { throw 'Measured process CPU counter moved backwards.' }
        [pscustomobject]@{
            Name = $last.Name; Pid = $last.Pid; StartedAtUtc = $last.StartedAtUtc
            CpuSeconds = $cpu
            CpuPercentOneCore = $cpu / $ElapsedSeconds * 100
            PrivateMB = $last.PrivateMB
            WorkingSetMB = $last.WorkingSetMB
        }
    })
    $totalCpu = ($rows | Measure-Object -Property CpuSeconds -Sum).Sum
    [pscustomobject]@{
        Scope = 'All interactive-session Explorer, Maxwell.Shell.Core and Windhawk processes; excludes other shell hosts and services.'
        CpuSeconds = $totalCpu
        CpuPercentOneCore = $totalCpu / $ElapsedSeconds * 100
        PrivateMB = ($rows | Measure-Object -Property PrivateMB -Sum).Sum
        Processes = $rows
        Limitation = 'Observed process-set cost, not attribution of all machine work to Opal. Per-process working sets are not summed.'
    }
}
