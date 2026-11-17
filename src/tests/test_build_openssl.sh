#!/usr/bin/env bash
set -euo pipefail

source <(sed -n '/^system_openssl3_available() {/,/^}/p' "${1:-build.sh}")
source <(sed -n '/^configure_linux_openssl() {/,/^}/p' "${1:-build.sh}")

pkg-config() {
    case "$1" in
        --exists) return 0 ;;
        --atleast-version=3) [ "$openssl_major" -ge 3 ] ;;
        --atleast-version=4) [ "$openssl_major" -ge 4 ] ;;
        *) return 1 ;;
    esac
}

for openssl_major in 1 2 3 4 5; do
    if system_openssl3_available; then
        [ "$openssl_major" -eq 3 ] || exit 1
    else
        [ "$openssl_major" -ne 3 ] || exit 1
    fi
done

(
    unset OPENSSL_ROOT_DIR OPENSSL_INCLUDE_DIR OPENSSL_SSL_LIBRARY OPENSSL_CRYPTO_LIBRARY
    DISTRO=arch
    TOOLS_DIR=/unused
    openssl_major=4
    package_installed() { [ "$1" = openssl-3 ]; }
    package_available() { return 1; }
    install_packages() { [ "$1" = openssl-3 ]; }
    log_info() { :; }
    configure_linux_openssl
    [ "$OPENSSL_INCLUDE_DIR" = /usr/include/openssl-3 ]
    [ "$OPENSSL_SSL_LIBRARY" = /usr/lib/libssl.so.3 ]
    [ "$OPENSSL_CRYPTO_LIBRARY" = /usr/lib/libcrypto.so.3 ]
)
