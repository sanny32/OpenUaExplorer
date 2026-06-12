# OpenUaExplorer

OpenUaExplorer is an open source OPC UA client for browsing, inspecting, and monitoring OPC UA servers.

![OpenUaExplorer light theme](.github/assets/app-light.png)

![OpenUaExplorer dark theme](.github/assets/app-dark.png)

## Features

- Browse the OPC UA address space and inspect node attributes.
- View Data Access, subscriptions, events, and history panels.
- Use the built-in activity log while working with connections.
- Switch between light and dark application themes.

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

