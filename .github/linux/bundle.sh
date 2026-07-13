#!/usr/bin/env bash
#
# Stage a self-contained OpenUaExplorer tree for packaging.
#
# The Qt build comes from the Qt installer rather than the distribution, so it has
# to travel with the package. Everything lands under /usr/lib/ouaexp - the layout
# Firefox and Thunderbird use - with the private Qt libraries in lib/, the Qt
# plugins in plugins/, and a bin/qt.conf that points Qt at both. Only libraries
# that live inside the Qt prefix are copied; anything from the distribution stays
# a package dependency, so the system OpenGL, xcb and C++ runtimes are used.
#
# OpenSSL is bundled the same way when openssl.sh has built it, because depending
# on the distribution's libssl3 excludes every distribution that still ships
# OpenSSL 1.1, such as Astra Linux 1.7.
#
# The staged tree is a plain /usr hierarchy that carries no packaging metadata, so
# the .deb and the .rpm workflow both build their package from this same output.

set -euo pipefail

if [ "$#" -lt 4 ] || [ "$#" -gt 5 ]; then
    echo "Usage: $0 <stage-dir> <executable> <qt-prefix> <qtopcua-install-dir>" \
         "[openssl-prefix]" >&2
    exit 1
fi

STAGE_DIR="$1"
EXECUTABLE="$2"
QT_PREFIX="$3"
QTOPCUA_DIR="$4"
OPENSSL_PREFIX="${5:-}"

APP_NAME=ouaexp
APP_ROOT="$STAGE_DIR/usr/lib/$APP_NAME"
APP_BIN_DIR="$APP_ROOT/bin"
APP_LIB_DIR="$APP_ROOT/lib"
APP_PLUGIN_DIR="$APP_ROOT/plugins"
APP_INSTALLED_LIB_DIR="/usr/lib/$APP_NAME/lib"
APP_BIN_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN/../lib"
APP_LIB_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN"
APP_PLUGIN_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN/../../lib"

# Qt plugins the application can load at run time. Groups that the installed Qt
# does not provide are skipped, so this list may name more than is ever present.
QT_PLUGIN_GROUPS=(
    platforms
    platformthemes
    platforminputcontexts
    xcbglintegrations
    egldeviceintegrations
    imageformats
    iconengines
    networkinformation
    tls
    styles
)

log() {
    printf '\033[32m-- %s\033[0m\n' "$*"
}

die() {
    printf '\033[31m** %s\033[0m\n' "$*" >&2
    exit 1
}

[ -x "$EXECUTABLE" ]     || die "Executable not found: $EXECUTABLE"
[ -d "$QT_PREFIX/lib" ]  || die "Qt prefix has no lib directory: $QT_PREFIX"
[ -d "$QTOPCUA_DIR" ]    || die "Qt OpcUa install directory not found: $QTOPCUA_DIR"

if [ -n "$OPENSSL_PREFIX" ]; then
    [ -d "$OPENSSL_PREFIX/lib" ] \
        || die "OpenSSL prefix has no lib directory: $OPENSSL_PREFIX"
fi

command -v patchelf >/dev/null || die "patchelf is required"

rm -rf "$APP_ROOT"
mkdir -p "$APP_BIN_DIR" "$APP_LIB_DIR" "$APP_PLUGIN_DIR"

log "Copying the executable"
install -m 0755 "$EXECUTABLE" "$APP_BIN_DIR/$APP_NAME"

# Qt resolves qt.conf next to the executable, and Prefix is taken relative to that
# directory, so the plugins are found no matter how the program was invoked.
cat > "$APP_BIN_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
EOF
chmod 0644 "$APP_BIN_DIR/qt.conf"

log "Copying Qt plugins"
for group in "${QT_PLUGIN_GROUPS[@]}"; do
    if [ -d "$QT_PREFIX/plugins/$group" ]; then
        cp -a "$QT_PREFIX/plugins/$group" "$APP_PLUGIN_DIR/"
    fi
done

# cmake/qtopcua.cmake installs into its own prefix, so the open62541 backend is not
# in the Qt plugin directory.
opcua_plugin_dir="$(find "$QTOPCUA_DIR" -type d -name opcua -print -quit)"
[ -n "$opcua_plugin_dir" ] || die "No opcua plugin directory under $QTOPCUA_DIR"
cp -a "$opcua_plugin_dir" "$APP_PLUGIN_DIR/"

# The Qt OpcUa build detaches the debug information of its backend into a .debug
# file that sits in the same plugin directory. Nothing loads it, it is larger than
# the rest of the bundle put together, and it is not an object a package may ship:
# lintian rejects it as unstripped and as having a bad dynamic table.
find "$APP_PLUGIN_DIR" -type f -name '*.debug' -delete

# Qt OpcUa links its open62541 backend against OpenSSL, but Qt itself opens it with
# dlopen, so the dependency walk below would never see it. Both are served by the
# copy made here: the installed package uses a fixed private libdir RPATH.
if [ -n "$OPENSSL_PREFIX" ]; then
    log "Copying OpenSSL"
    for lib in libcrypto libssl; do
        path="$(find "$OPENSSL_PREFIX/lib" -maxdepth 1 -type f -name "$lib.so.*" -print -quit)"
        [ -n "$path" ] || die "No $lib in $OPENSSL_PREFIX/lib"
        # The loader looks the library up under its soname, so that is the name it is
        # stored under, exactly as with the Qt libraries below.
        soname="$(patchelf --print-soname "$path")"
        install -m 0644 "$path" "$APP_LIB_DIR/$soname"
        log "  bundled $soname"
    done
fi

log "Resolving private library dependencies"

# A library belongs in the bundle only when it comes from one of the prefixes we built
# or downloaded. Distribution libraries are left alone and become Depends.
is_private() {
    case "$1" in
        "$QT_PREFIX"/*|"$QTOPCUA_DIR"/*) return 0 ;;
    esac
    # An empty prefix would turn the pattern into /*, which matches everything.
    if [ -n "$OPENSSL_PREFIX" ]; then
        case "$1" in
            "$OPENSSL_PREFIX"/*) return 0 ;;
        esac
    fi
    return 1
}

elf_files() {
    find "$APP_BIN_DIR" "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f \
        \( -name '*.so' -o -name '*.so.*' -o -name "$APP_NAME" \) -print
}

# ldd prints the resolved path for each DT_NEEDED entry. The entry name is what the
# loader will look for, so the copy is stored under that name and no symlink chain
# has to be recreated.
export LD_LIBRARY_PATH="$QT_PREFIX/lib:$QTOPCUA_DIR/lib:${OPENSSL_PREFIX:+$OPENSSL_PREFIX/lib:}${LD_LIBRARY_PATH:-}"

copied_any=1
while [ "$copied_any" -eq 1 ]; do
    copied_any=0
    while read -r elf; do
        while read -r needed path; do
            [ -n "$path" ] || continue
            is_private "$path" || continue
            [ -e "$APP_LIB_DIR/$needed" ] && continue
            cp -L "$path" "$APP_LIB_DIR/$needed"
            chmod 0644 "$APP_LIB_DIR/$needed"
            log "  bundled $needed"
            copied_any=1
        done < <(ldd "$elf" | sed -nE 's|^[[:space:]]*([^[:space:]]+) => (/[^[:space:]]+).*|\1 \2|p')
    done < <(elf_files)
done

# Stripping comes first: strip rewrites the section headers of what patchelf has
# already rewritten, and the two together leave an object whose sections no longer
# describe the file. Qt reads a plugin's metadata out of its .qtmetadata section, so
# a plugin that survives this as a loadable library still stops being a plugin.
log "Stripping binaries"
strip --strip-unneeded "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f -name '*.so*' \
    -exec strip --strip-unneeded {} +

log "Rewriting RPATHs"
patchelf --force-rpath --set-rpath "$APP_BIN_RPATH" "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" -type f -name '*.so*' -exec patchelf --force-rpath --set-rpath "$APP_LIB_RPATH" {} \;
find "$APP_PLUGIN_DIR" -type f -name '*.so' -exec patchelf --force-rpath --set-rpath "$APP_PLUGIN_RPATH" {} \;

# Qt finds a plugin's metadata by name of the section it lives in, and both strip and
# patchelf are able to leave the section behind while the library still loads. The
# program would then start and die on "no Qt platform plugin could be initialized",
# which is a bad way to learn that the bundle is broken.
#
# Whether a plugin carries the section at all is Qt's business, not ours - a few of
# the ones shipped do not - so what is checked is that the copy still has what the
# original had.
has_metadata() {
    readelf --sections --wide "$1" 2>/dev/null | grep -q '\.qtmetadata'
}

log "Checking that the plugins kept their metadata"
while read -r plugin; do
    has_metadata "$plugin" && continue

    # The OPC UA backend is installed into its own prefix, so where the original of a
    # plugin is depends on which one it is.
    name="${plugin##*/}"
    original="$(find "$QT_PREFIX/plugins" "$QTOPCUA_DIR" -type f -name "$name" -print -quit)"

    [ -n "$original" ] || continue
    ! has_metadata "$original" \
        || die "The metadata section was lost from $plugin while staging it"
done < <(find "$APP_PLUGIN_DIR" -type f -name '*.so')

find "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f -exec chmod 0644 {} +
chmod 0755 "$APP_BIN_DIR/$APP_NAME"

# Qt reads /proc/self/exe, so applicationDirPath() is the private bin directory even
# when the program is started through this symlink, and qt.conf still applies.
mkdir -p "$STAGE_DIR/usr/bin"
ln -sfn "../lib/$APP_NAME/bin/$APP_NAME" "$STAGE_DIR/usr/bin/$APP_NAME"

# From here on the bundle has to stand on its own, so nothing may resolve through
# LD_LIBRARY_PATH any more.
log "Checking that every library resolves through the bundle"
unset LD_LIBRARY_PATH
unresolved=0
while read -r elf; do
    if ldd "$elf" | grep -q 'not found'; then
        printf '\033[31m** unresolved libraries in %s\033[0m\n' "$elf" >&2
        ldd "$elf" | grep 'not found' >&2
        unresolved=1
    fi
done < <(elf_files)
[ "$unresolved" -eq 0 ] || die "The bundle is incomplete"

log "Bundle staged in $APP_ROOT"
