cmake_minimum_required(VERSION 3.16)

project(ouaexp
        VERSION 0.0.1
        DESCRIPTION "An OPC UA client application for exploring and monitoring OPC UA servers."
        LANGUAGES CXX)

include(GNUInstallDirs)

option(USE_QT5 "Force Qt5 usage" OFF)
option(USE_QT6 "Force Qt6 usage" OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(PRODUCT_NAME "Open UaExplorer")
set(PROJECT_BETARELEASE "" CACHE STRING "Beta release suffix, e.g. beta1")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

string(TIMESTAMP CURRENT_YEAR "%Y")

if(USE_QT5 AND USE_QT6)
    message(FATAL_ERROR "USE_QT5 and USE_QT6 are mutually exclusive")
endif()

if(USE_QT5)
    find_package(Qt5 5.15.2 COMPONENTS Core Gui Widgets Svg REQUIRED)
    set(QT_VERSION_MAJOR 5)
elseif(USE_QT6)
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg QUIET)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    else()
        find_package(Qt5 5.15.2 COMPONENTS Core Gui Widgets Svg REQUIRED)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()

if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(FATAL_ERROR "One or more Qt development packages not found. Please install Qt 5.15.2+ or Qt6 development packages.")
endif()

if(Qt6_FOUND)
    get_target_property(QT_BINARY_DIR Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    get_filename_component(QT_INSTALL_LIBEXECS "${QT6_INSTALL_LIBEXECS}" ABSOLUTE BASE_DIR "/usr")
    get_filename_component(QT_LIBEXEC_DIR "${QT_BINARY_DIR}/../libexec" REALPATH)
else()
    get_target_property(QT_BINARY_DIR Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    set(QT_LIBEXEC_DIR "${QT_BINARY_DIR}")
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "windeployqt found at: ${WINDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "windeployqt not found - Windows install will not deploy Qt runtime")
    endif()
endif()

if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "macdeployqt found at: ${MACDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "macdeployqt not found - macOS install will not deploy Qt runtime")
    endif()
endif()

add_executable(${PROJECT_NAME})

if(DEFINED ENV{GITHUB_REF})
    string(REGEX MATCH "refs/heads/(.*)" MATCH_RESULT $ENV{GITHUB_REF})
    if(MATCH_RESULT AND CMAKE_MATCH_1)
        set(GIT_BRANCH ${CMAKE_MATCH_1})
    endif()
else()
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    else()
        message(STATUS "Git not found - branch detection is not possible")
    endif()
endif()

if(DEFINED GIT_BRANCH AND "${GIT_BRANCH}" MATCHES "^(dev|release/.*)$")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}-dev)
elseif(PROJECT_BETARELEASE)
    set(PROJECT_VERSION ${PROJECT_VERSION}-${PROJECT_BETARELEASE})
endif()

add_compile_definitions(APP_NAME="${PRODUCT_NAME}")
add_compile_definitions(APP_PRODUCT_NAME="${PRODUCT_NAME}")
add_compile_definitions(APP_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_compile_definitions(APP_VERSION_MAJOR="${VERSION_MAJOR}")
add_compile_definitions(APP_VERSION_MINOR="${VERSION_MINOR}")
add_compile_definitions(APP_VERSION_PATCH="${VERSION_PATCH}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(BUILD_YEAR="${CURRENT_YEAR}")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    widgets/addressspacewidget.cpp
    widgets/attributeswidget.cpp
    widgets/connectionstatuswidget.cpp
    widgets/dataaccesswidget.cpp
    widgets/dataaccessmodel.cpp
    widgets/subscriptiondelegate.cpp
    widgets/endpointselectorwidget.cpp
    widgets/fixedgap.cpp
    widgets/headerview.cpp
    widgets/logwidget.cpp
    widgets/mainstatusbarwidget.cpp
    widgets/maintoolbar.cpp
    widgets/maintoolbutton.cpp
    widgets/securityselectorwidget.cpp
    widgets/trendpanelwidget.cpp
    widgets/trendgraphwidget.cpp
)

set(HEADERS
    mainwindow.h
    widgets/addressspacewidget.h
    widgets/attributeswidget.h
    widgets/connectionstatuswidget.h
    widgets/dataaccesswidget.h
    widgets/dataaccessitem.h
    widgets/dataaccessmodel.h
    widgets/subscriptiondelegate.h
    widgets/endpointselectorwidget.h
    widgets/fixedgap.h
    widgets/headerview.h
    widgets/logwidget.h
    widgets/mainstatusbarwidget.h
    widgets/maintoolbar.h
    widgets/maintoolbutton.h
    widgets/securityselectorwidget.h
    widgets/trendpanelwidget.h
    widgets/trendgraphwidget.h
)

set(UI_FILES
    mainwindow.ui
    widgets/addressspacewidget.ui
    widgets/attributeswidget.ui
    widgets/connectionstatuswidget.ui
    widgets/dataaccesswidget.ui
    widgets/endpointselectorwidget.ui
    widgets/logwidget.ui
    widgets/mainstatusbarwidget.ui
    widgets/securityselectorwidget.ui
    widgets/trendpanelwidget.ui
)

set(RESOURCE_FILES
    resources.qrc
)

set(UI_FILE_LIST "${CMAKE_CURRENT_BINARY_DIR}/ui_enum_files.txt")
set(UI_FILES_ABSOLUTE)
file(WRITE "${UI_FILE_LIST}" "")
foreach(UI_FILE IN LISTS UI_FILES)
    set(UI_FILE_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_FILE}")
    list(APPEND UI_FILES_ABSOLUTE "${UI_FILE_ABSOLUTE}")
    file(APPEND "${UI_FILE_LIST}" "${UI_FILE_ABSOLUTE}\n")
endforeach()

add_custom_target(normalize_ui_enums ALL
    COMMAND ${CMAKE_COMMAND}
            "-DUI_FILE_LIST=${UI_FILE_LIST}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeUiEnums.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeUiEnums.cmake"
        ${UI_FILES_ABSOLUTE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Normalizing Qt Designer UI enum names for Qt5 compatibility..."
    VERBATIM
)

add_dependencies(${PROJECT_NAME} normalize_ui_enums)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS normalize_ui_enums)

target_sources(${PROJECT_NAME} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

if(WIN32)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
elseif(UNIX)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    COMMENT "Uninstalling ${PROJECT_NAME}"
)

if(WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/${PRODUCT_NAME}" CACHE PATH "Install path prefix" FORCE)
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            set(_installed_exe \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe\")
            get_filename_component(_installed_exe_dir \"\${_installed_exe}\" DIRECTORY)
            set(_plugin_dir \"\${_installed_exe_dir}/plugins\")

            if(NOT EXISTS \"\${_installed_exe}\")
                message(FATAL_ERROR \"Installed executable not found: \${_installed_exe}\")
            endif()

            if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^[Dd]ebug$\")
                set(_deploy_mode --debug)
            else()
                set(_deploy_mode --release)
            endif()

            set(_windeploy_args
                \${_deploy_mode}
                --plugindir \"\${_plugin_dir}\"
                --no-opengl-sw
                --no-system-d3d-compiler
            )

            if(\"${Qt6_FOUND}\" STREQUAL \"TRUE\")
                list(APPEND _windeploy_args
                     --no-system-dxc-compiler
                     --skip-plugin-types generic,networkinformation,qmltooling,tls
                     --exclude-plugins qgif,qjpeg,qpdf,qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql)
            elseif(\"${Qt5_FOUND}\" STREQUAL \"TRUE\")
                list(APPEND _windeploy_args
                     --no-angle
                     --no-quick)
            endif()

            list(APPEND _windeploy_args \"\${_installed_exe}\")

            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \${_windeploy_args}
                RESULT_VARIABLE _windeployqt_result
            )

            if(NOT _windeployqt_result EQUAL 0)
                message(FATAL_ERROR \"windeployqt failed with exit code \${_windeployqt_result}\")
            endif()
        ")
    endif()
elseif(APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH "Install path prefix" FORCE)
    endif()

    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")

            if(NOT EXISTS \"\${_installed_app}\")
                message(FATAL_ERROR \"Installed app bundle not found: \${_installed_app}\")
            endif()

            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                        \"\${_installed_app}\"
                        -always-overwrite
                RESULT_VARIABLE _macdeployqt_result
            )

            if(NOT _macdeployqt_result EQUAL 0)
                message(FATAL_ERROR \"macdeployqt failed with exit code \${_macdeployqt_result}\")
            endif()
        ")
    endif()
elseif(UNIX)
    set(APP_EXECUTABLE_NAME "${PROJECT_NAME}")

    install(PROGRAMS "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION "${CMAKE_INSTALL_BINDIR}" RENAME "${APP_EXECUTABLE_NAME}")
endif()
