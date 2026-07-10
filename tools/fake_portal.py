#!/usr/bin/env python3
"""Minimal org.freedesktop.portal.Settings service for headless Linux test runs.

AppTheme detects the system color scheme by asking the freedesktop portal over
D-Bus, and only enables the manual light/dark toggle when the portal answers.
CI has no desktop portal, so the theme tests skip themselves. This script serves
just the Settings.Read part of the portal on the session bus, which is enough to
make AppTheme report a real scheme.

Prints READY on stdout once the bus name is owned, then serves until killed.
Requires dbus-python and PyGObject (apt: python3-dbus python3-gi).

The default scheme is "light" on purpose: under the offscreen platform plugin
Qt's own style hints report an unknown (i.e. non-dark) scheme, and
TestMainWindowTheme::systemPreferenceFollowsTheCurrentScheme() compares the two.
"""

import argparse
import signal
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

BUS_NAME = "org.freedesktop.portal.Desktop"
OBJECT_PATH = "/org/freedesktop/portal/desktop"
INTERFACE = "org.freedesktop.portal.Settings"
NAMESPACE = "org.freedesktop.appearance"
KEY = "color-scheme"

# Portal color-scheme codes, as consumed by colorSchemeFromDBus() in apptheme.cpp.
SCHEMES = {"default": 0, "dark": 1, "light": 2}


class Settings(dbus.service.Object):
    """Serves the color-scheme setting the way xdg-desktop-portal does."""

    def __init__(self, bus_name, scheme):
        super().__init__(bus_name, OBJECT_PATH)
        self._scheme = scheme

    def _color_scheme(self, namespace, key):
        if namespace != NAMESPACE or key != KEY:
            raise dbus.exceptions.DBusException(
                "org.freedesktop.portal.Error.NotFound",
                "No such setting: {}.{}".format(namespace, key),
            )
        return self._scheme

    @dbus.service.method(INTERFACE, in_signature="ss", out_signature="v")
    def Read(self, namespace, key):
        # The real portal double-wraps here; variant_level=1 adds the inner variant.
        return dbus.UInt32(self._color_scheme(namespace, key), variant_level=1)

    @dbus.service.method(INTERFACE, in_signature="ss", out_signature="v")
    def ReadOne(self, namespace, key):
        return dbus.UInt32(self._color_scheme(namespace, key))

    @dbus.service.method(INTERFACE, in_signature="as", out_signature="a{sa{sv}}")
    def ReadAll(self, namespaces):
        if namespaces and NAMESPACE not in namespaces:
            return {}
        return {NAMESPACE: {KEY: dbus.UInt32(self._scheme)}}

    @dbus.service.signal(INTERFACE, signature="ssv")
    def SettingChanged(self, namespace, key, value):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scheme", choices=sorted(SCHEMES), default="light",
                        help="Color scheme to advertise (default: light).")
    arguments = parser.parse_args()

    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    if bus.name_has_owner(BUS_NAME):
        print("A real portal already owns {}.".format(BUS_NAME), file=sys.stderr)
        return 1

    name = dbus.service.BusName(BUS_NAME, bus, do_not_queue=True)
    Settings(name, SCHEMES[arguments.scheme])

    loop = GLib.MainLoop()
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGTERM, loop.quit)
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT, loop.quit)

    print("READY", flush=True)
    loop.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
