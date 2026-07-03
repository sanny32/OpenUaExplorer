function(ouaexp_configure_target_macos target_name)
    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )

   if(QTOPCUA_INSTALL_DIR)
        set_property(TARGET ${target_name} APPEND PROPERTY
            INSTALL_RPATH "${QTOPCUA_INSTALL_DIR}/lib")
    endif()
endfunction()
