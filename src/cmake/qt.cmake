option(USE_QT5 "Force Qt5 usage" OFF)
option(USE_QT6 "Force Qt6 usage" OFF)

set(QT_COMMON_COMPONENTS Core Gui Widgets Svg)

if(USE_QT5 AND USE_QT6)
    message(FATAL_ERROR "USE_QT5 and USE_QT6 are mutually exclusive")
endif()

if(USE_QT5)
    find_package(Qt5 5.15.2 COMPONENTS ${QT_COMMON_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR 5)
elseif(USE_QT6)
    find_package(Qt6 COMPONENTS ${QT_COMMON_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt6 COMPONENTS ${QT_COMMON_COMPONENTS} QUIET)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    else()
        find_package(Qt5 5.15.2 COMPONENTS ${QT_COMMON_COMPONENTS} REQUIRED)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

if(LINUX)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET)
endif()

if(NOT DEFINED QT_VERSION_MAJOR)
    message(FATAL_ERROR
        "One or more Qt development packages not found. "
        "Please install Qt 5.15.2+ or Qt6 development packages.")
endif()

get_target_property(QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(QT_BINARY_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

if(QT_VERSION_MAJOR EQUAL 6)
    get_filename_component(QT_INSTALL_LIBEXECS "${QT6_INSTALL_LIBEXECS}"
        ABSOLUTE BASE_DIR "/usr")
    get_filename_component(QT_LIBEXEC_DIR "${QT_BINARY_DIR}/../libexec" REALPATH)
else()
    set(QT_LIBEXEC_DIR "${QT_BINARY_DIR}")
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "windeployqt found at: ${WINDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "windeployqt not found - Windows install will not deploy Qt runtime")
    endif()
endif()

if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
        HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "macdeployqt found at: ${MACDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "macdeployqt not found - macOS install will not deploy Qt runtime")
    endif()
endif()

function(ouaexp_configure_qt_target target_name)
    target_link_libraries(${target_name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
    )

    if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa)
        target_link_libraries(${target_name} PRIVATE Qt${QT_VERSION_MAJOR}::OpcUa)
        target_compile_definitions(${target_name} PRIVATE OUAEXP_HAS_OPCUA)
    else()
        message(WARNING
            "Qt OpcUa was not found. The application will build without OPC UA runtime support.")
    endif()

    if(LINUX AND (Qt6DBus_FOUND OR Qt5DBus_FOUND))
        target_link_libraries(${target_name} PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
        target_compile_definitions(${target_name} PRIVATE HAS_QTDBUS)
    endif()
endfunction()
