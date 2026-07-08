#!/usr/bin/env python3
"""Minimal OPC UA server used by the integration test (test_opcua_integration).

Exposes, under the standard Objects folder, everything the integration test
needs to drive the live client paths, and prints machine-readable lines once it
is listening:

    ENDPOINT opc.tcp://127.0.0.1:<port>/ouaexp/
    NODE     ns=<idx>;s=the.answer      (writable Double = 42, stable)
    COUNTER  ns=<idx>;s=counter         (Double, changes over time)
    METHOD   ns=<idx>;s=multiply        (Double,Double -> Double)
    OBJECTS  ns=0;i=85                   (owner object for the method)
    READY

The C++ test launches this with QProcess, waits for READY, drives the client,
then kills the process. Requires the 'asyncua' package (pip install asyncua); if
it is missing the script prints NO_ASYNCUA and the test skips itself.
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
        from asyncua.common.methods import uamethod
    except Exception as exc:  # pragma: no cover - exercised only when asyncua is absent
        print(f"NO_ASYNCUA {exc}", flush=True)
        return 1

    endpoint = f"opc.tcp://127.0.0.1:{args.port}/ouaexp/"
    server = Server()
    server.set_endpoint(endpoint)
    server.set_server_name("OuaExp Test Server")

    idx = server.register_namespace("http://ouaexp.tests")
    objects = server.nodes.objects

    answer = objects.add_variable(
        ua.NodeId("the.answer", idx),
        ua.QualifiedName("TheAnswer", idx),
        42.0,
        ua.VariantType.Double,
    )
    answer.set_writable()

    counter = objects.add_variable(
        ua.NodeId("counter", idx),
        ua.QualifiedName("Counter", idx),
        0.0,
        ua.VariantType.Double,
    )
    counter.set_writable()

    @uamethod
    def multiply(parent, x, y):
        return x * y

    objects.add_method(
        ua.NodeId("multiply", idx),
        ua.QualifiedName("Multiply", idx),
        multiply,
        [ua.VariantType.Double, ua.VariantType.Double],
        [ua.VariantType.Double],
    )

    server.start()
    try:
        print(f"ENDPOINT {endpoint}", flush=True)
        print(f"NODE ns={idx};s=the.answer", flush=True)
        print(f"COUNTER ns={idx};s=counter", flush=True)
        print(f"METHOD ns={idx};s=multiply", flush=True)
        print("OBJECTS ns=0;i=85", flush=True)
        print("READY", flush=True)

        # Drive the counter so subscriptions receive data-change notifications and
        # history accumulates samples.
        tick = 0.0
        while True:
            tick += 1.0
            counter.write_value(tick)
            time.sleep(0.2)
    except KeyboardInterrupt:  # pragma: no cover - graceful Ctrl+C shutdown
        pass
    finally:
        server.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
