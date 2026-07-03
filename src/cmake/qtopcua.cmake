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

set(QTOPCUA_VERSION "${Qt6Core_VERSION}")
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

function(ouaexp_prepare_qtopcua_config_opt build_dir)
    file(REMOVE "${build_dir}/config.opt.in")
    file(TOUCH  "${build_dir}/config.opt.in")
endfunction()

function(ouaexp_find_qtopcua_config out_var)
    set(search_dirs
        "${QTOPCUA_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/cmake/Qt6OpcUa"
        "${QTOPCUA_INSTALL_DIR}/lib/cmake/Qt6OpcUa"
    )
    if(CMAKE_LIBRARY_ARCHITECTURE)
        list(APPEND search_dirs
            "${QTOPCUA_INSTALL_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/Qt6OpcUa")
    endif()

    find_file(config_file
        NAMES Qt6OpcUaConfig.cmake
        PATHS ${search_dirs}
        NO_DEFAULT_PATH
        NO_CACHE
    )
    if(NOT config_file)
        file(GLOB_RECURSE config_matches CONFIGURE_DEPENDS
            "${QTOPCUA_INSTALL_DIR}/Qt6OpcUaConfig.cmake"
            "${QTOPCUA_INSTALL_DIR}/*/Qt6OpcUaConfig.cmake"
            "${QTOPCUA_INSTALL_DIR}/*/*/Qt6OpcUaConfig.cmake"
            "${QTOPCUA_INSTALL_DIR}/*/*/*/Qt6OpcUaConfig.cmake"
            "${QTOPCUA_INSTALL_DIR}/*/*/*/*/Qt6OpcUaConfig.cmake"
        )
        if(config_matches)
            list(GET config_matches 0 config_file)
        endif()
    endif()

    set(${out_var} "${config_file}" PARENT_SCOPE)
endfunction()

find_package(Qt6OpcUa CONFIG QUIET)

if(TARGET Qt6::OpcUa)
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

ouaexp_patch_qtopcua_history("${qtopcua_SOURCE_DIR}")

ouaexp_find_qtopcua_config(QTOPCUA_CONFIG_FILE)

if(NOT EXISTS "${QTOPCUA_CONFIG_FILE}")
    file(MAKE_DIRECTORY "${QTOPCUA_BUILD_DIR}")
    ouaexp_prepare_qtopcua_config_opt("${QTOPCUA_BUILD_DIR}")

    set(QTOPCUA_CONFIGURE_OPTIONS
        -DINPUT_open62541=qt
        -DQT_BUILD_TESTS=OFF
        -DQT_BUILD_EXAMPLES=OFF
        "-DCMAKE_TOOLCHAIN_FILE=${Qt6_DIR}/qt.toolchain.cmake"
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
            "${CMAKE_COMMAND}"
            -G Ninja
            -S "${qtopcua_SOURCE_DIR}"
            -B "${QTOPCUA_BUILD_DIR}"
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

ouaexp_find_qtopcua_config(QTOPCUA_CONFIG_FILE)
get_filename_component(QTOPCUA_CONFIG_DIR "${QTOPCUA_CONFIG_FILE}" DIRECTORY)
set(Qt6OpcUa_DIR "${QTOPCUA_CONFIG_DIR}"
    CACHE PATH "Directory containing Qt6OpcUaConfig.cmake" FORCE)
find_package(Qt6OpcUa CONFIG REQUIRED)
