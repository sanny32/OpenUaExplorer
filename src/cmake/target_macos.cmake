function(ouaexp_configure_target_macos target_name)
    set(MACOSX_COMPOSER_ICON_NAME "ouaexp")
    set(MACOSX_COMPOSER_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/res/${MACOSX_COMPOSER_ICON_NAME}.icon")
    set(MACOSX_BUNDLE_ICON_FILE "${MACOSX_COMPOSER_ICON_NAME}")
    set(MACOSX_ASSET_ICON_PLIST_ENTRY "")

    if(EXISTS "${MACOSX_COMPOSER_ICON}")
        file(GLOB_RECURSE MACOSX_COMPOSER_ICON_FILES
            CONFIGURE_DEPENDS
            "${MACOSX_COMPOSER_ICON}/*"
        )

        find_program(ACTOOL_EXECUTABLE actool REQUIRED)

        set(MACOSX_ASSET_ICON_PLIST_ENTRY
"    <key>CFBundleIconName</key>
    <string>${MACOSX_COMPOSER_ICON_NAME}</string>")

        # The app bundle may live in a subdirectory (this target is under
        # src/app), so resolve its real Resources dir instead of assuming it
        # sits at the top of the build tree.
        set(MACOSX_ICON_OUTPUT_DIR
            "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources")

        add_custom_target(${target_name}_macos_appicon ALL
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${MACOSX_ICON_OUTPUT_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${MACOSX_ICON_OUTPUT_DIR}/${MACOSX_COMPOSER_ICON_NAME}.icon"
            COMMAND "${ACTOOL_EXECUTABLE}"
                "${MACOSX_COMPOSER_ICON}"
                --app-icon "${MACOSX_COMPOSER_ICON_NAME}"
                --compile "${MACOSX_ICON_OUTPUT_DIR}"
                --output-partial-info-plist
                    "${PROJECT_BINARY_DIR}/assetcatalog_generated_info.plist"
                --minimum-deployment-target "11.0"
                --platform macosx
                --target-device mac
                --include-all-app-icons
                --enable-on-demand-resources NO
                --development-region en
            DEPENDS ${MACOSX_COMPOSER_ICON_FILES}
            COMMENT "Synchronizing macOS Icon Composer app icon assets"
        )
        add_dependencies(${target_name}_macos_appicon ${target_name})
    else()
        message(FATAL_ERROR
            "No macOS app icon found. Expected ${MACOSX_COMPOSER_ICON}."
        )
    endif()

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ouaexp.plist.in"
        "${PROJECT_BINARY_DIR}/Info.plist"
        @ONLY
    )

    # Launch Services reads the document-type icon out of the bundle by the name the
    # CFBundleTypeIconFile and UTTypeIconFile entries of Info.plist carry, so the
    # icon has to travel in Resources next to the compiled app icon.
    set(MACOSX_SESSION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/file-ouas.icns")
    target_sources(${target_name} PRIVATE "${MACOSX_SESSION_ICON}")
    set_source_files_properties("${MACOSX_SESSION_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )

    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )

    if(QTOPCUA_INSTALL_DIR)
        set_property(TARGET ${target_name} APPEND PROPERTY
            INSTALL_RPATH "${QTOPCUA_INSTALL_DIR}/lib")
    endif()
endfunction()
