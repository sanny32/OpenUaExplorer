#!/usr/bin/env python3
"""Minimal OPC UA server used by the integration tests.

Exposes, under the standard Objects folder, everything the integration tests
need to drive the live client paths, and prints machine-readable lines once it
is listening:

    ENDPOINT opc.tcp://127.0.0.1:<port>/ouaexp/
    NODE     ns=<idx>;s=the.answer      (writable Double = 42, stable)
    COUNTER  ns=<idx>;s=counter         (Double, changes over time)
    METHOD   ns=<idx>;s=multiply        (Double,Double -> Double)
    OBJECTS  ns=0;i=85                   (owner object for the method)
    SERVER_CERT <path>                   (only with --certificate-auth)
    READY

By default the server is anonymous and unsecured. Two authentication modes can
be requested instead:

    --user NAME --password SECRET
        Advertises only the username token on the unsecured endpoint and
        accepts exactly that credential pair.

    --certificate-auth [--client-certificate DER]...
        Generates a self-signed server key pair, advertises a
        Basic256Sha256/SignAndEncrypt endpoint carrying only the X509 token,
        and accepts exactly the listed client certificates. Passing none makes
        every client certificate untrusted, which is how the tests drive the
        rejection path.

The C++ tests launch this with QProcess, wait for READY, drive the client, then
kill the process. Requires the 'asyncua' package (pip install asyncua); if it is
missing the script prints NO_ASYNCUA and the tests skip themselves.
"""

import argparse
import asyncio
import sys
import tempfile
import time
from pathlib import Path


def build_user_manager(users, expected_user, expected_password, trusted_certificates):
    """Returns a user manager accepting one credential pair and a certificate allow-list."""

    class TestUserManager:
        def get_user(self, iserver, username=None, password=None, certificate=None):
            # A username token carries no certificate of its own; asyncua then passes the
            # secure-channel peer certificate here, which is empty on an open channel.
            if username is not None:
                if expected_user is None or username != expected_user:
                    return None
                if password != expected_password:
                    return None
                return users.User(role=users.UserRole.Admin)
            if certificate and certificate in trusted_certificates:
                return users.User(role=users.UserRole.User)
            return None

    return TestUserManager()


def create_server_certificate(certificate_dir, application_uri):
    """Generates a self-signed server key pair and returns the (certificate, key) paths."""
    from asyncua.crypto.cert_gen import setup_self_signed_certificate
    from cryptography.x509.oid import ExtendedKeyUsageOID

    certificate_file = certificate_dir / "server_cert.der"
    key_file = certificate_dir / "server_key.pem"
    asyncio.run(
        setup_self_signed_certificate(
            key_file,
            certificate_file,
            application_uri,
            "127.0.0.1",
            [ExtendedKeyUsageOID.CLIENT_AUTH, ExtendedKeyUsageOID.SERVER_AUTH],
            {"countryName": "XX", "organizationName": "OuaExp Tests", "commonName": "OuaExp Test Server"},
        )
    )
    return certificate_file, key_file


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=48401)
    parser.add_argument("--user", help="Enable username authentication for this user name.")
    parser.add_argument("--password", default="", help="Password expected together with --user.")
    parser.add_argument("--certificate-auth", action="store_true",
                        help="Enable a secured endpoint carrying the X509 user token.")
    parser.add_argument("--client-certificate", action="append", default=[], metavar="DER",
                        help="Trust this DER client certificate for X509 authentication.")
    args = parser.parse_args()

    try:
        from asyncua.sync import Server
        from asyncua import ua
        from asyncua.common.methods import uamethod
        from asyncua.server import user_managers as users
    except Exception as exc:  # pragma: no cover - exercised only when asyncua is absent
        print(f"NO_ASYNCUA {exc}", flush=True)
        return 1

    endpoint = f"opc.tcp://127.0.0.1:{args.port}/ouaexp/"
    server = Server()
    server.set_endpoint(endpoint)
    server.set_server_name("OuaExp Test Server")

    certificate_directory = None
    server_certificate = None
    if args.certificate_auth:
        certificate_directory = tempfile.TemporaryDirectory(prefix="ouaexp-server-pki-")
        server_certificate, server_key = create_server_certificate(
            Path(certificate_directory.name), server.aio_obj.get_application_uri())
        server.load_certificate(str(server_certificate))
        server.load_private_key(str(server_key))
        server.set_security_policy([ua.SecurityPolicyType.NoSecurity,
                                    ua.SecurityPolicyType.Basic256Sha256_SignAndEncrypt])
        server.set_identity_tokens([ua.X509IdentityToken])
    elif args.user:
        server.set_identity_tokens([ua.UserNameIdentityToken])

    if args.certificate_auth or args.user:
        trusted = {Path(path).read_bytes() for path in args.client_certificate}
        server.aio_obj.iserver.set_user_manager(
            build_user_manager(users, args.user, args.password, trusted))

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
        if server_certificate is not None:
            print(f"SERVER_CERT {server_certificate}", flush=True)
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
        if certificate_directory is not None:
            certificate_directory.cleanup()
    return 0


if __name__ == "__main__":
    sys.exit(main())
