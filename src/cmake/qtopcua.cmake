include(FetchContent)

function(ouaexp_copy_qtopcua_runtime target_name)
    if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::OpcUa>"
                "$<TARGET_FILE_DIR:${target_name}>"
            VERBATIM
        )
    endif()

    if(TARGET Qt${QT_VERSION_MAJOR}::QOpen62541Plugin)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>/opcua"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QOpen62541Plugin>"
                "$<TARGET_FILE_DIR:${target_name}>/opcua"
            VERBATIM
        )
    endif()
endfunction()

option(OUAEXP_FETCH_QTOPCUA
    "Fetch and build Qt OpcUa when it is not installed" ON)

find_package(Qt${QT_VERSION_MAJOR}OpcUa CONFIG QUIET)

if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa OR NOT OUAEXP_FETCH_QTOPCUA)
    return()
endif()

if(NOT QT_VERSION_MAJOR EQUAL 6)
    message(WARNING
        "Automatic Qt OpcUa bootstrap is available only for Qt 6. "
        "Install Qt5OpcUa for the selected Qt 5 kit.")
    return()
endif()

set(QTOPCUA_VERSION "${Qt6Core_VERSION}")
set(QTOPCUA_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-build")
set(QTOPCUA_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-install")
set(QTOPCUA_CONFIG_FILE
    "${QTOPCUA_INSTALL_DIR}/lib/cmake/Qt6OpcUa/Qt6OpcUaConfig.cmake")

FetchContent_Declare(qtopcua
    GIT_REPOSITORY https://code.qt.io/qt/qtopcua.git
    GIT_TAG "v${QTOPCUA_VERSION}"
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(qtopcua)
if(NOT qtopcua_POPULATED)
    FetchContent_Populate(qtopcua)
endif()

if(NOT EXISTS "${QTOPCUA_CONFIG_FILE}")
    find_program(QTOPCUA_CONFIGURE_EXECUTABLE
        NAMES qt-configure-module qt-configure-module.bat
        HINTS "${QT_BINARY_DIR}"
        REQUIRED
    )

    file(MAKE_DIRECTORY "${QTOPCUA_BUILD_DIR}")

    set(QTOPCUA_CONFIGURE_OPTIONS
        -DINPUT_open62541=qt
        -DQT_BUILD_TESTS=OFF
        -DQT_BUILD_EXAMPLES=OFF
        "-DCMAKE_INSTALL_PREFIX=${QTOPCUA_INSTALL_DIR}"
    )
    if(OUAEXP_OPENSSL_ROOT_DIR)
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DOPENSSL_ROOT_DIR=${OUAEXP_OPENSSL_ROOT_DIR}")
    endif()

    message(STATUS
        "Configuring Qt OpcUa ${QTOPCUA_VERSION} with the bundled open62541 backend")
    execute_process(
        COMMAND "${QTOPCUA_CONFIGURE_EXECUTABLE}"
            "${qtopcua_SOURCE_DIR}"
            --
            ${QTOPCUA_CONFIGURE_OPTIONS}
        WORKING_DIRECTORY "${QTOPCUA_BUILD_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "Building and installing Qt OpcUa ${QTOPCUA_VERSION}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            --build "${QTOPCUA_BUILD_DIR}"
            --config RelWithDebInfo
            --target install
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(Qt6OpcUa_DIR "${QTOPCUA_INSTALL_DIR}/lib/cmake/Qt6OpcUa"
    CACHE PATH "Directory containing Qt6OpcUaConfig.cmake" FORCE)
find_package(Qt6OpcUa CONFIG REQUIRED)
