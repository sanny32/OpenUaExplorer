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

set(MBEDTLS_ROOT_DIR "" CACHE PATH "Root directory of an mbedTLS installation")
set(MBEDTLS_VERSION "2.28.8" CACHE STRING
    "mbedTLS version built from source for Qt5 open62541 encryption")
set(MBEDTLS_FETCH_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/mbedtls-build")
set(MBEDTLS_FETCH_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/mbedtls-install")
set(QTOPCUA_VERSION "${Qt${QT_VERSION_MAJOR}Core_VERSION}")
set(QTOPCUA_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-src")
set(QTOPCUA_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-build")
set(QTOPCUA_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/qtopcua-install")

function(ouaexp_remove_incomplete_qtopcua_source)
    if(NOT EXISTS "${QTOPCUA_SOURCE_DIR}")
        return()
    endif()
    if(EXISTS "${QTOPCUA_SOURCE_DIR}/src/opcua/client/qopcuabrowserequest.h"
       AND EXISTS "${QTOPCUA_SOURCE_DIR}/src/3rdparty/open62541/open62541.h")
        return()
    endif()
    message(STATUS "Removing incomplete Qt OpcUa source tree")
    file(REMOVE_RECURSE "${QTOPCUA_SOURCE_DIR}")
endfunction()

function(ouaexp_locate_mbedtls)
    find_path(MBEDTLS_INCLUDE_DIR
        NAMES mbedtls/x509.h mbedtls/x509_crt.h
        HINTS "${MBEDTLS_ROOT_DIR}/include"
              "${MBEDTLS_FETCH_INSTALL_DIR}/include"
              "$ENV{INCLUDE}"
    )
    find_library(MBEDTLS_X509_LIBRARY
        NAMES mbedx509 libmbedx509
        HINTS "${MBEDTLS_ROOT_DIR}/lib"
              "${MBEDTLS_FETCH_INSTALL_DIR}/lib"
              "$ENV{LIB}"
    )
    find_library(MBEDTLS_CRYPTO_LIBRARY
        NAMES mbedcrypto libmbedcrypto
        HINTS "${MBEDTLS_ROOT_DIR}/lib"
              "${MBEDTLS_FETCH_INSTALL_DIR}/lib"
              "$ENV{LIB}"
    )
endfunction()

# Qt5's qtopcua bundles open62541 v1.0, which targets the mbedTLS 2.x API but ships
# no encryption backend of its own. Rather than require a system-wide mbedTLS, fetch
# and build the mbedTLS 2.28 LTS line from source into the build tree and hand its
# headers/libraries to the qtopcua configure (which auto-enables UA_ENABLE_ENCRYPTION
# once mbedTLS is on the include/lib paths). The build is idempotent: it is skipped
# once the install tree already exists.
function(ouaexp_build_qt5_mbedtls)
    if(EXISTS "${MBEDTLS_FETCH_INSTALL_DIR}/include/mbedtls/x509.h")
        return()
    endif()
    message(STATUS
        "Fetching and building mbedTLS ${MBEDTLS_VERSION} for Qt5 open62541 encryption")
    FetchContent_Declare(mbedtls
        GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
        GIT_TAG "v${MBEDTLS_VERSION}"
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(mbedtls)
    if(NOT mbedtls_POPULATED)
        FetchContent_Populate(mbedtls)
    endif()

    set(_mbedtls_build_type "${CMAKE_BUILD_TYPE}")
    if(NOT _mbedtls_build_type)
        set(_mbedtls_build_type Release)
    endif()
    set(_mbedtls_configure_command "${CMAKE_COMMAND}"
        -S "${mbedtls_SOURCE_DIR}"
        -B "${MBEDTLS_FETCH_BUILD_DIR}"
        "-DCMAKE_INSTALL_PREFIX=${MBEDTLS_FETCH_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${_mbedtls_build_type}"
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DENABLE_TESTING=OFF
        -DENABLE_PROGRAMS=OFF
        -DUSE_STATIC_MBEDTLS_LIBRARY=ON
        -DUSE_SHARED_MBEDTLS_LIBRARY=OFF
    )
    if(CMAKE_GENERATOR)
        list(APPEND _mbedtls_configure_command -G "${CMAKE_GENERATOR}")
    endif()
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND _mbedtls_configure_command -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND _mbedtls_configure_command -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(MSVC)
        list(APPEND _mbedtls_configure_command
            -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL)
    endif()

    execute_process(
        COMMAND ${_mbedtls_configure_command}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${MBEDTLS_FETCH_BUILD_DIR}"
            --config "${_mbedtls_build_type}" --parallel
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${MBEDTLS_FETCH_BUILD_DIR}"
            --config "${_mbedtls_build_type}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endfunction()

function(ouaexp_find_qt5_mbedtls)
    set(QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE OFF CACHE INTERNAL "")
    if(NOT QT_VERSION_MAJOR EQUAL 5)
        return()
    endif()
    ouaexp_locate_mbedtls()
    if(NOT (MBEDTLS_INCLUDE_DIR AND MBEDTLS_X509_LIBRARY AND MBEDTLS_CRYPTO_LIBRARY))
        ouaexp_build_qt5_mbedtls()
        unset(MBEDTLS_INCLUDE_DIR CACHE)
        unset(MBEDTLS_X509_LIBRARY CACHE)
        unset(MBEDTLS_CRYPTO_LIBRARY CACHE)
        ouaexp_locate_mbedtls()
    endif()
    if(NOT (MBEDTLS_INCLUDE_DIR AND MBEDTLS_X509_LIBRARY AND MBEDTLS_CRYPTO_LIBRARY))
        message(FATAL_ERROR
            "Failed to provide mbedTLS for the Qt5 bundled open62541 backend. "
            "The automatic source build did not produce include/mbedtls/x509.h, "
            "lib/mbedx509 and lib/mbedcrypto under ${MBEDTLS_FETCH_INSTALL_DIR}. "
            "Set MBEDTLS_ROOT_DIR to a prebuilt mbedTLS installation to override.")
    endif()
    set(QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE ON CACHE INTERNAL "")
    set(QTOPCUA_MBEDTLS_INCLUDE_DIR "${MBEDTLS_INCLUDE_DIR}" CACHE INTERNAL "")
    set(QTOPCUA_MBEDTLS_LIBRARIES
        "${MBEDTLS_X509_LIBRARY};${MBEDTLS_CRYPTO_LIBRARY}" CACHE INTERNAL "")
    message(STATUS "mbedTLS is available; Qt5 bundled open62541 encryption is enabled.")
endfunction()

function(ouaexp_validate_qt5_open62541_history source_dir)
    if(NOT QT_VERSION_MAJOR EQUAL 5)
        return()
    endif()
    set(QTOPCUA_OPEN62541_HISTORY_READY OFF CACHE INTERNAL "")
    set(open62541_header "${source_dir}/src/3rdparty/open62541/open62541.h")
    set(open62541_source "${source_dir}/src/3rdparty/open62541/open62541.c")
    if(NOT EXISTS "${open62541_header}" OR NOT EXISTS "${open62541_source}")
        return()
    endif()
    file(READ "${open62541_header}" open62541_header_contents)
    if(NOT open62541_header_contents MATCHES "UA_TYPES_HISTORYDATA"
       OR NOT open62541_header_contents MATCHES "UA_HistoryReadResponse"
       OR NOT open62541_header_contents MATCHES "UA_HistoryData")
        message(STATUS
            "Qt5 bundled open62541 has no generated historizing types; "
            "Qt5 direct HistoryRead is disabled.")
        return()
    endif()
    set(QTOPCUA_OPEN62541_HISTORY_READY ON CACHE INTERNAL "")
    set(OUAEXP_QTOPCUA_SOURCE_DIR "${source_dir}" CACHE INTERNAL "")
endfunction()

set(QTOPCUA_SKIP_SYSTEM_PACKAGE OFF)
if(QT_VERSION_MAJOR EQUAL 5)
    set(QTOPCUA_SKIP_SYSTEM_PACKAGE ON)
    get_filename_component(QTOPCUA_QT5_PREFIX "${QT_BINARY_DIR}/.." ABSOLUTE)
    set(QTOPCUA_CONFIG_FILE
        "${QTOPCUA_QT5_PREFIX}/lib/cmake/Qt5OpcUa/Qt5OpcUaConfig.cmake")
    set(QTOPCUA_OPEN62541_PLUGIN
        "${QTOPCUA_QT5_PREFIX}/plugins/opcua/open62541_backend.dll")
    if(EXISTS "${QTOPCUA_CONFIG_FILE}"
       AND (NOT WIN32 OR EXISTS "${QTOPCUA_OPEN62541_PLUGIN}"))
        set(Qt5OpcUa_DIR "${QTOPCUA_QT5_PREFIX}/lib/cmake/Qt5OpcUa"
            CACHE PATH "Directory containing Qt5OpcUaConfig.cmake" FORCE)
        find_package(Qt5OpcUa CONFIG REQUIRED)
    endif()
else()
    find_package(Qt${QT_VERSION_MAJOR}OpcUa CONFIG QUIET)
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa)
    if(QT_VERSION_MAJOR EQUAL 5)
        if(QTOPCUA_SKIP_SYSTEM_PACKAGE)
            set(existing_qtopcua_source "${CMAKE_BINARY_DIR}/_deps/qtopcua-src")
            if(EXISTS "${existing_qtopcua_source}/src/3rdparty/open62541/open62541.h")
                ouaexp_validate_qt5_open62541_history("${existing_qtopcua_source}")
            endif()
        endif()
    endif()
    ouaexp_find_qt5_mbedtls()
    return()
endif()

FetchContent_Declare(qtopcua
    GIT_REPOSITORY https://code.qt.io/qt/qtopcua.git
    GIT_TAG "v${QTOPCUA_VERSION}"
    GIT_SHALLOW TRUE
)

ouaexp_remove_incomplete_qtopcua_source()
FetchContent_GetProperties(qtopcua)
if(NOT qtopcua_POPULATED)
    FetchContent_Populate(qtopcua)
endif()

# Qt's bundled open62541 backend hard-codes dataEncoding="Default Binary" on every
# HistoryRead request (qopen62541backend.cpp). Strict UA servers (e.g. Unified
# Automation) reject that encoding for scalar variables and return an empty result,
# so HistoryRead silently yields no data; UaExpert and the OPC UA spec send a null
# dataEncoding instead. Qt exposes no API to override it, so we neutralise the line
# at configure time. The replacement keys on the code text rather than a line number
# so it survives across Qt OpcUa point releases; it is idempotent and warns loudly if
# the upstream source changes, so the workaround is never silently a no-op.
function(ouaexp_patch_qtopcua_history source_dir)
    if(QT_VERSION_MAJOR LESS 6)
        return()
    endif()
    set(backend "${source_dir}/src/plugins/opcua/open62541/qopen62541backend.cpp")
    if(NOT EXISTS "${backend}")
        return()
    endif()
    file(READ "${backend}" contents)
    if(contents MATCHES "ouaexp patch: dataEncoding left null")
        return() # already patched
    endif()
    set(needle "uarequest.nodesToRead[i].dataEncoding = UA_QUALIFIEDNAME_ALLOC(0, \"Default Binary\");")
    string(FIND "${contents}" "${needle}" found)
    if(found EQUAL -1)
        message(WARNING
            "Qt OpcUa HistoryRead dataEncoding workaround did not match qtopcua "
            "${QTOPCUA_VERSION}; the upstream source changed. Re-check whether the "
            "HistoryRead dataEncoding fix is still required.")
        return()
    endif()
    string(REPLACE "${needle}"
        "// ouaexp patch: dataEncoding left null (Default Binary breaks history on strict UA servers)"
        contents "${contents}")
    file(WRITE "${backend}" "${contents}")
    message(STATUS "Applied Qt OpcUa HistoryRead dataEncoding workaround")
endfunction()

function(ouaexp_patch_qtopcua_open62541_features source_dir)
    set(project_file "${source_dir}/src/plugins/opcua/open62541/open62541.pro")
    if(NOT EXISTS "${project_file}")
        return()
    endif()
    file(READ "${project_file}" contents)
    if(QTOPCUA_OPEN62541_HISTORY_READY
       AND NOT contents MATCHES "UA_ENABLE_HISTORIZING")
        string(REPLACE
            "qtConfig(open62541):!qtConfig(system-open62541) {\n"
            "qtConfig(open62541):!qtConfig(system-open62541) {\n    DEFINES += UA_ENABLE_HISTORIZING\n"
            contents "${contents}")
    endif()
    file(WRITE "${project_file}" "${contents}")
endfunction()

function(ouaexp_remove_stale_qtopcua_build)
    if(NOT (QT_VERSION_MAJOR EQUAL 5 AND EXISTS "${QTOPCUA_BUILD_DIR}"))
        return()
    endif()
    file(GLOB_RECURSE makefiles
        "${QTOPCUA_BUILD_DIR}/src/plugins/opcua/open62541/Makefile*")
    foreach(makefile IN LISTS makefiles)
        file(READ "${makefile}" contents)
        if(NOT QTOPCUA_OPEN62541_HISTORY_READY
           AND contents MATCHES "UA_ENABLE_HISTORIZING")
            message(STATUS
                "Removing stale Qt OpcUa build directory with incompatible "
                "UA_ENABLE_HISTORIZING flags")
            file(REMOVE_RECURSE "${QTOPCUA_BUILD_DIR}")
            return()
        endif()
        if(QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE
           AND NOT contents MATCHES "UA_ENABLE_ENCRYPTION")
            message(STATUS
                "Removing stale Qt OpcUa build directory without "
                "UA_ENABLE_ENCRYPTION flags")
            file(REMOVE_RECURSE "${QTOPCUA_BUILD_DIR}")
            return()
        endif()
        if(NOT QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE
           AND contents MATCHES "UA_ENABLE_ENCRYPTION")
            message(STATUS
                "Removing stale Qt OpcUa build directory with obsolete "
                "UA_ENABLE_ENCRYPTION flags")
            file(REMOVE_RECURSE "${QTOPCUA_BUILD_DIR}")
            return()
        endif()
    endforeach()
endfunction()

ouaexp_validate_qt5_open62541_history("${qtopcua_SOURCE_DIR}")
ouaexp_find_qt5_mbedtls()
ouaexp_patch_qtopcua_history("${qtopcua_SOURCE_DIR}")
ouaexp_patch_qtopcua_open62541_features("${qtopcua_SOURCE_DIR}")
ouaexp_remove_stale_qtopcua_build()

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
    set(QTOPCUA_ORIGINAL_LIB "$ENV{LIB}")
    if(CMAKE_HOST_WIN32 AND OPENSSL_INCLUDE_DIR)
        file(TO_CMAKE_PATH "${OPENSSL_INCLUDE_DIR}" _ssl_include)
        set(ENV{INCLUDE} "${_ssl_include};$ENV{INCLUDE}")
    endif()
    if(CMAKE_HOST_WIN32 AND QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE)
        file(TO_CMAKE_PATH "${QTOPCUA_MBEDTLS_INCLUDE_DIR}" _mbedtls_include)
        set(ENV{INCLUDE} "${_mbedtls_include};$ENV{INCLUDE}")
        set(QTOPCUA_MBEDTLS_LIBRARY_DIRS)
        foreach(_mbedtls_library IN LISTS QTOPCUA_MBEDTLS_LIBRARIES)
            get_filename_component(_mbedtls_library_dir "${_mbedtls_library}" DIRECTORY)
            file(TO_CMAKE_PATH "${_mbedtls_library_dir}" _mbedtls_library_dir)
            set(ENV{LIB} "${_mbedtls_library_dir};$ENV{LIB}")
            list(APPEND QTOPCUA_MBEDTLS_LIBRARY_DIRS "${_mbedtls_library_dir}")
        endforeach()
        list(REMOVE_DUPLICATES QTOPCUA_MBEDTLS_LIBRARY_DIRS)
    endif()
    set(QTOPCUA_QMAKE_ARGUMENTS)
    if(QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE)
        list(APPEND QTOPCUA_QMAKE_ARGUMENTS
            "INCLUDEPATH+=${QTOPCUA_MBEDTLS_INCLUDE_DIR}"
            "QMAKE_LIBDIR+=${QTOPCUA_MBEDTLS_LIBRARY_DIRS}"
        )
        if(CMAKE_HOST_WIN32)
            # mbedcrypto's entropy_poll.c needs the Windows CryptoAPI
            # (CryptAcquireContext/CryptGenRandom from advapi32); the static mbedTLS
            # libraries do not carry that system dependency into qmake's link. qmake
            # propagates these command-line LIBS down to the open62541 plugin.
            list(APPEND QTOPCUA_QMAKE_ARGUMENTS
                "LIBS+=-ladvapi32"
                "LIBS+=-lbcrypt"
            )
        endif()
    endif()

    message(STATUS
        "Configuring Qt OpcUa ${QTOPCUA_VERSION} with the bundled open62541 backend")
    execute_process(
        COMMAND "${QTOPCUA_QMAKE_EXECUTABLE}"
            "${qtopcua_SOURCE_DIR}/qtopcua.pro"
            ${QTOPCUA_QMAKE_ARGUMENTS}
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
    set(ENV{LIB} "${QTOPCUA_ORIGINAL_LIB}")

    set(Qt5OpcUa_DIR "${QTOPCUA_QT5_PREFIX}/lib/cmake/Qt5OpcUa"
        CACHE PATH "Directory containing Qt5OpcUaConfig.cmake" FORCE)
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
    if(OPENSSL_ROOT_DIR)
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
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
