#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$PROJECT_DIR/src"
TOOLS_DIR="$PROJECT_DIR/.build-tools"
BUILD_TYPE=Release
RUN_TESTS=0
RUN_INSTALL=0
INSTALL_PREFIX=""
QT_FROM_AQT=0

QT_PREFIX=""
QT_VERSION=""
CMAKE_BIN=""

print_banner() {
    echo "================================"
    echo " OpenUaExplorer build script"
    echo "================================"
    echo ""
}

usage() {
    echo "Usage: ./build.sh [--tests] [--install[=PREFIX]]"
    echo ""
    echo "Options:"
    echo "  --tests             Build and run the test suite"
    echo "  --install[=PREFIX]  Install after building"
}

version_part() {
    local version="$1"
    local index="$2"
    local part
    local -a parts

    IFS=. read -r -a parts <<<"$version"
    part="${parts[$index]:-0}"
    printf '%s' "${part%%[^0-9]*}"
}

version_ge() {
    local left="$1"
    local right="$2"
    local i
    local left_part
    local right_part

    for i in 0 1 2 3; do
        left_part="$(version_part "$left" "$i")"
        right_part="$(version_part "$right" "$i")"
        left_part="${left_part:-0}"
        right_part="${right_part:-0}"

        if ((10#$left_part > 10#$right_part)); then
            return 0
        fi
        if ((10#$left_part < 10#$right_part)); then
            return 1
        fi
    done

    return 0
}

extract_version() {
    grep -oE '[0-9]+(\.[0-9]+){1,3}' | head -n1
}

required_cmake_version() {
    local version

    version="$(sed -nE 's/^[[:space:]]*cmake_minimum_required[[:space:]]*\([[:space:]]*VERSION[[:space:]]+([0-9]+(\.[0-9]+){1,3}).*/\1/p' \
        "$SOURCE_DIR/CMakeLists.txt" | head -n1)"
    if [ -z "$version" ]; then
        echo "Error: cannot detect minimum CMake version from $SOURCE_DIR/CMakeLists.txt" >&2
        exit 1
    fi

    printf '%s\n' "$version"
}

required_qt_version() {
    local version

    version="$(sed -nE 's/^[[:space:]]*find_package[[:space:]]*\([[:space:]]*Qt6[[:space:]]+([0-9]+(\.[0-9]+){1,3}).*/\1/p' \
        "$SOURCE_DIR/cmake/qt.cmake" | head -n1)"
    if [ -z "$version" ]; then
        echo "Error: cannot detect minimum Qt version from $SOURCE_DIR/cmake/qt.cmake" >&2
        exit 1
    fi

    printf '%s\n' "$version"
}

cmake_version() {
    "$1" --version 2>/dev/null | head -n1 | extract_version || true
}

ensure_python_tools() {
    if [ -x "$TOOLS_DIR/aqt-venv/bin/python3" ]; then
        return
    fi

    mkdir -p "$TOOLS_DIR"
    python3 -m venv "$TOOLS_DIR/aqt-venv"
    "$TOOLS_DIR/aqt-venv/bin/python3" -m pip install --upgrade pip
}

ensure_cmake() {
    local required="$1"
    local found

    if command -v cmake >/dev/null 2>&1; then
        found="$(cmake_version "$(command -v cmake)")"
        if [ -n "$found" ] && version_ge "$found" "$required"; then
            CMAKE_BIN="$(command -v cmake)"
            echo "Using CMake $found from $CMAKE_BIN"
            return
        fi
        echo "Found CMake ${found:-unknown}, but $required or newer is required."
    fi

    ensure_python_tools
    "$TOOLS_DIR/aqt-venv/bin/python3" -m pip install --upgrade cmake
    CMAKE_BIN="$TOOLS_DIR/aqt-venv/bin/cmake"
    found="$(cmake_version "$CMAKE_BIN")"

    if [ -z "$found" ] || ! version_ge "$found" "$required"; then
        echo "Error: cannot install CMake $required or newer." >&2
        exit 1
    fi

    echo "Using local CMake $found from $CMAKE_BIN"
}

download_file() {
    local url="$1"
    local output="$2"

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail "$url" -o "$output"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$output" "$url"
    else
        echo "Error: curl or wget is required to download $url" >&2
        exit 1
    fi
}

run_as_root() {
    if [ "$EUID" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        su -c "$(printf '%q ' "$@")"
    fi
}

detect_linux_distro() {
    local ID=""
    local ID_LIKE=""

    if [ ! -f /etc/os-release ]; then
        echo "Error: cannot detect Linux distribution." >&2
        exit 1
    fi

    . /etc/os-release

    DISTRO=""
    case "${ID:-}" in
        debian|ubuntu|linuxmint|zorin|astra)
            DISTRO="debian"
            ;;
        fedora|rhel|rocky|almalinux|redos)
            DISTRO="rhel"
            ;;
        altlinux)
            DISTRO="altlinux"
            ;;
        opensuse*|suse|sles)
            DISTRO="suse"
            ;;
        arch|endeavouros|manjaro)
            DISTRO="arch"
            ;;
    esac

    if [ -z "$DISTRO" ]; then
        case " ${ID_LIKE:-} " in
            *" debian "*)
                DISTRO="debian"
                ;;
            *" rhel "*|*" fedora "*)
                DISTRO="rhel"
                ;;
            *" suse "*)
                DISTRO="suse"
                ;;
            *" arch "*)
                DISTRO="arch"
                ;;
        esac
    fi

    if [ -z "$DISTRO" ]; then
        echo "Error: unsupported Linux distribution: ${ID:-unknown}" >&2
        exit 1
    fi
}

linux_general_packages() {
    case "$DISTRO" in
        debian)
            echo "build-essential cmake ninja-build git curl tar gzip pkg-config libgl1-mesa-dev libxcb-cursor-dev libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 libxcb-util1 libssl-dev python3 python3-pip python3-venv"
            ;;
        rhel)
            echo "gcc gcc-c++ cmake ninja-build git curl tar gzip pkgconf-pkg-config mesa-libGL-devel xcb-util-cursor-devel xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util libxkbcommon-x11 openssl-devel python3 python3-pip"
            ;;
        altlinux)
            echo "gcc gcc-c++ cmake ninja-build git curl tar gzip pkg-config libGL-devel libxcbutil-cursor libxcbutil-wm libxcbutil-image libxcbutil-keysyms libxcbutil-renderutil libxcbutil libxkbcommon openssl-devel python3 python3-module-pip python3-module-venv"
            ;;
        suse)
            echo "gcc gcc-c++ cmake ninja git curl tar gzip pkg-config Mesa-libGL-devel xcb-util-cursor-devel libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 libxcb-util1 libxkbcommon-x11-0 libopenssl-devel python3 python3-pip python3-venv"
            ;;
        arch)
            echo "base-devel cmake ninja git curl tar gzip pkgconf mesa libxcb xcb-util-cursor xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util libxkbcommon openssl python python-pip"
            ;;
    esac
}

linux_qt6_packages() {
    case "$DISTRO" in
        debian)
            echo "qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools libqt6svg6-dev qt6-charts-dev"
            ;;
        rhel)
            echo "qt6-qtbase-devel qt6-qttools-devel qt6-qtsvg-devel qt6-qtcharts-devel"
            ;;
        altlinux)
            echo "qt6-base-devel qt6-tools-devel qt6-svg-devel qt6-charts-devel"
            ;;
        suse)
            echo "qt6-base-devel qt6-tools-devel qt6-svg-devel qt6-charts-devel"
            ;;
        arch)
            echo "qt6-base qt6-tools qt6-svg qt6-charts"
            ;;
    esac
}

package_installed() {
    local package="$1"

    case "$DISTRO" in
        debian|altlinux)
            dpkg -s "$package" >/dev/null 2>&1 || rpm -q "$package" >/dev/null 2>&1
            ;;
        rhel|suse)
            rpm -q "$package" >/dev/null 2>&1
            ;;
        arch)
            pacman -Qq "$package" >/dev/null 2>&1
            ;;
    esac
}

install_packages() {
    local missing=()
    local package

    for package in "$@"; do
        if package_installed "$package"; then
            echo "Found package: $package"
        else
            missing+=("$package")
        fi
    done

    if [ "${#missing[@]}" -eq 0 ]; then
        return
    fi

    echo "Installing packages: ${missing[*]}"
    case "$DISTRO" in
        debian)
            run_as_root apt-get update
            run_as_root apt-get install -y "${missing[@]}"
            ;;
        rhel)
            run_as_root dnf install -y "${missing[@]}"
            ;;
        altlinux)
            run_as_root apt-get install -y "${missing[@]}"
            ;;
        suse)
            run_as_root zypper --non-interactive install "${missing[@]}"
            ;;
        arch)
            run_as_root pacman -S --needed --noconfirm "${missing[@]}"
            ;;
    esac
}

repo_qt6_probe_package() {
    case "$DISTRO" in
        debian)
            echo "qt6-base-dev"
            ;;
        rhel)
            echo "qt6-qtbase-devel"
            ;;
        altlinux)
            echo "qt6-base-devel"
            ;;
        suse)
            echo "qt6-base-devel"
            ;;
        arch)
            echo "qt6-base"
            ;;
    esac
}

repo_qt6_version() {
    local package
    local version

    package="$(repo_qt6_probe_package)"
    case "$DISTRO" in
        debian|altlinux)
            version="$(apt-cache policy "$package" 2>/dev/null | sed -nE 's/^[[:space:]]*Candidate:[[:space:]]*([^[:space:]]+).*/\1/p' | extract_version)"
            ;;
        rhel)
            version="$(dnf repoquery --qf '%{version}' --latest-limit=1 "$package" 2>/dev/null | extract_version)"
            if [ -z "$version" ]; then
                version="$(dnf list --available "$package" 2>/dev/null | awk 'NR > 1 { print $2; exit }' | extract_version)"
            fi
            ;;
        suse)
            version="$(zypper --non-interactive info "$package" 2>/dev/null | sed -nE 's/^Version[[:space:]]*:[[:space:]]*([^[:space:]]+).*/\1/p' | extract_version)"
            ;;
        arch)
            version="$(pacman -Si "$package" 2>/dev/null | sed -nE 's/^Version[[:space:]]*:[[:space:]]*([^[:space:]]+).*/\1/p' | extract_version)"
            ;;
    esac

    printf '%s\n' "$version"
}

qt_version_from_prefix() {
    local prefix="$1"
    local qmake

    for qmake in "$prefix/bin/qmake6" "$prefix/bin/qmake" "$prefix/bin/qtpaths6"; do
        if [ -x "$qmake" ]; then
            case "$(basename "$qmake")" in
                qtpaths6)
                    "$qmake" --qt-version 2>/dev/null || "$qmake" --version 2>/dev/null | extract_version
                    ;;
                *)
                    "$qmake" -query QT_VERSION 2>/dev/null
                    ;;
            esac
            return
        fi
    done
}

qt_prefix_from_system() {
    local prefix
    local tool
    local config

    if command -v pkg-config >/dev/null 2>&1; then
        prefix="$(pkg-config --variable=prefix Qt6Core 2>/dev/null || true)"
        if [ -n "$prefix" ]; then
            echo "$prefix"
            return
        fi
    fi

    for tool in qmake6 qmake qtpaths6; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            continue
        fi

        case "$tool" in
            qtpaths6)
                prefix="$("$tool" --install-prefix 2>/dev/null || true)"
                ;;
            *)
                prefix="$("$tool" -query QT_INSTALL_PREFIX 2>/dev/null || true)"
                ;;
        esac

        if [ -n "$prefix" ]; then
            echo "$prefix"
            return
        fi
    done

    config="$(find /usr /usr/local -type f -name Qt6CoreConfig.cmake 2>/dev/null | head -n1 || true)"
    if [ -n "$config" ]; then
        dirname "$(dirname "$config")"
    fi
}

aqt_arch_candidates() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "linux_gcc_64 gcc_64"
            ;;
        aarch64|arm64)
            echo "linux_gcc_arm64"
            ;;
        *)
            echo "Error: unsupported Linux architecture for aqtinstall: $(uname -m)" >&2
            exit 1
            ;;
    esac
}

aqt_arch() {
    local version="$1"
    local available
    local candidate

    available="$("$TOOLS_DIR/aqt-venv/bin/python3" -m aqt list-qt linux desktop --arch "$version" 2>/dev/null \
        | tr ' ' '\n')"

    for candidate in $(aqt_arch_candidates); do
        if printf '%s\n' "$available" | grep -Fxq "$candidate"; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

qt_release_has_final_tag() {
    local version="$1"

    git ls-remote --exit-code --tags https://code.qt.io/qt/qtbase.git \
        "refs/tags/v${version}" >/dev/null 2>&1
}

aqt_qt6_versions() {
    local versions
    local version
    local required="$1"

    versions="$("$TOOLS_DIR/aqt-venv/bin/python3" -m aqt list-qt linux desktop 2>/dev/null \
        | tr ' ' '\n' | sed -nE '/^6\.[0-9]+\.[0-9]+$/p' | sort -Vr)"

    for version in $versions; do
        if version_ge "$version" "$required" && [ -n "$(aqt_arch "$version")" ]; then
            echo "$version"
        fi
    done
}

aqt_prefix_in() {
    local base="$1"
    local qmake

    for qmake in "$base"/*/bin/qmake6 "$base"/*/bin/qmake; do
        if [ -x "$qmake" ]; then
            dirname "$(dirname "$qmake")"
            return
        fi
    done
}

install_qt6_with_aqt() {
    local required="$1"
    local arch
    local version
    local root="$TOOLS_DIR/qt"
    local installed=0

    ensure_python_tools
    "$TOOLS_DIR/aqt-venv/bin/python3" -m pip install --upgrade aqtinstall

    QT_FROM_AQT=1

    for version in $(aqt_qt6_versions "$required"); do
        if ! qt_release_has_final_tag "$version"; then
            echo "Skipping Qt $version (no final release tag; beta/RC)." >&2
            continue
        fi

        QT_PREFIX="$(aqt_prefix_in "$root/$version")"
        if [ -n "$QT_PREFIX" ]; then
            installed=1
            break
        fi

        arch="$(aqt_arch "$version")"
        echo "Installing Qt $version for $arch with aqtinstall..."
        if "$TOOLS_DIR/aqt-venv/bin/python3" -m aqt install-qt linux desktop "$version" "$arch" \
            -O "$root" -m qtcharts; then
            QT_PREFIX="$(aqt_prefix_in "$root/$version")"
            if [ -n "$QT_PREFIX" ]; then
                installed=1
                break
            fi
        fi
        echo "Qt $version is not installable with the required modules, trying an older version..." >&2
    done

    if [ "$installed" -eq 0 ]; then
        echo "Error: cannot install Qt 6 >= $required with the required modules using aqtinstall." >&2
        exit 1
    fi

    QT_VERSION="$(qt_version_from_prefix "$QT_PREFIX")"
}

configure_linux_qt() {
    local required="$1"
    local repo_version
    local -a general_packages
    local -a qt_packages

    read -r -a general_packages <<<"$(linux_general_packages)"
    install_packages "${general_packages[@]}"

    repo_version="$(repo_qt6_version)"
    if [ -n "$repo_version" ] && version_ge "$repo_version" "$required"; then
        echo "Repository Qt $repo_version satisfies minimum Qt $required."
        QT_FROM_AQT=0
        read -r -a qt_packages <<<"$(linux_qt6_packages)"
        install_packages "${qt_packages[@]}"
        QT_PREFIX="$(qt_prefix_from_system)"
        QT_VERSION="$(qt_version_from_prefix "$QT_PREFIX")"
    else
        if [ -n "$repo_version" ]; then
            echo "Repository Qt $repo_version is older than required Qt $required."
        else
            echo "Repository Qt 6 packages were not found."
        fi
        install_qt6_with_aqt "$required"
    fi

    if [ -z "$QT_PREFIX" ] || [ -z "$QT_VERSION" ]; then
        echo "Error: cannot detect Qt installation." >&2
        exit 1
    fi

    if ! version_ge "$QT_VERSION" "$required"; then
        echo "Error: Qt $QT_VERSION is older than required Qt $required." >&2
        exit 1
    fi

    echo "Using Qt $QT_VERSION from $QT_PREFIX"
}

qmake_from_qt_prefix() {
    local qmake

    for qmake in "$QT_PREFIX/bin/qmake6" "$QT_PREFIX/bin/qmake"; do
        if [ -x "$qmake" ]; then
            echo "$qmake"
            return
        fi
    done

    echo "Error: qmake not found in $QT_PREFIX/bin" >&2
    exit 1
}

linuxdeployqt_arch() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "x86_64"
            ;;
        *)
            echo "Error: unsupported architecture for linuxdeployqt: $(uname -m)" >&2
            exit 1
            ;;
    esac
}

ensure_linuxdeployqt() {
    local arch
    local appimage
    local appimage_dir
    local extract_dir

    arch="$(linuxdeployqt_arch)"
    appimage_dir="$TOOLS_DIR/linuxdeployqt"
    appimage="$appimage_dir/linuxdeployqt-continuous-$arch.AppImage"
    extract_dir="$appimage_dir/squashfs-root-$arch"

    if [ -x "$extract_dir/AppRun" ]; then
        echo "$extract_dir/AppRun"
        return
    fi

    mkdir -p "$appimage_dir"
    if [ ! -f "$appimage" ]; then
        download_file \
            "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-$arch.AppImage" \
            "$appimage"
    fi
    chmod +x "$appimage"

    if "$appimage" --version >/dev/null 2>&1; then
        echo "$appimage"
        return
    fi

    (cd "$appimage_dir" && "./$(basename "$appimage")" --appimage-extract >/dev/null)
    rm -rf "$extract_dir"
    mv "$appimage_dir/squashfs-root" "$extract_dir"
    echo "$extract_dir/AppRun"
}

deploy_linux_with_linuxdeployqt() {
    local appdir="$1"
    local build_dir="$2"
    local app="$appdir/usr/bin/ouaexp"
    local qmake
    local linuxdeployqt
    local opcua_plugin

    if [ ! -x "$app" ]; then
        echo "Error: installed executable not found: $app" >&2
        exit 1
    fi

    qmake="$(qmake_from_qt_prefix)"
    linuxdeployqt="$(ensure_linuxdeployqt)"

    "$linuxdeployqt" "$app" \
        -qmake="$qmake" \
        -extra-plugins=iconengines,platformthemes

    # Qt OpcUa is built from source into the CMake build tree, so its backend plugin
    # is not in Qt's plugin dir and linuxdeployqt cannot find it via -extra-plugins.
    # Copy it into the deployed plugin dir by hand; its Qt dependencies (libQt6OpcUa
    # and friends) are already bundled through the application binary.
    opcua_plugin="$build_dir/_deps/qtopcua-install/plugins/opcua"
    if [ -d "$opcua_plugin" ]; then
        mkdir -p "$appdir/usr/plugins"
        cp -a "$opcua_plugin" "$appdir/usr/plugins/"
        echo "Copied Qt OpcUa plugin into $appdir/usr/plugins/opcua"
    else
        echo "Warning: Qt OpcUa plugin not found at $opcua_plugin" >&2
    fi

    echo "Deployed Linux AppDir into $appdir"
}

default_install_prefix() {
    case "$(uname -s)" in
        Linux)
            if [ "$QT_FROM_AQT" -eq 1 ]; then
                echo "$1/install"
            else
                echo "/usr/local"
            fi
            ;;
        Darwin)
            echo "/Applications"
            ;;
    esac
}

install_project() {
    local build_dir="$1"
    local prefix="$INSTALL_PREFIX"

    if [ -z "$prefix" ]; then
        prefix="$(default_install_prefix "$build_dir")"
    fi

    echo ""
    echo "Installing into $prefix"
    if [ "$(uname -s)" = "Linux" ] && [ "$QT_FROM_AQT" -eq 1 ]; then
        mkdir -p "$prefix"
        DESTDIR="$prefix" "$CMAKE_BIN" --install "$build_dir" --prefix /usr
        deploy_linux_with_linuxdeployqt "$prefix" "$build_dir"
    elif [ "$(uname -s)" = "Linux" ] && [ "$QT_FROM_AQT" -eq 0 ] && [ ! -w "$prefix" ]; then
        run_as_root "$CMAKE_BIN" --install "$build_dir" --prefix "$prefix"
    else
        "$CMAKE_BIN" --install "$build_dir" --prefix "$prefix"
    fi
}

build_project() {
    local compiler="$1"
    local qt_version_safe
    local arch
    local build_dir
    local cmake_args
    local build_tests

    qt_version_safe="$(printf '%s' "$QT_VERSION" | tr '. ' '__')"
    arch="$(uname -m)"
    build_dir="$PROJECT_DIR/build-ouaexp-Qt_${qt_version_safe}_${compiler}_${arch}-${BUILD_TYPE}"
    build_tests=OFF
    if [ "$RUN_TESTS" -eq 1 ]; then
        build_tests=ON
    fi

    echo ""
    echo "Configuring build in $build_dir"
    cmake_args=(
        -S "$SOURCE_DIR"
        -B "$build_dir"
        -GNinja
        -DCMAKE_PREFIX_PATH="$QT_PREFIX"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DOUAEXP_BUILD_TESTS="$build_tests"
    )
    if [ -n "${OPENSSL_ROOT_DIR:-}" ]; then
        cmake_args+=("-DOPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR")
    fi

    "$CMAKE_BIN" "${cmake_args[@]}"

    "$CMAKE_BIN" --build "$build_dir" --parallel
    if [ "$RUN_TESTS" -eq 1 ]; then
        "$CMAKE_BIN" --build "$build_dir" --parallel --target ouaexp_check
    fi
    if [ "$RUN_INSTALL" -eq 1 ]; then
        install_project "$build_dir"
    fi
    echo ""
    echo "Build finished successfully in $build_dir"
}

build_linux() {
    local min_cmake
    local min_qt

    detect_linux_distro
    min_cmake="$(required_cmake_version)"
    min_qt="$(required_qt_version)"

    configure_linux_qt "$min_qt"
    ensure_cmake "$min_cmake"

    build_project "gcc"
}

brew_install_or_upgrade() {
    local formula

    for formula in "$@"; do
        if brew list --versions "$formula" >/dev/null 2>&1; then
            if brew outdated --quiet "$formula" | grep -Fxq "$formula"; then
                brew upgrade "$formula"
            fi
        else
            brew install "$formula"
        fi
    done
}

homebrew_cmake_bin() {
    local cmake_prefix
    local cmake_bin

    cmake_prefix="$(brew --prefix cmake 2>/dev/null || true)"
    if [ -n "$cmake_prefix" ]; then
        cmake_bin="$cmake_prefix/bin/cmake"
        if [ -x "$cmake_bin" ]; then
            echo "$cmake_bin"
            return
        fi
    fi

    if command -v cmake >/dev/null 2>&1; then
        command -v cmake
        return
    fi

    echo "Error: CMake executable was not found after Homebrew installation." >&2
    exit 1
}

build_macos() {
    local min_cmake
    local min_qt
    local cmake_found
    local openssl_prefix

    if ! xcode-select -p >/dev/null 2>&1; then
        echo "Error: Xcode Command Line Tools not found. Install them with: xcode-select --install" >&2
        exit 1
    fi
    if ! command -v brew >/dev/null 2>&1; then
        echo "Error: Homebrew not found. Install it from https://brew.sh" >&2
        exit 1
    fi

    min_cmake="$(required_cmake_version)"
    min_qt="$(required_qt_version)"

    brew_install_or_upgrade cmake ninja qt openssl@3

    CMAKE_BIN="$(homebrew_cmake_bin)"
    cmake_found="$(cmake_version "$CMAKE_BIN")"
    if [ -z "$cmake_found" ] || ! version_ge "$cmake_found" "$min_cmake"; then
        echo "Error: CMake $min_cmake or newer is required, found ${cmake_found:-unknown} at $CMAKE_BIN." >&2
        exit 1
    fi

    QT_PREFIX="$(brew --prefix qt 2>/dev/null)"
    QT_VERSION="$(qt_version_from_prefix "$QT_PREFIX")"
    if [ -z "$QT_VERSION" ] || ! version_ge "$QT_VERSION" "$min_qt"; then
        echo "Error: Qt $min_qt or newer is required, found ${QT_VERSION:-unknown}." >&2
        exit 1
    fi

    openssl_prefix="$(brew --prefix openssl@3 2>/dev/null || true)"
    if [ -n "$openssl_prefix" ]; then
        export OPENSSL_ROOT_DIR="$openssl_prefix"
    fi

    echo "Using Qt $QT_VERSION from $QT_PREFIX"
    echo "Using CMake $(cmake_version "$CMAKE_BIN") from $CMAKE_BIN"
    build_project "clang"
}

main() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            -h|--help)
                usage
                return 0
                ;;
            --tests)
                RUN_TESTS=1
                ;;
            --install)
                RUN_INSTALL=1
                ;;
            --install=*)
                RUN_INSTALL=1
                INSTALL_PREFIX="${1#--install=}"
                if [ -z "$INSTALL_PREFIX" ]; then
                    echo "Error: --install prefix must not be empty" >&2
                    return 1
                fi
                ;;
            *)
                echo "Error: unknown argument: $1" >&2
                usage >&2
                return 1
                ;;
        esac
        shift
    done

    print_banner

    case "$(uname -s)" in
        Linux)
            build_linux
            ;;
        Darwin)
            build_macos
            ;;
        *)
            echo "Error: unsupported OS: $(uname -s)" >&2
            return 1
            ;;
    esac
}

main "$@"
