# OpenUaExplorer

OpenUaExplorer is an open source OPC UA client for browsing, inspecting, and monitoring OPC UA servers.

![OpenUaExplorer light theme](.github/assets/app-light.png)

![OpenUaExplorer dark theme](.github/assets/app-dark.png)

## Features

- Browse the OPC UA address space and inspect node attributes.
- View Data Access, subscriptions, events, and history panels.
- Use the built-in activity log while working with connections.
- Switch between light and dark application themes.

## Building

The repository ships helper scripts that install the required build tools and
Qt 6, configure the project and build it — usually in a single command:
`build.sh` on Linux and macOS, and `build.ps1` on
[Windows](#windows).

```sh
./build.sh
```

Options:

| Option              | Description                                              |
|---------------------|----------------------------------------------------------|
| `--tests`           | Build and run the test suite after building.             |
| `--install[=PREFIX]`| Install after building (optionally into `PREFIX`).       |
| `--help`            | Show usage.                                              |

The Windows script exposes the same options as PowerShell switches — see
[Windows](#windows).

### Linux

`build.sh` detects the distribution and installs the needed packages, so it
needs `sudo` (or to be run as root) the first time. Supported package managers:
Debian/Ubuntu (`apt`), Fedora/RHEL/Rocky (`dnf`), ALT Linux (`apt-rpm`),
openSUSE (`zypper`) and Arch (`pacman`).

The build is continuously tested on:

- <img src="docs/icons/logo_ubuntu.svg" width="16" height="16" /> **Ubuntu Linux** 22.04, 24.04 and 26.04
- <img src="docs/icons/logo_fedora.svg" width="16" height="16" /> **Fedora Linux** 43 and 44
- <img src="docs/icons/logo_rocky.png" width="16" height="16" /> **Rocky Linux** 10.1
- <img src="docs/icons/logo_redos.png" width="16" height="16" /> **RED OS** 8
- <img src="docs/icons/logo_astra.png" width="18" height="18" /> **Astra Linux** 1.8
- <img src="docs/icons/logo_alt.png" width="16" height="16" /> **ALT Linux** 11
- <img src="docs/icons/logo_opensuse.svg" width="16" height="16" /> **openSUSE Tumbleweed**
- <img src="docs/icons/logo_arch.svg" width="16" height="16" /> **Arch Linux**

It installs the toolchain (compiler, CMake, Ninja, pkg-config), the X11/XCB and
OpenGL runtime libraries and Qt 6 (Base, Tools, SVG and Charts). When the
distribution ships a Qt older than the minimum required, it automatically
downloads a suitable Qt 6 with [aqtinstall](https://github.com/miurahr/aqtinstall).

Build only:

```sh
./build.sh
```

Build, then install:

```sh
./build.sh --install
```

### macOS

The build is continuously tested on:

- <img src="docs/icons/logo_apple.svg" width="16" height="16" /> **macOS** 26 (Apple silicon)

Recent macOS releases with the prerequisites below should also work.

Prerequisites — install these once:

- **Xcode Command Line Tools**: `xcode-select --install`
- **[Homebrew](https://brew.sh)**

`build.sh` then installs the rest through Homebrew (`cmake`, `ninja`, `qt`,
`openssl@3`, `brotli`, `webp`) and builds the app bundle:

Build only:

```sh
./build.sh
```

Build, then install (into `$HOME/Applications` by default):

```sh
./build.sh --install
```

### Windows

Windows builds use the `build.ps1` PowerShell script. The build is continuously
tested on:

- <img src="docs/icons/logo_windows.svg" width="16" height="16" /> **Windows** 10 and 11 (x64, MSVC)

`build.ps1` installs the toolchain (Python, CMake, Ninja, OpenSSL and the Visual
Studio C++ Build Tools), then downloads Qt 6 (Base, SVG and Charts) with
[aqtinstall](https://github.com/miurahr/aqtinstall) when no suitable Qt is
already installed under `C:\Qt`, builds Qt OPC UA from source, and builds the app
with Ninja and MSVC.

Prerequisites — install these once:

- A **package manager**: [winget](https://aka.ms/getwinget) (ships with Windows
  11 and recent Windows 10) or [Chocolatey](https://chocolatey.org/install).
  `build.ps1` uses it to install any missing build tools. Confirm it is available
  with `winget --version` (or `choco --version`).
- **Visual Studio 2022** with the *Desktop development with C++* workload, or let
  the script install the Build Tools through your package manager.

PowerShell blocks unsigned scripts by default, so run it with an execution-policy
override (no administrator rights required):

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Alternatively, allow local scripts once for your user, then call it directly:

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
.\build.ps1
```

Options:

| Option                    | Description                                          |
|---------------------------|------------------------------------------------------|
| `-Tests`                  | Build and run the test suite after building.         |
| `-Install`                | Install after building.                              |
| `-InstallPrefix <PREFIX>` | Install after building into `PREFIX`.                |
| `-Help`                   | Show usage.                                          |

Build only:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Build, then install (into `C:\Program Files\Open UaExplorer` by default; the
default location needs an elevated *Administrator* PowerShell):

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Install
```

### Manual CMake build

If you already have Qt 6 and a compiler installed, you can configure and build
directly with CMake (point `CMAKE_PREFIX_PATH` at your Qt kit):

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
cmake --build build --parallel
```

## Tests

Install the test dependencies:

```sh
pip install -r tools/requirements.txt
```

Configure the build:

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

Build:

```sh
cmake --build build --parallel
```

Run the suite with CTest:

```sh
ctest --test-dir build --output-on-failure
```

## Test coverage

A cross-platform helper measures how much of the tested production code the unit
tests exercise. It configures a dedicated coverage build, runs the CTest suite
and writes a Cobertura + HTML report, picking the backend from the compiler:

| Compiler    | Backend          | Install                              |
|-------------|------------------|--------------------------------------|
| GCC / Clang | `gcovr`          | `pip install gcovr`                  |
| MSVC        | `OpenCppCoverage`| `winget install OpenCppCoverage`     |

Run it from the repository root (in a shell where your compiler and Qt are on
the path — e.g. an MSVC *Developer* prompt on Windows):

Build, test and write the report:

```sh
python tools/coverage.py
```

Also open the HTML report:

```sh
python tools/coverage.py --open
```

If needed, forward extra CMake args (e.g. Qt location) after a literal `--`:

```sh
python tools/coverage.py -- -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

## MIT License

Copyright 2026 Alexandr Ananev [mail@ananev.org]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

