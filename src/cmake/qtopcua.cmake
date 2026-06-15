include(FetchContent)

function(ouaexp_copy_qtopcua_runtime target_name)
    if(WIN32)
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

if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa)
    return()
endif()

if(NOT OUAEXP_FETCH_QTOPCUA)
    find_package(Qt${QT_VERSION_MAJOR}OpcUa CONFIG REQUIRED)
    return()
endif()

set(QTOPCUA_VERSION "${Qt${QT_VERSION_MAJOR}Core_VERSION}")
set(QTOPCUA_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-build")
set(QTOPCUA_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-install")

FetchContent_Declare(qtopcua
    GIT_REPOSITORY https://code.qt.io/qt/qtopcua.git
    GIT_TAG "v${QTOPCUA_VERSION}"
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(qtopcua)
if(NOT qtopcua_POPULATED)
    FetchContent_Populate(qtopcua)
endif()

if(QT_VERSION_MAJOR EQUAL 5)
    set(QTOPCUA_QMAKE_EXECUTABLE "${QT_QMAKE_EXECUTABLE}")
    if(CMAKE_HOST_WIN32)
        find_program(QTOPCUA_MAKE_EXECUTABLE NAMES nmake REQUIRED)
        set(QTOPCUA_BUILD_OPTIONS)
    else()
        find_program(QTOPCUA_MAKE_EXECUTABLE NAMES make gmake REQUIRED)
        include(ProcessorCount)
        ProcessorCount(QTOPCUA_PROCESSOR_COUNT)
        if(NOT QTOPCUA_PROCESSOR_COUNT)
            set(QTOPCUA_PROCESSOR_COUNT 2)
        endif()
        set(QTOPCUA_BUILD_OPTIONS "-j${QTOPCUA_PROCESSOR_COUNT}")
    endif()

    file(MAKE_DIRECTORY "${QTOPCUA_BUILD_DIR}")
    set(QTOPCUA_ORIGINAL_INCLUDE "$ENV{INCLUDE}")
    if(CMAKE_HOST_WIN32 AND OUAEXP_OPENSSL_ROOT_DIR)
        file(TO_CMAKE_PATH "${OUAEXP_OPENSSL_ROOT_DIR}/include" _ssl_include)
        set(ENV{INCLUDE} "${_ssl_include};$ENV{INCLUDE}")
    endif()

    message(STATUS
        "Configuring Qt OpcUa ${QTOPCUA_VERSION} with the bundled open62541 backend")
    execute_process(
        COMMAND "${QTOPCUA_QMAKE_EXECUTABLE}"
            "${qtopcua_SOURCE_DIR}/qtopcua.pro"
            --
            --open62541=qt
        WORKING_DIRECTORY "${QTOPCUA_BUILD_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "Building and installing Qt OpcUa ${QTOPCUA_VERSION}")
    execute_process(
        COMMAND "${QTOPCUA_MAKE_EXECUTABLE}" ${QTOPCUA_BUILD_OPTIONS}
        WORKING_DIRECTORY "${QTOPCUA_BUILD_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${QTOPCUA_MAKE_EXECUTABLE}" install
        WORKING_DIRECTORY "${QTOPCUA_BUILD_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(ENV{INCLUDE} "${QTOPCUA_ORIGINAL_INCLUDE}")

    find_package(Qt5OpcUa CONFIG REQUIRED)
    return()
endif()

set(QTOPCUA_CONFIG_FILE
    "${QTOPCUA_INSTALL_DIR}/lib/cmake/Qt6OpcUa/Qt6OpcUaConfig.cmake")

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
    if(CMAKE_OSX_ARCHITECTURES)
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
    endif()

    # qt-cmake needs cmake and ninja on PATH, which QtCreator does not set.
    get_filename_component(CMAKE_BIN_DIR "${CMAKE_COMMAND}" DIRECTORY)
    find_program(NINJA NAMES ninja REQUIRED)
    get_filename_component(NINJA_DIR "${NINJA}" DIRECTORY)
    if(CMAKE_HOST_WIN32)
        set(CMAKE_PATH "${CMAKE_BIN_DIR};${NINJA_DIR};$ENV{PATH}")
    else()
        set(CMAKE_PATH "${CMAKE_BIN_DIR}:${NINJA_DIR}:$ENV{PATH}")
    endif()

    message(STATUS
        "Configuring Qt OpcUa ${QTOPCUA_VERSION} with the bundled open62541 backend")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${CMAKE_PATH}"
            "${QTOPCUA_CONFIGURE_EXECUTABLE}"
            "${qtopcua_SOURCE_DIR}"
            --
            ${QTOPCUA_CONFIGURE_OPTIONS}
        WORKING_DIRECTORY "${QTOPCUA_BUILD_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "Building and installing Qt OpcUa ${QTOPCUA_VERSION}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${CMAKE_PATH}"
            "${CMAKE_COMMAND}"
            --build "${QTOPCUA_BUILD_DIR}"
            --config RelWithDebInfo
            --target install
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(Qt6OpcUa_DIR "${QTOPCUA_INSTALL_DIR}/lib/cmake/Qt6OpcUa"
    CACHE PATH "Directory containing Qt6OpcUaConfig.cmake" FORCE)
find_package(Qt6OpcUa CONFIG REQUIRED)
