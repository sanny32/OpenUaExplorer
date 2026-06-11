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
            RESULT_VARIABLE _macdeployqt_result
        )

        if(NOT _macdeployqt_result EQUAL 0)
            message(FATAL_ERROR
                \"macdeployqt failed with exit code \${_macdeployqt_result}\")
        endif()
    ")
endif()
