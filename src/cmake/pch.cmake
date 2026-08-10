option(OUAEXP_ENABLE_PCH "Use a shared precompiled header for the ouaexp libraries" ON)

set(OUAEXP_PCH_HEADER "${CMAKE_CURRENT_LIST_DIR}/ouaexp_pch.h")
set(OUAEXP_WIDGETS_PCH_HEADER "${CMAKE_CURRENT_LIST_DIR}/ouaexp_widgets_pch.h")
set(OUAEXP_TEST_PCH_HEADER "${CMAKE_CURRENT_LIST_DIR}/ouaexp_test_pch.h")

function(ouaexp_apply_pch target)
    if(OUAEXP_ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE "${OUAEXP_PCH_HEADER}")
    endif()
endfunction()

# For targets that link Qt Widgets. ouaexp_apply_pch() is the one to use
# elsewhere: its header stays free of widget includes so that a Gui-only target
# such as ouaexp_core is not forced to compile against Qt Widgets.
function(ouaexp_apply_widgets_pch target)
    if(OUAEXP_ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE "${OUAEXP_WIDGETS_PCH_HEADER}")
    endif()
endfunction()

# Builds the shared test precompiled header. Only one target may do this; every
# test executable then reuses the result via ouaexp_reuse_test_pch().
function(ouaexp_create_test_pch target)
    if(OUAEXP_ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE "${OUAEXP_TEST_PCH_HEADER}")
    endif()
endfunction()

# Reuse only works while the compile flags and preprocessor definitions match the
# provider exactly: Clang rejects the PCH outright when a macro differs, so a test
# target that needs its own definitions has to opt out (see the NO_PCH argument of
# ouaexp_register_test).
function(ouaexp_reuse_test_pch target provider)
    if(OUAEXP_ENABLE_PCH)
        target_precompile_headers(${target} REUSE_FROM ${provider})
    endif()
endfunction()
