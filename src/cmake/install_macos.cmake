if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/Applications"
        CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

if(MACDEPLOYQT_EXECUTABLE)
    get_filename_component(_qt_lib_dir "${QT_BINARY_DIR}/../lib" REALPATH)
    set(_macdeployqt_libpaths)
    foreach(_macdeployqt_libpath IN ITEMS
            "${_qt_lib_dir}"
            "${QTOPCUA_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}"
            "${QTOPCUA_INSTALL_DIR}/lib"
            "/opt/homebrew/lib"
            "/opt/homebrew/opt/brotli/lib"
            "/opt/homebrew/opt/webp/lib"
            "/usr/local/lib"
            "/usr/local/opt/brotli/lib"
            "/usr/local/opt/webp/lib")
        if(EXISTS "${_macdeployqt_libpath}")
            list(APPEND _macdeployqt_libpaths "-libpath=${_macdeployqt_libpath}")
        endif()
    endforeach()

    install(CODE "
        set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
        set(_macdeployqt_libpaths ${_macdeployqt_libpaths})

        if(NOT EXISTS \"\${_installed_app}\")
            message(FATAL_ERROR \"Installed app bundle not found: \${_installed_app}\")
        endif()

        execute_process(
            COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                    \"\${_installed_app}\"
                    -always-overwrite
                    -codesign=-
                    \${_macdeployqt_libpaths}
            RESULT_VARIABLE _macdeployqt_result
        )

        if(NOT _macdeployqt_result EQUAL 0)
            message(FATAL_ERROR
                \"macdeployqt failed with exit code \${_macdeployqt_result}\")
        endif()
    ")
endif()
