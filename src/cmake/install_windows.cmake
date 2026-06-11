if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/${PRODUCT_NAME}"
        CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

if(TARGET OpenSSL::Crypto)
    install(FILES ${OUAEXP_OPENSSL_RUNTIME_FILES} DESTINATION .)
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::OpcUa)
    install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::OpcUa>" DESTINATION .)
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::QOpen62541Plugin)
    install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QOpen62541Plugin>"
        DESTINATION plugins/opcua)
endif()

if(WINDEPLOYQT_EXECUTABLE)
    install(CODE "
        set(_installed_exe \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe\")
        get_filename_component(_installed_exe_dir \"\${_installed_exe}\" DIRECTORY)
        set(_plugin_dir \"\${_installed_exe_dir}/plugins\")

        if(NOT EXISTS \"\${_installed_exe}\")
            message(FATAL_ERROR \"Installed executable not found: \${_installed_exe}\")
        endif()

        if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^[Dd]ebug$\")
            set(_deploy_mode --debug)
        else()
            set(_deploy_mode --release)
        endif()

        set(_windeploy_args
            \${_deploy_mode}
            --plugindir \"\${_plugin_dir}\"
            --no-opengl-sw
            --no-system-d3d-compiler
        )

        if(\"${QT_VERSION_MAJOR}\" STREQUAL \"6\")
            list(APPEND _windeploy_args
                --no-system-dxc-compiler
                --skip-plugin-types generic,networkinformation,qmltooling,tls
                --exclude-plugins qgif,qjpeg,qpdf,qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql)
        elseif(\"${QT_VERSION_MAJOR}\" STREQUAL \"5\")
            list(APPEND _windeploy_args
                --no-angle
                --no-quick)
        endif()

        list(APPEND _windeploy_args \"\${_installed_exe}\")

        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \${_windeploy_args}
            RESULT_VARIABLE _windeployqt_result
        )

        if(NOT _windeployqt_result EQUAL 0)
            message(FATAL_ERROR
                \"windeployqt failed with exit code \${_windeployqt_result}\")
        endif()
    ")
endif()
