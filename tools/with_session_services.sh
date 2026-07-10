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

# An empty password unlocks (and, on a fresh machine, creates) the default keyring.
eval "$(printf '' | gnome-keyring-daemon --unlock --components=secrets)"
export GNOME_KEYRING_CONTROL

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
