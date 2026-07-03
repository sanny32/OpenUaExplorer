if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/Applications"
        CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

if(MACDEPLOYQT_EXECUTABLE)
    install(CODE "
        set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")

        if(NOT EXISTS \"\${_installed_app}\")
            message(FATAL_ERROR \"Installed app bundle not found: \${_installed_app}\")
        endif()

        execute_process(
            COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                    \"\${_installed_app}\"
                    -always-overwrite
                    -codesign=-
            RESULT_VARIABLE _macdeployqt_result
        )

        if(NOT _macdeployqt_result EQUAL 0)
            message(FATAL_ERROR
                \"macdeployqt failed with exit code \${_macdeployqt_result}\")
        endif()

        # macdeployqt deploys every plugin related to the Qt frameworks it copies
        # and offers no per-plugin exclude. A Qt Widgets app does not use the PDF
        # image-format plugin or the virtual-keyboard input context, yet Homebrew
        # ships them, so macdeployqt drags in QtPdf / QtVirtualKeyboard[Qml] (and
        # fails to resolve them). Drop those unused components before signing so
        # they never reach the artifact.
        set(_unwanted_components
            \"\${_installed_app}/Contents/PlugIns/imageformats/libqpdf.dylib\"
            \"\${_installed_app}/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib\"
            \"\${_installed_app}/Contents/Frameworks/QtPdf.framework\"
            \"\${_installed_app}/Contents/Frameworks/QtVirtualKeyboard.framework\"
            \"\${_installed_app}/Contents/Frameworks/QtVirtualKeyboardQml.framework\"
        )
        foreach(_component IN LISTS _unwanted_components)
            if(EXISTS \"\${_component}\")
                message(STATUS \"Removing unused deployed component: \${_component}\")
                file(REMOVE_RECURSE \"\${_component}\")
            endif()
        endforeach()

        find_program(_codesign_exe codesign)
        if(NOT _codesign_exe)
            message(FATAL_ERROR
                \"codesign not found; cannot sign \${_installed_app}\")
        endif()

        execute_process(
            COMMAND \"\${_codesign_exe}\"
                    --force --deep --sign - --timestamp=none
                    \"\${_installed_app}\"
            RESULT_VARIABLE _codesign_result
        )
        if(NOT _codesign_result EQUAL 0)
            message(FATAL_ERROR
                \"codesign failed with exit code \${_codesign_result}\")
        endif()

        execute_process(
            COMMAND \"\${_codesign_exe}\"
                    --verify --deep --strict \"\${_installed_app}\"
            RESULT_VARIABLE _codesign_verify_result
        )
        if(NOT _codesign_verify_result EQUAL 0)
            message(FATAL_ERROR
                \"codesign verification of \${_installed_app} failed with exit \"
                \"code \${_codesign_verify_result}\")
        endif()
    ")
endif()
