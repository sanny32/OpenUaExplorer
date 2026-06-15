if(NOT DEFINED OPENSSL_ROOT_DIR)
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT_DIR}"
            CACHE PATH "Root directory of the OpenSSL 3 installation")
    elseif(WIN32)
        set(OPENSSL_ROOT_DIR "C:/Qt/Tools/OpenSSLv3/Win_x64"
            CACHE PATH "Root directory of the OpenSSL 3 installation")
    endif()
endif()

find_package(OpenSSL 3 QUIET COMPONENTS Crypto SSL)

if(WIN32 AND TARGET OpenSSL::Crypto)
    set(OUAEXP_OPENSSL_RUNTIME_FILES
        "${OPENSSL_ROOT_DIR}/bin/libcrypto-3-x64.dll"
        "${OPENSSL_ROOT_DIR}/bin/libssl-3-x64.dll"
    )
endif()

function(ouaexp_configure_openssl target_name)
    if(TARGET OpenSSL::Crypto)
        target_link_libraries(${target_name} PRIVATE OpenSSL::Crypto)
        target_compile_definitions(${target_name} PRIVATE OUAEXP_HAS_OPENSSL)
    else()
        message(WARNING
            "OpenSSL Crypto was not found. Client certificate generation will be unavailable.")
    endif()
endfunction()

function(ouaexp_copy_openssl_runtime target_name)
    foreach(OPENSSL_RUNTIME_FILE IN LISTS OUAEXP_OPENSSL_RUNTIME_FILES)
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
