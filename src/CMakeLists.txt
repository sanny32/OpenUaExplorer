cmake_minimum_required(VERSION 3.22)

project(ouaexp
        VERSION 0.0.1
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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qlementine.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtopcua.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_version.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ui_normalization.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/coverage.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_windows.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_macos.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_linux.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake")

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
)

add_library(ouaexp_core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)
target_include_directories(ouaexp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)
target_link_libraries(ouaexp_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpcUa
)
ouaexp_configure_dependencies(ouaexp_core)
ouaexp_apply_coverage(ouaexp_core)
if(MSVC)
    target_compile_options(ouaexp_core PUBLIC /utf-8)
endif()

add_library(ouaexp_ui STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_FILES}
)
target_include_directories(ouaexp_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)
target_link_libraries(ouaexp_ui PUBLIC ouaexp_core)
ouaexp_configure_ui_enum_normalization(ouaexp_ui)
ouaexp_configure_qt_target(ouaexp_ui)
ouaexp_configure_qlementine_app_style(ouaexp_ui)
if(MSVC)
    target_compile_options(ouaexp_ui PUBLIC /utf-8)
endif()

add_executable(${PROJECT_NAME}
    main.cpp
    ${RESOURCE_FILES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE ouaexp_ui)
ouaexp_copy_runtime(${PROJECT_NAME})
ouaexp_configure_tests()

if(WIN32)
    ouaexp_configure_target_windows(${PROJECT_NAME})
elseif(APPLE)
    ouaexp_configure_target_macos(${PROJECT_NAME})
elseif(LINUX)
    ouaexp_configure_target_linux(${PROJECT_NAME})
endif()

ouaexp_configure_uninstall_target()

if(WIN32)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_windows.cmake")
elseif(APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_macos.cmake")
elseif(LINUX)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_linux.cmake")
endif()
