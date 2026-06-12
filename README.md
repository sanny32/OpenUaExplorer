# OpenUaExplorer

OpenUaExplorer is an open source OPC UA client for browsing, inspecting, and monitoring OPC UA servers.

![OpenUaExplorer light theme](.github/assets/app-light.png)

![OpenUaExplorer dark theme](.github/assets/app-dark.png)

## Features

- Browse the OPC UA address space and inspect node attributes.
- View Data Access, subscriptions, events, and history panels.
- Use the built-in activity log while working with connections.
- Switch between light and dark application themes.

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

```sh
python tools/coverage.py            # build, test, report
python tools/coverage.py --open     # also open the HTML report
# Forward extra CMake args (e.g. Qt location) after a literal --:
python tools/coverage.py -- -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

It prints a `Line coverage: NN.N%` summary and writes the full report to
`build-coverage/coverage/` (`coverage.xml` for CI, `index.html` to browse).
Coverage instrumentation is gated behind the `OUAEXP_ENABLE_COVERAGE` CMake
option, so normal builds are unaffected.

The build defaults to the `RelWithDebInfo` configuration: the instrumentation
already forces `-O0`/`/Od` plus debug info on the test targets for accurate line
mapping, while a release config keeps Qt happy when loading the (release-built)
Qt OPC UA backend plugin — a debug build cannot load release Qt plugins.

One end-to-end test (`ouaexp_tests_integration`) drives the client against a real
OPC UA server launched from [tools/opcua_test_server.py](tools/opcua_test_server.py).
It needs Python with the `asyncua` package (`pip install asyncua`); when Python,
asyncua, or an OPC UA backend is unavailable the test skips itself, so it never
breaks a build that lacks the dependency.

## MIT License

Copyright 2026 Alexandr Ananev [mail@ananev.org]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

