#!/usr/bin/env python3
"""Minimal OPC UA server used by the integration test (test_opcua_integration).

Exposes a single writable Double variable under the standard Objects folder and
prints three machine-readable lines once it is listening:

    ENDPOINT opc.tcp://127.0.0.1:<port>/ouaexp/
    NODE     ns=<idx>;s=the.answer
    READY

The C++ test launches this with QProcess, waits for READY, drives a full
discover/connect/browse/read/write/disconnect cycle, then kills the process.
Requires the 'asyncua' package (pip install asyncua); if it is missing the
script prints NO_ASYNCUA and the test skips itself.
"""

import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=48401)
    args = parser.parse_args()

    try:
        from asyncua.sync import Server
        from asyncua import ua
    except Exception as exc:  # pragma: no cover - exercised only when asyncua is absent
        print(f"NO_ASYNCUA {exc}", flush=True)
        return 1

    endpoint = f"opc.tcp://127.0.0.1:{args.port}/ouaexp/"
    server = Server()
    server.set_endpoint(endpoint)
    server.set_server_name("OuaExp Test Server")

    idx = server.register_namespace("http://ouaexp.tests")
    variable = server.nodes.objects.add_variable(
        ua.NodeId("the.answer", idx),
        ua.QualifiedName("TheAnswer", idx),
        42.0,
        ua.VariantType.Double,
    )
    variable.set_writable()

    server.start()
    try:
        print(f"ENDPOINT {endpoint}", flush=True)
        print(f"NODE ns={idx};s=the.answer", flush=True)
        print("READY", flush=True)
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:  # pragma: no cover - graceful Ctrl+C shutdown
        pass
    finally:
        server.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
