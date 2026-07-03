#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$PROJECT_DIR/src"
TOOLS_DIR="$PROJECT_DIR/.build-tools"
BUILD_TYPE="${BUILD_TYPE:-Release}"

QT_PREFIX=""
QT_VERSION=""
CMAKE_BIN=""

usage() {
    echo "Usage: ./build.sh"
    echo ""
    echo "Environment:"
    echo "  BUILD_TYPE=Release|Debug|RelWithDebInfo"
}

case "${1:-}" in
    -h|--help)
        usage
        exit 0
        ;;
esac

echo "================================"
echo " OpenUaExplorer build script"
echo "================================"
echo ""

version_part() {
    local version="$1"
    local index="$2"
    local part

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
    sed -nE 's/.*([0-9]+(\.[0-9]+){1,3}).*/\1/p' | head -n1
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
            echo "build-essential cmake ninja-build git curl tar gzip pkg-config libgl1-mesa-dev libxcb-cursor-dev libssl-dev python3 python3-pip python3-venv"
            ;;
        rhel)
            echo "gcc gcc-c++ cmake ninja-build git curl tar gzip pkgconf-pkg-config mesa-libGL-devel xcb-util-cursor-devel openssl-devel python3 python3-pip"
            ;;
        altlinux)
            echo "gcc gcc-c++ cmake ninja-build git curl tar gzip pkg-config libGL-devel libxcbutil-cursor openssl-devel python3 python3-module-pip python3-module-venv"
            ;;
        suse)
            echo "gcc gcc-c++ cmake ninja git curl tar gzip pkg-config Mesa-libGL-devel xcb-util-cursor-devel libopenssl-devel python3 python3-pip python3-venv"
            ;;
        arch)
            echo "base-devel cmake ninja git curl tar gzip pkgconf mesa libxcb xcb-util-cursor openssl python python-pip"
            ;;
    esac
}

linux_qt6_packages() {
    case "$DISTRO" in
        debian)
            echo "qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools libqt6svg6-dev"
            ;;
        rhel)
            echo "qt6-qtbase-devel qt6-qttools-devel qt6-qtsvg-devel"
            ;;
        altlinux)
            echo "qt6-base-devel qt6-tools-devel qt6-svg-devel"
            ;;
        suse)
            echo "qt6-base-devel qt6-tools-devel qt6-svg-devel"
            ;;
        arch)
            echo "qt6-base qt6-tools qt6-svg"
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

aqt_arch() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "gcc_64"
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

latest_aqt_qt6_version() {
    local versions
    local version
    local latest=""
    local required="$1"

    versions="$("$TOOLS_DIR/aqt-venv/bin/python3" -m aqt list-qt linux desktop 2>/dev/null \
        | tr ' ' '\n' | sed -nE '/^6\.[0-9]+\.[0-9]+$/p')"

    for version in $versions; do
        if version_ge "$version" "$required" && { [ -z "$latest" ] || version_ge "$version" "$latest"; }; then
            latest="$version"
        fi
    done

    if [ -z "$latest" ]; then
        echo "Error: cannot find a Qt 6 version >= $required with aqtinstall." >&2
        exit 1
    fi

    echo "$latest"
}

install_qt6_with_aqt() {
    local required="$1"
    local arch
    local version
    local root="$TOOLS_DIR/qt"

    ensure_python_tools
    "$TOOLS_DIR/aqt-venv/bin/python3" -m pip install --upgrade aqtinstall

    arch="$(aqt_arch)"
    version="$(latest_aqt_qt6_version "$required")"
    QT_PREFIX="$root/$version/$arch"

    if [ ! -x "$QT_PREFIX/bin/qmake6" ] && [ ! -x "$QT_PREFIX/bin/qmake" ]; then
        echo "Installing Qt $version for $arch with aqtinstall..."
        "$TOOLS_DIR/aqt-venv/bin/python3" -m aqt install-qt linux desktop "$version" "$arch" \
            -O "$root" -m qtsvg qttools
    fi

    QT_VERSION="$(qt_version_from_prefix "$QT_PREFIX")"
}

configure_linux_qt() {
    local required="$1"
    local repo_version
    local packages

    install_packages $(linux_general_packages)

    repo_version="$(repo_qt6_version)"
    if [ -n "$repo_version" ] && version_ge "$repo_version" "$required"; then
        echo "Repository Qt $repo_version satisfies minimum Qt $required."
        packages="$(linux_qt6_packages)"
        install_packages $packages
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

build_project() {
    local compiler="$1"
    local qt_version_safe
    local arch
    local build_dir
    local cmake_args

    qt_version_safe="$(printf '%s' "$QT_VERSION" | tr '. ' '__')"
    arch="$(uname -m)"
    build_dir="$PROJECT_DIR/build-ouaexp-Qt_${qt_version_safe}_${compiler}_${arch}-${BUILD_TYPE}"

    echo ""
    echo "Configuring build in $build_dir"
    cmake_args=(
        -S "$SOURCE_DIR"
        -B "$build_dir"
        -GNinja
        -DCMAKE_PREFIX_PATH="$QT_PREFIX"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    )
    if [ -n "${OPENSSL_ROOT_DIR:-}" ]; then
        cmake_args+=("-DOPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR")
    fi

    "$CMAKE_BIN" "${cmake_args[@]}"

    "$CMAKE_BIN" --build "$build_dir" --parallel
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
    build_project "g++"
}

build_macos() {
    local min_cmake
    local min_qt
    local openssl_prefix
    local qt_formula

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

    if brew info qt@6 >/dev/null 2>&1; then
        qt_formula="qt@6"
    else
        qt_formula="qt"
    fi

    brew install cmake ninja "$qt_formula" openssl@3

    CMAKE_BIN="$(command -v cmake)"
    if ! version_ge "$(cmake_version "$CMAKE_BIN")" "$min_cmake"; then
        echo "Error: CMake $min_cmake or newer is required." >&2
        exit 1
    fi

    QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null)"
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

case "$(uname -s)" in
    Linux)
        build_linux
        ;;
    Darwin)
        build_macos
        ;;
    *)
        echo "Error: unsupported OS: $(uname -s)" >&2
        exit 1
        ;;
esac
