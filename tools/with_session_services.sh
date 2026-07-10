#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
# SPDX-License-Identifier: MIT
#
# Runs "$@" inside a D-Bus session that provides the two services the Linux
# tests need but a headless CI machine does not have:
#
#   org.freedesktop.secrets           gnome-keyring, for the SecretStore tests
#   org.freedesktop.portal.Settings   tools/fake_portal.py, for the theme tests
#
# Must itself run under dbus-run-session, e.g.
#
#   dbus-run-session -- bash tools/with_session_services.sh cmake --build build --target ouaexp_check

set -euo pipefail

if [ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]; then
    echo "with_session_services.sh must run under dbus-run-session." >&2
    exit 1
fi

if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
    XDG_RUNTIME_DIR=$(mktemp -d)
    export XDG_RUNTIME_DIR
fi

# Clients ask the secret service for the "default" collection, which resolves through
# this alias file. Without it -- and without an actual login keyring behind it -- the
# daemon tries to open a GTK prompt to create one, which cannot work on a headless
# machine: the prompt dies and every write fails. An empty password does not create the
# keyring either, so the daemon is unlocked with a throwaway one instead.
keyring_dir="${XDG_DATA_HOME:-$HOME/.local/share}/keyrings"
rm -rf "$keyring_dir"
mkdir -p "$keyring_dir"
printf 'login' >"$keyring_dir/default"

eval "$(printf '%s' "${OUAEXP_KEYRING_PASSWORD:-ouaexp-ci}" \
    | gnome-keyring-daemon --unlock --components=secrets)"
export GNOME_KEYRING_CONTROL

# Fail here, loudly, rather than let every SecretStore test quietly skip itself.
if ! printf 'probe' | timeout 30 secret-tool store \
        --label=ouaexp-ci-probe service ouaexp-ci-probe key probe; then
    echo "The secret service is not usable; SecretStore tests would skip." >&2
    exit 1
fi
timeout 10 secret-tool clear service ouaexp-ci-probe key probe || true

# The portal needs dbus-python and PyGObject, which come from apt rather than pip,
# so it runs under the system interpreter instead of whatever "python" resolves to.
portal_python=${OUAEXP_PORTAL_PYTHON:-/usr/bin/python3}
portal_log=$(mktemp)
"$portal_python" "$(dirname "$0")/fake_portal.py" --scheme light >"$portal_log" 2>&1 &
portal_pid=$!
trap 'kill "$portal_pid" 2>/dev/null || true; rm -f "$portal_log"' EXIT

for _ in $(seq 100); do
    if grep -q READY "$portal_log"; then
        break
    fi
    if ! kill -0 "$portal_pid" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

if ! grep -q READY "$portal_log"; then
    echo "fake_portal.py failed to start:" >&2
    cat "$portal_log" >&2
    exit 1
fi

"$@"
