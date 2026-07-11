option(OUAEXP_ENABLE_PCH "Use a shared precompiled header for the ouaexp libraries" ON)

set(OUAEXP_PCH_HEADER "${CMAKE_CURRENT_LIST_DIR}/ouaexp_pch.h")

function(ouaexp_apply_pch target)
    if(OUAEXP_ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE "${OUAEXP_PCH_HEADER}")
    endif()
endfunction()
