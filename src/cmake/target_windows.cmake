function(ouaexp_configure_target_windows target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()

    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE ON)
endfunction()
