# Building and testing details

The helper scripts `build.sh` (Linux, macOS) and `build.ps1` (Windows) install
the required build tools and Qt, configure the project and build it. This
document describes what exactly they install on each platform, and how to run
the test suite and measure its coverage. For the common usage and options, see
the [Building](README.md#building) section of the README.

The minimum is Qt 6.9: the patches the build applies to the bundled Qt OPC UA
backend (`src/cmake/patches`) rewrite plugin code that only reached Qt in that
release.

## Linux

`build.sh` installs the toolchain (compiler, CMake, Ninja, pkg-config), the
X11/XCB and OpenGL runtime libraries and Qt 6 (Base, Tools, SVG and Charts)
through the distribution package manager. When the distribution ships a Qt
older than the minimum required, it automatically downloads a suitable Qt 6
with [aqtinstall](https://github.com/miurahr/aqtinstall).

aqtinstall needs Python 3.9 or newer; on an older Python, pip falls back to an
aqtinstall release that does not know any Qt past 6.7. When the system Python is
older, `build.sh` builds Python 3.12 into `.build-tools` and uses it for
aqtinstall.

Which Qt aqtinstall may install is capped by the system glibc: Qt's binaries are
linked against glibc 2.28 up to Qt 6.9 and against glibc 2.34 from Qt 6.10 on.
On Astra Linux 1.7 (glibc 2.28) that leaves Qt 6.9 as the newest usable release;
a newer Qt installs but does not start.

## macOS

`build.sh` installs the dependencies through [Homebrew](https://brew.sh):
`cmake`, `ninja`, `qt`, `openssl@3`, `brotli` and `webp`.

## Windows

`build.ps1` installs the toolchain (Python, CMake, Ninja, OpenSSL and the
Visual Studio C++ Build Tools) through winget or Chocolatey, then downloads
Qt 6 (Base, SVG and Charts) with
[aqtinstall](https://github.com/miurahr/aqtinstall) when no suitable Qt is
already installed under `C:\Qt`, builds Qt OPC UA from source, and builds the
app with Ninja and MSVC.

Instead of overriding the execution policy on every run, you can allow local
scripts once for your user and then call the script directly:

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
.\build.ps1
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
