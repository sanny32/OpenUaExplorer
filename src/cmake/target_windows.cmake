function(ouaexp_configure_target_windows target_name rc_template icon_file session_icon_file)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()

    set(ICON_FILE "${icon_file}")
    set(SESSION_ICON_FILE "${session_icon_file}")
    configure_file(
        "${rc_template}"
        "${PROJECT_BINARY_DIR}/ouaexp.rc"
        @ONLY
    )

    # The generator only sees the .rc, not the icons it names, so without this an edited
    # icon leaves the resource - and therefore the executable - untouched.
    set_source_files_properties("${PROJECT_BINARY_DIR}/ouaexp.rc" PROPERTIES
        OBJECT_DEPENDS "${icon_file};${session_icon_file}"
    )

    target_sources(${target_name} PRIVATE "${PROJECT_BINARY_DIR}/ouaexp.rc")
    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE ON)
endfunction()
