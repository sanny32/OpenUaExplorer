include(FetchContent)
find_package(Git REQUIRED)

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
set(OPCUA_OPEN62541_VERSION "")

function(ouaexp_detect_open62541_version source_dir)
    set(open62541_header "${source_dir}/src/3rdparty/open62541/open62541.h")
    if(NOT EXISTS "${open62541_header}")
        set(OPCUA_OPEN62541_VERSION "" PARENT_SCOPE)
        return()
    endif()

    file(STRINGS "${open62541_header}" version_line
        REGEX "^#define[ \t]+UA_OPEN62541_VERSION[ \t]+\"[^\"]*\"")
    if(version_line MATCHES "^#define[ \t]+UA_OPEN62541_VERSION[ \t]+\"([^\"]*)\"")
        set(OPCUA_OPEN62541_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
        set(OPCUA_OPEN62541_VERSION "" PARENT_SCOPE)
    endif()
endfunction()

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
        file(GLOB_RECURSE config_matches
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

function(ouaexp_normalize_patch patch out_var)
    get_filename_component(name "${patch}" NAME)
    set(normalized "${CMAKE_BINARY_DIR}/qtopcua-patches/${name}")
    configure_file("${patch}" "${normalized}" @ONLY NEWLINE_STYLE UNIX)
    set(${out_var} "${normalized}" PARENT_SCOPE)
endfunction()

# Applies the cmake/patches fixes to the fetched Qt OpcUa backend. Each patch carries a header
# explaining why it exists; one that no longer applies is a hard error rather than a silently
# dropped workaround.
function(ouaexp_patch_qtopcua source_dir)
    if(NOT EXISTS "${source_dir}/.git")
        return()
    endif()
    file(GLOB patches "${CMAKE_CURRENT_LIST_DIR}/patches/*.patch")
    list(SORT patches)
    foreach(patch IN LISTS patches)
        get_filename_component(name "${patch}" NAME)
        ouaexp_normalize_patch("${patch}" patch)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} apply --reverse --check -C1 "${patch}"
            WORKING_DIRECTORY "${source_dir}"
            RESULT_VARIABLE already_applied
            OUTPUT_QUIET ERROR_QUIET
        )
        if(already_applied EQUAL 0)
            continue()
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} apply -C1 "${patch}"
            WORKING_DIRECTORY "${source_dir}"
            RESULT_VARIABLE applied
            ERROR_VARIABLE apply_error
        )
        if(NOT applied EQUAL 0)
            message(FATAL_ERROR
                "The Qt OpcUa patch ${name} does not apply to qtopcua ${QTOPCUA_VERSION}: "
                "${apply_error}Rebase the patch onto the new sources, or drop it if the "
                "upstream defect it works around is fixed.")
        endif()
        message(STATUS "Applied Qt OpcUa patch ${name}")
        set(OUAEXP_QTOPCUA_PATCHED TRUE PARENT_SCOPE)
    endforeach()
endfunction()

ouaexp_patch_qtopcua("${QTOPCUA_SOURCE_DIR}")
if(OUAEXP_QTOPCUA_PATCHED AND EXISTS "${QTOPCUA_INSTALL_DIR}")
    message(STATUS "Qt OpcUa sources were patched; rebuilding the bundled backend")
    file(REMOVE_RECURSE "${QTOPCUA_BUILD_DIR}" "${QTOPCUA_INSTALL_DIR}")
    unset(Qt6OpcUa_DIR CACHE)
endif()

find_package(Qt6OpcUa CONFIG QUIET)

if(TARGET Qt6::OpcUa)
    ouaexp_detect_open62541_version("${QTOPCUA_SOURCE_DIR}")
    return()
endif()

FetchContent_Declare(qtopcua
    GIT_REPOSITORY https://code.qt.io/qt/qtopcua.git
    GIT_TAG "v${QTOPCUA_VERSION}"
    GIT_SHALLOW TRUE
    SOURCE_DIR "${QTOPCUA_SOURCE_DIR}"
    BINARY_DIR "${QTOPCUA_BUILD_DIR}"
    SOURCE_SUBDIR cmake-populate-only
)

ouaexp_remove_incomplete_qtopcua_source()
FetchContent_MakeAvailable(qtopcua)
FetchContent_GetProperties(qtopcua)

ouaexp_detect_open62541_version("${qtopcua_SOURCE_DIR}")

ouaexp_patch_qtopcua("${qtopcua_SOURCE_DIR}")

ouaexp_find_qtopcua_config(QTOPCUA_CONFIG_FILE)

if(NOT EXISTS "${QTOPCUA_CONFIG_FILE}")
    file(MAKE_DIRECTORY "${QTOPCUA_BUILD_DIR}")
    ouaexp_prepare_qtopcua_config_opt("${QTOPCUA_BUILD_DIR}")
  
    set(QTOPCUA_CONFIGURE_OPTIONS
        -DINPUT_open62541=qt
        -DFEATURE_open62541_security=ON
        -DQT_BUILD_TESTS=OFF
        -DQT_BUILD_EXAMPLES=OFF
        "-DCMAKE_TOOLCHAIN_FILE=${Qt6_DIR}/qt.toolchain.cmake"
        "-DCMAKE_INSTALL_PREFIX=${QTOPCUA_INSTALL_DIR}"
    )

    if(NOT "$ENV{CC}" STREQUAL "")
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DCMAKE_C_COMPILER=$ENV{CC}")
    endif()
    if(NOT "$ENV{CXX}" STREQUAL "")
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DCMAKE_CXX_COMPILER=$ENV{CXX}")
    endif()

    if(OPENSSL_ROOT_DIR)
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            list(APPEND QTOPCUA_CONFIGURE_OPTIONS
                "-DOPENSSL_INCLUDE_DIR=${OPENSSL_ROOT_DIR}/include"
                "-DOPENSSL_SSL_LIBRARY=${OPENSSL_ROOT_DIR}/lib/libssl.so"
                "-DOPENSSL_CRYPTO_LIBRARY=${OPENSSL_ROOT_DIR}/lib/libcrypto.so")
        endif()
    endif()
    if(APPLE)
        # Qt's toolchain file defaults to a universal build, but Homebrew
        # OpenSSL is single-arch; build the backend for the same architectures
        # as the application.
        if(CMAKE_OSX_ARCHITECTURES)
            set(QTOPCUA_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
        else()
            set(QTOPCUA_OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DCMAKE_OSX_ARCHITECTURES=${QTOPCUA_OSX_ARCHITECTURES}")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT "$ENV{CXX}" STREQUAL "")
        list(APPEND QTOPCUA_CONFIGURE_OPTIONS
            "-DCMAKE_SHARED_LINKER_FLAGS=-static-libgcc -static-libstdc++"
            "-DCMAKE_MODULE_LINKER_FLAGS=-static-libgcc -static-libstdc++"
            "-DCMAKE_EXE_LINKER_FLAGS=-static-libgcc -static-libstdc++")
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
