function(ouaexp_configure_target_windows target_name rc_template icon_file)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()

    set(ICON_FILE "${icon_file}")
    configure_file(
        "${rc_template}"
        "${PROJECT_BINARY_DIR}/ouaexp.rc"
        @ONLY
    )

    target_sources(${target_name} PRIVATE "${PROJECT_BINARY_DIR}/ouaexp.rc")
    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE ON)
endfunction()
