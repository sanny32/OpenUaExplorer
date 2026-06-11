include("${CMAKE_CURRENT_LIST_DIR}/dependencies/openssl.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/dependencies/qtkeychain.cmake")

function(ouaexp_configure_dependencies target_name)
    ouaexp_configure_openssl(${target_name})
    ouaexp_configure_qtkeychain(${target_name})
endfunction()

function(ouaexp_copy_windows_runtime target_name)
    if(NOT WIN32)
        return()
    endif()

    ouaexp_copy_openssl_runtime(${target_name})
    ouaexp_copy_qtopcua_runtime(${target_name})
endfunction()
