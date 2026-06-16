if(NOT DEFINED OPENSSL_ROOT_DIR)
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT_DIR}"
            CACHE PATH "Root directory of the OpenSSL installation")
    elseif(WIN32)
        set(OPENSSL_ROOT_DIR "C:/Qt/Tools/OpenSSLv3/Win_x64"
            CACHE PATH "Root directory of the OpenSSL installation")
    endif()
endif()

# Qt 5.15.2 builds against OpenSSL 1.1.1; Qt6 against 3.x.
find_package(OpenSSL 1.1 REQUIRED COMPONENTS Crypto SSL)

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
