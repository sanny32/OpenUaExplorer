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
            "/opt/homebrew/opt/qt/lib"
            "/opt/homebrew/opt/brotli/lib"
            "/opt/homebrew/opt/webp/lib"
            "/usr/local/lib"
            "/usr/local/opt/qt/lib"
            "/usr/local/opt/brotli/lib"
            "/usr/local/opt/webp/lib")
        if(EXISTS "${_macdeployqt_libpath}")
            list(APPEND _macdeployqt_libpaths "-libpath=${_macdeployqt_libpath}")
        endif()
    endforeach()

    install(CODE "
        set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
        set(_macdeployqt_libpaths ${_macdeployqt_libpaths})
        set(_qtopcua_install_dir \"${QTOPCUA_INSTALL_DIR}\")

        if(NOT EXISTS \"\${_installed_app}\")
            message(FATAL_ERROR \"Installed app bundle not found: \${_installed_app}\")
        endif()

        file(GLOB_RECURSE _qtopcua_frameworks LIST_DIRECTORIES true
            \"\${_qtopcua_install_dir}/QtOpcUa.framework\")
        foreach(_qtopcua_framework IN LISTS _qtopcua_frameworks)
            if(IS_DIRECTORY \"\${_qtopcua_framework}\")
                get_filename_component(_fw_dir \"\${_qtopcua_framework}\" DIRECTORY)
                list(APPEND _macdeployqt_libpaths \"-libpath=\${_fw_dir}\")
            endif()
        endforeach()
        if(_macdeployqt_libpaths)
            list(REMOVE_DUPLICATES _macdeployqt_libpaths)
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
