if(NOT DEFINED OPENSSL_ROOT_DIR)
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT_DIR}"
            CACHE PATH "Root directory of the OpenSSL installation")
    elseif(WIN32)
        set(OPENSSL_ROOT_DIR "C:/Qt/Tools/OpenSSLv3/Win_x64"
            CACHE PATH "Root directory of the OpenSSL installation")
    endif()
endif()

find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto SSL)

# Derive the root from the found libraries so sub-builds (Qt OpcUa) locate the
# same OpenSSL; on macOS Homebrew lives outside CMake's default search prefixes.
if(APPLE AND NOT DEFINED OPENSSL_ROOT_DIR)
    get_filename_component(_openssl_lib_dir "${OPENSSL_CRYPTO_LIBRARY}" DIRECTORY)
    get_filename_component(_openssl_root "${_openssl_lib_dir}" DIRECTORY)
    set(OPENSSL_ROOT_DIR "${_openssl_root}"
        CACHE PATH "Root directory of the OpenSSL installation")
endif()

if(WIN32 AND TARGET OpenSSL::Crypto)
    file(GLOB OPENSSL_RUNTIME_FILES
        "${OPENSSL_ROOT_DIR}/bin/libcrypto-*-x64.dll"
        "${OPENSSL_ROOT_DIR}/bin/libssl-*-x64.dll")
endif()

function(ouaexp_configure_openssl target_name)
    target_link_libraries(${target_name} PRIVATE OpenSSL::Crypto)
endfunction()

function(ouaexp_copy_openssl_runtime target_name)
    foreach(OPENSSL_RUNTIME_FILE IN LISTS OPENSSL_RUNTIME_FILES)
        if(EXISTS "${OPENSSL_RUNTIME_FILE}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OPENSSL_RUNTIME_FILE}"
                    "$<TARGET_FILE_DIR:${target_name}>"
                VERBATIM
            )
        endif()
    endforeach()
endfunction()
