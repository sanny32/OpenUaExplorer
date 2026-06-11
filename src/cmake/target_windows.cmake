function(ouaexp_configure_target_windows target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()

    set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/light/app.ico")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ouaexp.rc.in"
        "${PROJECT_BINARY_DIR}/ouaexp.rc"
        @ONLY
    )

    target_sources(${target_name} PRIVATE "${PROJECT_BINARY_DIR}/ouaexp.rc")
    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE ON)
endfunction()
