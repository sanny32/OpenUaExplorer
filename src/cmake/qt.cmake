set(QT_COMMON_COMPONENTS Core Gui Widgets Svg Network)

find_package(Qt6 6.9 COMPONENTS ${QT_COMMON_COMPONENTS} REQUIRED)
set(QT_VERSION_MAJOR 6)

if(OUAEXP_BUILD_TESTS)
    find_package(Qt6 COMPONENTS Test REQUIRED)
endif()

if(LINUX)
    find_package(Qt6 COMPONENTS DBus QUIET)
endif()

get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BINARY_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

get_filename_component(QT_INSTALL_LIBEXECS "${QT6_INSTALL_LIBEXECS}"
    ABSOLUTE BASE_DIR "/usr")
get_filename_component(QT_LIBEXEC_DIR "${QT_BINARY_DIR}/../libexec" REALPATH)

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
        Qt${QT_VERSION_MAJOR}::Network
    )

    target_link_libraries(${target_name} PRIVATE Qt${QT_VERSION_MAJOR}::OpcUa)

    if(LINUX AND Qt6DBus_FOUND)
        target_link_libraries(${target_name} PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
        target_compile_definitions(${target_name} PRIVATE HAS_QTDBUS)
    endif()
endfunction()
