#!/usr/bin/env bash
set -euo pipefail

source <(sed -n '/^system_openssl3_available() {/,/^}/p' "${1:-build.sh}")

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
