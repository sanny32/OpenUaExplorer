#!/usr/bin/env bash
#
# Build the OpenSSL that the Qt installer ships, for the bundle to carry.
#
# The package used to depend on the distribution's libssl3, which rules out every
# distribution that predates OpenSSL 3 - Astra Linux 1.7 among them. The bundle
# already carries its Qt, so it carries the OpenSSL that Qt was tested against as
# well. On Linux the Qt installer provides it as source (Tools/OpenSSLv3/src), so
# it has to be compiled before bundle.sh can pick it up.
#
# Only the shared libraries and the headers are installed: the headers let Qt OpcUa
# link its open62541 backend against this OpenSSL rather than the system one, and
# bundle.sh copies libssl.so.3 and libcrypto.so.3 out of the prefix.
#
# The build is skipped when the prefix already holds the libraries, so the whole
# prefix can be restored from a cache.

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <openssl-source-dir> <install-prefix>" >&2
    exit 1
fi

SOURCE_ROOT="$1"
PREFIX="$2"

log() {
    printf '\033[32m-- %s\033[0m\n' "$*"
}

die() {
    printf '\033[31m** %s\033[0m\n' "$*" >&2
    exit 1
}

if [ -e "$PREFIX/lib/libssl.so.3" ] && [ -e "$PREFIX/lib/libcrypto.so.3" ]; then
    log "OpenSSL is already built in $PREFIX"
    exit 0
fi

[ -d "$SOURCE_ROOT" ] || die "OpenSSL source directory not found: $SOURCE_ROOT"

# The tool unpacks as Tools/OpenSSLv3/src, but the archive has carried an extra
# level in the past, so the tree is searched for the configure script instead of
# assuming its depth.
if [ -x "$SOURCE_ROOT/Configure" ]; then
    SOURCE_DIR="$SOURCE_ROOT"
else
    SOURCE_DIR="$(
        find "$SOURCE_ROOT" -maxdepth 3 -name Configure -type f -printf '%h\n' -quit
    )"
    [ -n "$SOURCE_DIR" ] || die "No OpenSSL Configure script under $SOURCE_ROOT"
fi

log "Building OpenSSL from $SOURCE_DIR"

# --libdir keeps the libraries in lib/ on every architecture, so bundle.sh and the
# packaging workflows have one path to look in.
#
# --openssldir is compiled into libcrypto as the place to look for the trust store,
# and it has to name a directory that exists on the systems the package installs on
# rather than this build's prefix. /etc/ssl/certs is where Debian, Ubuntu, Astra and
# openSUSE keep the CA bundle, and Fedora, RHEL and their derivatives symlink it
# onto /etc/pki/tls/certs, so it is the one path common to all of them. Without it
# the bundled OpenSSL would find no CA certificates and every TLS connection - the
# update check among them - would fail to verify.
(
    cd "$SOURCE_DIR"
    ./Configure \
        --prefix="$PREFIX" \
        --libdir=lib \
        --openssldir=/etc/ssl \
        shared \
        no-tests
    make -j"$(nproc)"
    make install_sw
)

[ -e "$PREFIX/lib/libssl.so.3" ] || die "The build produced no libssl.so.3"
[ -e "$PREFIX/lib/libcrypto.so.3" ] || die "The build produced no libcrypto.so.3"

log "OpenSSL installed in $PREFIX"
