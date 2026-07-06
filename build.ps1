param(
    [switch]$Tests,
    [switch]$Install,
    [string]$InstallPrefix = "",
    [switch]$Help
)

function Show-Usage {
    Write-Host "Usage: .\build.ps1 [-Tests] [-Install] [-InstallPrefix <PREFIX>]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Tests                   Build and run the test suite"
    Write-Host "  -Install                 Install after building"
    Write-Host "  -InstallPrefix <PREFIX>  Install after building into PREFIX"
}

if ($Help) {
    Show-Usage
    exit 0
}

# A prefix implies installation, matching build.sh's --install=PREFIX.
if ($InstallPrefix) {
    $Install = $true
}

# build.sh fixes the configuration to Release; mirror that here.
$BuildType = "Release"

$SourceDir        = Join-Path $PSScriptRoot "src"
$SourceCMakeLists = Join-Path $SourceDir "CMakeLists.txt"
$QtCMake          = Join-Path $SourceDir "cmake\qt.cmake"

$MinimumCMakeMatch = Select-String -Path $SourceCMakeLists -Pattern '^\s*cmake_minimum_required\s*\(\s*VERSION\s+([0-9]+(\.[0-9]+){1,3})' | Select-Object -First 1
$AppVersionMatch   = Select-String -Path $SourceCMakeLists -Pattern '^\s*VERSION\s+(\d+)\.(\d+)\.(\d+)' | Select-Object -First 1
$MinimumQtMatch    = Select-String -Path $QtCMake -Pattern '^\s*find_package\s*\(\s*Qt6\s+([0-9]+(\.[0-9]+){1,3})' | Select-Object -First 1

if (-not $MinimumCMakeMatch) {
    Write-Error "Can't detect minimum CMake version from $SourceCMakeLists"
    exit 1
}
if (-not $AppVersionMatch) {
    Write-Error "Can't detect application version from $SourceCMakeLists"
    exit 1
}

$MinimumCMakeVersion = [version]$MinimumCMakeMatch.Matches[0].Groups[1].Value
$MinimumQtVersion    = if ($MinimumQtMatch) { [version]$MinimumQtMatch.Matches[0].Groups[1].Value } else { [version]"6.6" }

$ProductName   = "Open UaExplorer"
$DefaultPrefix = "C:/Program Files/$ProductName"

# Keep the default "Continue" policy: under "Stop", Windows PowerShell 5.1 turns a
# native command's redirected stderr (e.g. python "2>$null" emitting a harmless
# RequestsDependencyWarning) into a terminating NativeCommandError. Every external
# tool below is checked explicitly via $LASTEXITCODE / try-catch instead.
$ErrorActionPreference = "Continue"

Write-Host ""
Write-Host "===================================="
Write-Host "=== Open UaExplorer Build Script ==="
Write-Host "===================================="
Write-Host "Qt Version: auto (newest >= $MinimumQtVersion)"
Write-Host "Compiler:   msvc (auto)"
Write-Host "Generator:  Ninja"
Write-Host "BuildType:  $BuildType"
Write-Host "Python:     auto (latest if missing)"
Write-Host "Tests:      $(if ($Tests) { 'ON' } else { 'OFF' })"
Write-Host "Install:    $(if ($Install) { 'ON' } else { 'OFF' })"
Write-Host "===================================="

# Check Windows version and architecture
Write-Host ""
Write-Host "Checking system requirements..."
$os = Get-CimInstance -ClassName Win32_OperatingSystem
$windowsVersion = [System.Environment]::OSVersion.Version
$architecture = $env:PROCESSOR_ARCHITECTURE

Write-Host "Windows Version: $($os.Caption)"
Write-Host "Build Number: $($os.BuildNumber)"
Write-Host "Architecture: $architecture"

if ($windowsVersion.Major -lt 10) {
    Write-Error "This script requires Windows 10 or later."
    exit 1
}

if ($architecture -ne "AMD64") {
    Write-Error "64-bit build requested but running on a non-AMD64 system. Architecture: $architecture"
    exit 1
}

# Refresh PATH from the machine/user registry so tools installed during this run
# (winget/choco place them there) become visible without opening a new shell.
function Update-SessionPath {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path", "User")
}

# Pick an available package manager, preferring winget (ships with Windows 11).
function Get-PackageManager {
    if (Get-Command winget -ErrorAction SilentlyContinue) { return "winget" }
    if (Get-Command choco  -ErrorAction SilentlyContinue) { return "choco" }
    return $null
}

# Resolve the newest winget package id for Python 3. Ids are per minor version
# (e.g. Python.Python.3.13), so we enumerate them to install the latest without
# hard-coding a version number.
function Get-WingetPythonId {
    $search = winget search --id "Python.Python.3." --source winget `
        --accept-source-agreements --disable-interactivity 2>$null
    $ids = [regex]::Matches(($search -join "`n"), 'Python\.Python\.3\.\d+') |
        ForEach-Object { $_.Value } | Sort-Object -Unique
    if (-not $ids) { return $null }
    return $ids |
        Sort-Object { [int]($_ -replace '^Python\.Python\.3\.', '') } -Descending |
        Select-Object -First 1
}

# Function to install the latest Python
function Install-Python {
    switch (Get-PackageManager) {
        "winget" {
            $id = Get-WingetPythonId
            if (-not $id) {
                Write-Host "Could not resolve a Python package id from winget."
                return $false
            }
            Write-Host "Installing $id via winget..."
            winget install --id $id -e --source winget `
                --accept-source-agreements --accept-package-agreements --disable-interactivity
        }
        "choco" {
            Write-Host "Installing python via Chocolatey..."
            choco install python -y
        }
        default {
            Write-Host "Neither winget nor Chocolatey is available. Please install Python manually from: https://www.python.org/downloads/"
            return $false
        }
    }

    Update-SessionPath
    Start-Sleep -Seconds 2
    if (Get-Command python -ErrorAction SilentlyContinue) {
        Write-Host "Python version: $(python --version 2>&1)"
        return $true
    }

    Write-Host "Python installation completed, but python is not yet on PATH."
    Write-Host "Please restart your terminal and run the script again."
    return $false
}

# Function to install CMake
function Install-CMakeSystem {
    switch (Get-PackageManager) {
        "winget" {
            Write-Host "Installing Kitware.CMake via winget..."
            winget install --id Kitware.CMake -e --source winget `
                --accept-source-agreements --accept-package-agreements --disable-interactivity
        }
        "choco" {
            Write-Host "Installing cmake via Chocolatey..."
            choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'
        }
        default {
            Write-Host "Neither winget nor Chocolatey is available. Please install CMake manually from: https://cmake.org/download/"
            return $false
        }
    }

    Update-SessionPath
    Start-Sleep -Seconds 2
    if (Get-Command cmake -ErrorAction SilentlyContinue) {
        Write-Host "CMake version: $(cmake --version | Select-Object -First 1)"
        return $true
    }

    Write-Host "CMake installation completed, but cmake is not yet on PATH."
    Write-Host "Please restart your terminal and run the script again."
    return $false
}

# Function to get CMake version
function Get-CMakeVersion {
    param(
        [string]$CMakePath
    )

    try {
        $versionOutput = & $CMakePath --version 2>&1
        $versionLine = $versionOutput | Select-Object -First 1
        if ($versionLine -match 'cmake version\s+([0-9]+(\.[0-9]+){1,3})') {
            return [version]$Matches[1]
        }
    }
    catch {
        return $null
    }

    return $null
}

# Look for a Qt already installed under C:\Qt (official installer / aqt layout),
# mirroring build.sh which prefers an existing Qt before falling back to aqt.
# Returns the newest Qt 6 >= minimum whose MSVC build has qmake and Qt Charts
# (the one non-base module this project needs), or $null.
function Get-InstalledQt {
    param(
        [version]$Minimum
    )

    $qtRoot = "C:\Qt"
    if (-not (Test-Path $qtRoot)) {
        return $null
    }

    $preferredCompilers = @("msvc2022_64", "msvc2019_64")

    $candidates = Get-ChildItem $qtRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^6\.\d+\.\d+$' -and [version]$_.Name -ge $Minimum } |
        Sort-Object { [version]$_.Name } -Descending

    foreach ($candidate in $candidates) {
        foreach ($compiler in $preferredCompilers) {
            $compilerDir = Join-Path $candidate.FullName $compiler
            $qmake       = Join-Path $compilerDir "bin\qmake.exe"
            $chartsCfg   = Join-Path $compilerDir "lib\cmake\Qt6Charts\Qt6ChartsConfig.cmake"
            if ((Test-Path $qmake) -and (Test-Path $chartsCfg)) {
                return @{
                    Version  = $candidate.Name
                    Arch     = "win64_$compiler"
                    Compiler = $compiler
                }
            }
        }
    }

    return $null
}

# Ensure aqtinstall is importable. Called lazily, only when we actually need to
# download Qt or a Qt tool, so a fully provisioned machine never invokes aqt.
function Initialize-Aqt {
    if ($script:AqtReady) {
        return
    }
    Write-Host ""
    Write-Host "Checking for aqtinstall..."
    python -c "import aqt" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Installing aqtinstall..."
        python -m pip install --upgrade pip
        python -m pip install aqtinstall
        python -c "import aqt" 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to install aqtinstall. Please install manually: pip install aqtinstall"
            exit 1
        }
        Write-Host "aqtinstall installed successfully."
    }
    $script:AqtReady = $true
}

# Run an "aqt list-*" query with retries. aqt reads Updates.xml from
# download.qt.io, which fails intermittently; a transient failure must not be
# mistaken for "not available". Returns the raw output, or $null if it keeps
# failing after all attempts.
function Invoke-AqtList {
    param(
        [string[]]$Arguments,
        [int]$Retries = 3
    )
    for ($attempt = 1; $attempt -le $Retries; $attempt++) {
        $output = & python -m aqt @Arguments 2>$null
        if ($LASTEXITCODE -eq 0 -and $output) {
            return $output
        }
        if ($attempt -lt $Retries) {
            Start-Sleep -Seconds 2
        }
    }
    return $null
}

# Determine the newest Qt 6 release aqtinstall can provide that satisfies the
# project minimum, mirroring build.sh: list available versions, skip anything
# without a final qtbase release tag (aqt also lists betas/RCs) and require the
# MSVC desktop arch to be installable.
function Get-QtVersion {
    param(
        [version]$Minimum
    )

    $listOutput = Invoke-AqtList @("list-qt", "windows", "desktop")
    if (-not $listOutput) {
        Write-Error "Failed to query available Qt versions via aqtinstall (network error reaching download.qt.io)."
        exit 1
    }

    $versions = ($listOutput -split '\s+') |
        Where-Object { $_ -match '^6\.\d+\.\d+$' } |
        Sort-Object { [version]$_ } -Unique -Descending

    $gitAvailable = [bool](Get-Command git -ErrorAction SilentlyContinue)
    if (-not $gitAvailable) {
        Write-Warning "git not found; skipping beta/RC filtering of Qt releases."
    }

    $preferredArchs = @("win64_msvc2022_64", "win64_msvc2019_64")

    foreach ($version in $versions) {
        if ([version]$version -lt $Minimum) {
            continue
        }

        if ($gitAvailable) {
            $null = git ls-remote --exit-code --tags https://code.qt.io/qt/qtbase.git "refs/tags/v$version" 2>$null
            if ($LASTEXITCODE -ne 0) {
                Write-Host "Skipping Qt $version (no final release tag; beta/RC)."
                continue
            }
        }

        $archOutput = Invoke-AqtList @("list-qt", "windows", "desktop", "--arch", $version)
        if (-not $archOutput) {
            Write-Warning "Could not query architectures for Qt $version (network error reaching download.qt.io); skipping. Re-run once your connection is stable."
            continue
        }
        $archs = $archOutput -split '\s+'
        foreach ($candidate in $preferredArchs) {
            if ($archs -contains $candidate) {
                return @{
                    Version  = $version
                    Arch     = $candidate
                    Compiler = ($candidate -replace '^win64_', '')
                }
            }
        }
        Write-Host "Skipping Qt $version (no MSVC desktop build available)."
    }

    Write-Error "Could not find an installable Qt 6 >= $Minimum with the MSVC desktop toolchain."
    exit 1
}

# Locate a Visual Studio installation that ships the x64 C++ toolset
function Get-VisualStudioPath {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null
        if ($vsPath) {
            return $vsPath
        }
    }

    $candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools"
    )
    foreach ($path in $candidates) {
        if (Test-Path (Join-Path $path "Common7\Tools\VsDevCmd.bat")) {
            return $path
        }
    }
    return $null
}

# Import the MSVC developer environment into the current PowerShell session.
# The Ninja generator does not discover the compiler on its own, so cl.exe and
# the Windows SDK must be on PATH before CMake is invoked.
function Import-VisualStudioEnvironment {
    param(
        [string]$VsPath
    )

    $vsDevCmd = Join-Path $VsPath "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path $vsDevCmd)) {
        Write-Error "VsDevCmd.bat not found at: $vsDevCmd"
        exit 1
    }

    Write-Host "Importing Visual Studio environment from: $VsPath"
    $envOutput = & "${env:COMSPEC}" /s /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && set"
    foreach ($line in $envOutput) {
        if ($line -match '^([^=]+)=(.*)$') {
            Set-Item -Path "Env:$($Matches[1])" -Value $Matches[2]
        }
    }
}

# Function to install Visual Studio Build Tools (VS2022)
function Install-VisualStudioBuildTools {
    $override = "--quiet --wait --norestart " +
        "--add Microsoft.VisualStudio.Workload.VCTools " +
        "--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 " +
        "--add Microsoft.VisualStudio.Component.Windows10SDK.19041"

    switch (Get-PackageManager) {
        "winget" {
            Write-Host "Installing Microsoft.VisualStudio.2022.BuildTools via winget (this may take a while)..."
            winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget `
                --accept-source-agreements --accept-package-agreements --disable-interactivity `
                --override $override
        }
        "choco" {
            Write-Host "Installing Visual Studio 2022 Build Tools via Chocolatey (this may take a while)..."
            choco install visualstudio2022buildtools -y
            choco install visualstudio2022-workload-vctools -y
        }
        default {
            Write-Host "Neither winget nor Chocolatey is available. Please install Visual Studio Build Tools manually from: https://visualstudio.microsoft.com/downloads/"
            return $false
        }
    }

    Update-SessionPath
    return $true
}

# Check if Python is available
Write-Host ""
Write-Host "Checking for Python..."
try {
    $pythonVersion = & python --version 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Python not found"
    }

    Write-Host "Python found: $pythonVersion"

    if ($pythonVersion -match "Python (\d+)\.(\d+)") {
        $major = [int]$matches[1]
        $minor = [int]$matches[2]

        if ($major -lt 3 -or ($major -eq 3 -and $minor -lt 9)) {
            Write-Host "Python version $major.$minor is too old. aqtinstall requires Python 3.9+."
            $choice = Read-Host "Do you want to install a newer Python version? (y/n)"
            if ($choice -eq 'y' -or $choice -eq 'Y') {
                $success = Install-Python
                if (-not $success) {
                    Write-Error "Python installation failed."
                    exit 1
                }
            } else {
                Write-Error "Please install Python 3.9 or newer manually."
                exit 1
            }
        }
    }
}
catch {
    Write-Host "Python not found."
    $choice = Read-Host "Do you want to download and install the latest Python? (y/n)"
    if ($choice -eq 'y' -or $choice -eq 'Y') {
        $success = Install-Python
        if (-not $success) {
            Write-Error "Python installation failed or requires terminal restart."
            exit 1
        }
    } else {
        Write-Host "Please install Python manually from: https://www.python.org/downloads/"
        Write-Host "Make sure to check 'Add Python to PATH' during installation."
        exit 1
    }
}

# Check if MSVC / Visual Studio is available
Write-Host ""
Write-Host "Checking for Visual Studio C++ toolset..."
$vsPath = Get-VisualStudioPath
if (-not $vsPath) {
    Write-Host "Visual Studio C++ toolset not found."
    $choice = Read-Host "Do you want to install Visual Studio Build Tools? (y/n)"
    if ($choice -eq 'y' -or $choice -eq 'Y') {
        $success = Install-VisualStudioBuildTools
        if ($success) {
            $vsPath = Get-VisualStudioPath
            if (-not $vsPath) {
                Write-Error "Visual Studio C++ toolset still not found after installation. Please restart your terminal and run the script again."
                exit 1
            }
        } else {
            Write-Error "Visual Studio Build Tools installation failed. Please install manually."
            exit 1
        }
    } else {
        Write-Host "Please install Visual Studio 2022 with C++ support from: https://visualstudio.microsoft.com/downloads/"
        exit 1
    }
}
Write-Host "Visual Studio found: $vsPath"

# Determine the Qt to build against: prefer an already-installed Qt (build.sh
# does the same), and only query/install via aqtinstall when none is present.
Write-Host ""
Write-Host "Detecting Qt..."
$qtInfo = Get-InstalledQt -Minimum $MinimumQtVersion
if ($qtInfo) {
    Write-Host "Using installed Qt $($qtInfo.Version) ($($qtInfo.Compiler)) from C:\Qt"
} else {
    Write-Host "No suitable Qt found under C:\Qt; querying aqtinstall for the newest release..."
    Initialize-Aqt
    $qtInfo = Get-QtVersion -Minimum $MinimumQtVersion
    Write-Host "Selected Qt $($qtInfo.Version) ($($qtInfo.Compiler))"
}
$QtVersion = $qtInfo.Version
$QtArch    = $qtInfo.Arch
$Compiler  = $qtInfo.Compiler

# Check if CMake is available
Write-Host ""
Write-Host "Checking for CMake..."
$cmakePath = $null
$cmakeNeedsInstall = $false
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$qtBundledCMake = "C:\Qt\Tools\CMake_64\bin\cmake.exe"
if ($cmakeCommand) {
    $cmakePath = $cmakeCommand.Source
    Write-Host "Found system CMake: $cmakePath"
} elseif (Test-Path $qtBundledCMake) {
    $cmakePath = $qtBundledCMake
    Write-Host "Found Qt-bundled CMake: $cmakePath"
}
if ($cmakePath) {
    $cmakeVersion = Get-CMakeVersion -CMakePath $cmakePath
    if (-not $cmakeVersion) {
        Write-Host "Failed to detect CMake version."
        $cmakeNeedsInstall = $true
    }
    elseif ($cmakeVersion -lt $MinimumCMakeVersion) {
        Write-Host "Found CMake version $cmakeVersion, but version $MinimumCMakeVersion or newer is required."
        $cmakeNeedsInstall = $true
    }
    else {
        Write-Host "CMake version $cmakeVersion is supported."
    }
} else {
    Write-Host "CMake not found (checked PATH and C:\Qt\Tools\CMake_64)."
    $cmakeNeedsInstall = $true
}

if ($cmakeNeedsInstall) {
    $choice = Read-Host "Do you want to install/update CMake system-wide? (y/n)"
    if ($choice -eq 'y' -or $choice -eq 'Y') {
        $success = Install-CMakeSystem
        if ($success) {
            $cmakePath = (Get-Command cmake).Source
            $cmakeVersion = Get-CMakeVersion -CMakePath $cmakePath
            if (-not $cmakeVersion -or $cmakeVersion -lt $MinimumCMakeVersion) {
                Write-Error "Installed CMake version is lower than required $MinimumCMakeVersion."
                exit 1
            }
            Write-Host "CMake installed: $cmakePath"
        } else {
            Write-Error "CMake installation failed. Please install manually from: https://cmake.org/download/"
            exit 1
        }
    } else {
        Write-Host "Please install CMake $MinimumCMakeVersion or newer manually from: https://cmake.org/download/"
        Write-Host "Make sure to add it to PATH during installation."
        exit 1
    }
}

& $cmakePath --version

# Check if Qt is available
Write-Host ""
Write-Host "Checking for Qt..."
$QtDir = "C:\Qt\$QtVersion\$Compiler"
if (-not (Test-Path $QtDir)) {
    Write-Host "Downloading Qt $QtVersion ($Compiler)..."

    # qtcharts backs the charting library; Core/Gui/Widgets/Svg/Network ship in
    # the base package. Qt OpcUa is built from source at configure time.
    python -m aqt install-qt windows desktop $QtVersion $QtArch --outputdir C:\Qt -m qtcharts

    if ($LASTEXITCODE -ne 0) {
        Remove-Item $QtDir -Recurse -Force -ErrorAction SilentlyContinue
        Write-Error "ERROR: aqtinstall failed while installing Qt. Exiting."
        exit 1
    }
}
Write-Host "Found Qt: $QtDir"

# Check for the OpenSSL runtime shipped by the Qt tools (needed by Qt OpcUa)
Write-Host ""
Write-Host "Checking for OpenSSL..."
$OpenSslRoot = "C:\Qt\Tools\OpenSSLv3\Win_x64"
if (-not (Test-Path (Join-Path $OpenSslRoot "bin"))) {
    Initialize-Aqt
    Write-Host "Downloading OpenSSL (tools_opensslv3_x64)..."
    python -m aqt install-tool windows desktop tools_opensslv3_x64 --outputdir C:\Qt

    if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $OpenSslRoot "bin"))) {
        Write-Error "ERROR: Failed to install OpenSSL via aqtinstall."
        exit 1
    }
}
Write-Host "Found OpenSSL: $OpenSslRoot"

# Ninja is required both for the main build and for building Qt OpcUa
Write-Host ""
Write-Host "Checking for Ninja..."
$ninjaCommand = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninjaCommand) {
    Write-Host "Found Ninja: $($ninjaCommand.Source)"
} else {
    $ninjaDir = "C:\Qt\Tools\Ninja"
    if (-not (Test-Path (Join-Path $ninjaDir "ninja.exe"))) {
        Initialize-Aqt
        Write-Host "Downloading Ninja (tools_ninja)..."
        python -m aqt install-tool windows desktop tools_ninja --outputdir C:\Qt

        if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $ninjaDir "ninja.exe"))) {
            Write-Error "ERROR: Failed to install Ninja via aqtinstall."
            exit 1
        }
    }
    $env:PATH = "$ninjaDir;$env:PATH"
    Write-Host "Found Ninja: $(Join-Path $ninjaDir 'ninja.exe')"
}

# Put the Qt tools on PATH (windeployqt, qmake) and import the MSVC environment
$QtBin = Join-Path $QtDir "bin"
$env:PATH = "$QtBin;$env:PATH"

Write-Host ""
Import-VisualStudioEnvironment -VsPath $vsPath

# Create the build directory, following build.sh's naming convention:
# build-ouaexp-Qt_<version>_<compiler>_<arch>-<BuildType>
$QtVersionSafe = $QtVersion -replace '\.', '_'
$MachineArch   = "x86_64"
$BuildDir = Join-Path $PSScriptRoot "build-ouaexp-Qt_${QtVersionSafe}_msvc_${MachineArch}-$BuildType"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

$QtPrefix    = $QtDir       -replace '\\', '/'
$OpenSslPath = $OpenSslRoot -replace '\\', '/'

Write-Host ""
Write-Host "Configuring project with CMake..."
$cmakeArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_PREFIX_PATH=`"$QtPrefix`"",
    "-DOPENSSL_ROOT_DIR=`"$OpenSslPath`"",
    "-DOUAEXP_BUILD_TESTS=$(if ($Tests) { 'ON' } else { 'OFF' })"
)

& $cmakePath @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}

Write-Host ""
Write-Host "Building project..."
& $cmakePath --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

if ($Tests) {
    Write-Host ""
    Write-Host "Running tests..."
    & $cmakePath --build $BuildDir --parallel --target ouaexp_check
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed"
        exit 1
    }
}

if ($Install) {
    $prefix = if ($InstallPrefix) { $InstallPrefix } else { $DefaultPrefix }
    Write-Host ""
    Write-Host "Installing into $prefix ..."
    & $cmakePath --install $BuildDir --prefix $prefix
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Install failed"
        exit 1
    }
}

Write-Host ""
Write-Host "=== Build finished successfully ==="
Write-Host ""
Write-Host "Build directory: $BuildDir"
Write-Host ""
if (-not $Install) {
    Write-Host "To install or uninstall Open UaExplorer, run:"
    Write-Host ""
    Write-Host "    cmake --install `"$BuildDir`""
    Write-Host "    cmake --build `"$BuildDir`" --target uninstall"
    Write-Host ""
    Write-Host "Default install directory: $DefaultPrefix"
    Write-Host ""
}
