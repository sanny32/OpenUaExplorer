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
# The staged tree is a plain /usr hierarchy that carries no packaging metadata, so
# the .deb and the .rpm workflow both build their package from this same output.

set -euo pipefail

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <stage-dir> <executable> <qt-prefix> <qtopcua-install-dir>" >&2
    exit 1
fi

STAGE_DIR="$1"
EXECUTABLE="$2"
QT_PREFIX="$3"
QTOPCUA_DIR="$4"

APP_NAME=ouaexp
APP_ROOT="$STAGE_DIR/usr/lib/$APP_NAME"
APP_BIN_DIR="$APP_ROOT/bin"
APP_LIB_DIR="$APP_ROOT/lib"
APP_PLUGIN_DIR="$APP_ROOT/plugins"

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

log "Resolving private library dependencies"

# A library belongs in the bundle only when it comes from one of the two prefixes we
# built or downloaded. Distribution libraries are left alone and become Depends.
is_private() {
    case "$1" in
        "$QT_PREFIX"/*|"$QTOPCUA_DIR"/*) return 0 ;;
        *) return 1 ;;
    esac
}

elf_files() {
    find "$APP_BIN_DIR" "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f \
        \( -name '*.so' -o -name '*.so.*' -o -name "$APP_NAME" \) -print
}

# ldd prints the resolved path for each DT_NEEDED entry. The entry name is what the
# loader will look for, so the copy is stored under that name and no symlink chain
# has to be recreated.
export LD_LIBRARY_PATH="$QT_PREFIX/lib:$QTOPCUA_DIR/lib:${LD_LIBRARY_PATH:-}"

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

log "Rewriting RPATHs"
patchelf --set-rpath '$ORIGIN/../lib' "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" -type f -name '*.so*' -exec patchelf --set-rpath '$ORIGIN' {} \;
find "$APP_PLUGIN_DIR" -type f -name '*.so' -exec patchelf --set-rpath '$ORIGIN/../../lib' {} \;

log "Stripping binaries"
strip --strip-unneeded "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f -name '*.so*' \
    -exec strip --strip-unneeded {} +

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
