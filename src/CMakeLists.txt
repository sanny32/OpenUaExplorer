cmake_minimum_required(VERSION 3.22)

project(ouaexp
        VERSION 1.0.0
        DESCRIPTION "An OPC UA client application for exploring and monitoring OPC UA servers."
        LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(PRODUCT_NAME "Open UaExplorer")
set(ORGANIZATION_NAME "OpenUaExplorer contributers")

include("${PROJECT_SOURCE_DIR}/cmake/qt.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/qtopcua.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/project_version.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/coverage.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/uninstall.cmake")

enable_testing()

ouaexp_apply_project_version_suffix()

add_compile_definitions(
    APP_NAME="${PRODUCT_NAME}"
    APP_ORGANIZATION_NAME="${ORGANIZATION_NAME}"
    APP_PRODUCT_NAME="${PRODUCT_NAME}"
    APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
    APP_VERSION_MAJOR="${VERSION_MAJOR}"
    APP_VERSION_MINOR="${VERSION_MINOR}"
    APP_VERSION_PATCH="${VERSION_PATCH}"
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_YEAR="${CURRENT_YEAR}"
    OPENSSL_VERSION="${OPENSSL_VERSION}"
    QTKEYCHAIN_VERSION="${QTKEYCHAIN_VERSION}"
)

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(ui)
add_subdirectory(app)
add_subdirectory(tests)

ouaexp_configure_uninstall_target()

if(WIN32)
    include("${PROJECT_SOURCE_DIR}/cmake/install_windows.cmake")
elseif(APPLE)
    include("${PROJECT_SOURCE_DIR}/cmake/install_macos.cmake")
elseif(LINUX)
    include("${PROJECT_SOURCE_DIR}/cmake/install_linux.cmake")
endif()
