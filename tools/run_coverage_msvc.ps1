# Windows convenience launcher for tools/coverage.py.
#
# Imports the MSVC x64 developer environment, puts jom and OpenCppCoverage on
# PATH and runs the cross-platform coverage driver. Visual Studio and Qt are
# auto-detected; override with -QtPrefix / -Generator or pass extra CMake args
# after a literal '--'.
#
#   powershell -File tools\run_coverage_msvc.ps1
#   powershell -File tools\run_coverage_msvc.ps1 -QtPrefix C:/Qt/6.11.0/msvc2022_64
[CmdletBinding()]
param(
    [string]$QtPrefix,
    [string]$Generator = "NMake Makefiles JOM",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraCMakeArgs
)
$ErrorActionPreference = "Stop"

# Repo root is the parent of this script's tools/ directory - never hard-coded.
$repoRoot = Split-Path $PSScriptRoot -Parent

# Locate Visual Studio via vswhere and import its x64 dev environment.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found; is Visual Studio installed?" }
$vsPath = & $vswhere -latest -products * -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found under $vsPath" }
cmd /c "`"$vcvars`" && set" | ForEach-Object {
    if ($_ -match '^(.*?)=(.*)$') { Set-Item -Path "Env:$($matches[1])" -Value $matches[2] }
}

# Auto-detect a Qt MSVC kit if none was supplied (latest version wins).
if (-not $QtPrefix) {
    $QtPrefix = Get-ChildItem "C:\Qt" -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+' } |
        Sort-Object Name -Descending |
        ForEach-Object { Get-ChildItem $_.FullName -Directory -Filter "msvc*_64" -ErrorAction SilentlyContinue } |
        Select-Object -First 1 -ExpandProperty FullName
}
if ($QtPrefix) {
    # Pass Qt via the environment so the path survives PowerShell arg parsing.
    $env:CMAKE_PREFIX_PATH = $QtPrefix
    # The test executables need the Qt runtime DLLs on PATH to even start.
    if (Test-Path "$QtPrefix\bin") { $env:Path = "$QtPrefix\bin;$env:Path" }
    Write-Host "Using Qt: $QtPrefix"
}

# Put jom (NMake JOM generator), OpenCppCoverage and the OpenSSL runtime the
# app links against on PATH if present.
foreach ($extra in @("C:\Qt\Tools\QtCreator\bin\jom",
                     "C:\Program Files\OpenCppCoverage",
                     "C:\Qt\Tools\OpenSSLv3\Win_x64\bin")) {
    if (Test-Path $extra) { $env:Path = "$extra;$env:Path" }
}

Set-Location $repoRoot
$cmakeArgs = @("-G", $Generator) + $ExtraCMakeArgs
python tools/coverage.py -- @cmakeArgs
exit $LASTEXITCODE
