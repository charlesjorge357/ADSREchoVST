# Apply every JUCE patch in this directory to the JUCE submodule.
#
# For Projucer builds, which do NOT run the CMake auto-apply step. Run this
# once after a fresh clone (or after updating the JUCE submodule) before you
# build in Projucer / Visual Studio:
#
#     powershell -ExecutionPolicy Bypass -File patches\apply-juce-patches.ps1
#
# Idempotent: a patch already present is detected and skipped, so it is safe
# to re-run.
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$juceDir   = Join-Path $scriptDir '..\JUCE'

# Run git quietly and return its exit code. git's --check probes write to
# stderr on the "does not apply" path (which is expected); *>$null swallows
# every stream so PowerShell 5.1 does not wrap that stderr into a terminating
# NativeCommandError. $LASTEXITCODE still reflects git's real exit code.
function Invoke-GitQuiet {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]] $GitArgs)
    & git @GitArgs *>$null
    return $LASTEXITCODE
}

Get-ChildItem -Path $scriptDir -Filter *.patch | ForEach-Object {
    $patch = $_.FullName
    $name  = $_.Name

    if ((Invoke-GitQuiet -C $juceDir apply --reverse --check $patch) -eq 0) {
        Write-Host "already applied: $name"
        return
    }

    if ((Invoke-GitQuiet -C $juceDir apply --check $patch) -eq 0) {
        git -C $juceDir apply $patch
        Write-Host "applied:         $name"
    }
    else {
        Write-Warning "does not apply cleanly (JUCE version changed?): $name"
        Write-Warning "Regenerate it - see patches/README.md."
    }
}
