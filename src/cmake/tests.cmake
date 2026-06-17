function(ouaexp_configure_test_environment test_name)
    if(WIN32)
        set(test_home "${CMAKE_BINARY_DIR}/.test-home")
        set(test_appdata "${CMAKE_BINARY_DIR}/.test-appdata")
        set(test_environment
            "QT_QPA_PLATFORM=offscreen"
            "USERPROFILE=${test_home}"
            "HOME=${test_home}"
            "APPDATA=${test_home}/AppData/Roaming"
            "LOCALAPPDATA=${test_appdata}")
        set(environment_modifications
            "PATH=path_list_prepend:${QT_BINARY_DIR}"
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::OpcUa>"
            "QT_PLUGIN_PATH=path_list_prepend:${QT_BINARY_DIR}/../plugins")
        if(DEFINED QTOPCUA_INSTALL_DIR)
            list(APPEND environment_modifications
                "QT_PLUGIN_PATH=path_list_prepend:${QTOPCUA_INSTALL_DIR}/plugins")
        endif()
        if(TARGET OpenSSL::Crypto)
            list(APPEND environment_modifications
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:OpenSSL::Crypto>")
        endif()
        if(OPENSSL_ROOT_DIR)
            list(APPEND environment_modifications
                "PATH=path_list_prepend:${OPENSSL_ROOT_DIR}/bin")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${test_environment}"
            ENVIRONMENT_MODIFICATION "${environment_modifications}")
    elseif(APPLE)
        set(environment_modifications
            "QT_PLUGIN_PATH=path_list_prepend:${QT_BINARY_DIR}/../plugins")
        if(DEFINED QTOPCUA_INSTALL_DIR)
            list(APPEND environment_modifications
                "QT_PLUGIN_PATH=path_list_prepend:${QTOPCUA_INSTALL_DIR}/plugins")
        endif()
        if(TARGET OpenSSL::Crypto)
            list(APPEND environment_modifications
                "DYLD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:OpenSSL::Crypto>")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            ENVIRONMENT_MODIFICATION "${environment_modifications}")
    else()
        set(environment_modifications
            "LD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>"
            "LD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::OpcUa>"
            "QT_PLUGIN_PATH=path_list_prepend:${QT_BINARY_DIR}/../plugins")
        if(DEFINED QTOPCUA_INSTALL_DIR)
            list(APPEND environment_modifications
                "QT_PLUGIN_PATH=path_list_prepend:${QTOPCUA_INSTALL_DIR}/plugins")
        endif()
        if(TARGET OpenSSL::Crypto)
            list(APPEND environment_modifications
                "LD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:OpenSSL::Crypto>")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            ENVIRONMENT_MODIFICATION "${environment_modifications}")
    endif()
endfunction()

function(ouaexp_configure_platform_tests)
    if(LINUX)
        include("${OUAEXP_SOURCE_DIR}/cmake/tests_linux_dbus.cmake")
        ouaexp_configure_linux_dbus_tests()
    endif()
endfunction()

function(ouaexp_configure_unit_tests)
    include("${OUAEXP_SOURCE_DIR}/cmake/tests_unit.cmake")
    ouaexp_configure_common_unit_tests()
endfunction()

function(ouaexp_configure_ui_tests)
    include("${OUAEXP_SOURCE_DIR}/cmake/tests_ui.cmake")
    ouaexp_configure_common_ui_tests()
endfunction()

function(ouaexp_configure_integration_tests)
    include("${OUAEXP_SOURCE_DIR}/cmake/tests_integration.cmake")
    ouaexp_configure_opcua_integration_tests()
endfunction()

function(ouaexp_configure_tests)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    function(ouaexp_add_test test_name source_file)
        add_executable(${test_name} "${OUAEXP_TEST_SOURCE_DIR}/${source_file}")
        target_link_libraries(${test_name} PRIVATE
            ouaexp_core
            Qt${QT_VERSION_MAJOR}::Test
        )
        ouaexp_copy_qtopcua_runtime(${test_name})
        ouaexp_apply_coverage(${test_name})
        set_target_properties(${test_name} PROPERTIES FOLDER "Tests")
        add_test(NAME ${test_name} COMMAND ${test_name})
        set_tests_properties(${test_name} PROPERTIES LABELS "ouaexp")
        ouaexp_configure_test_environment(${test_name})
        set_property(GLOBAL APPEND PROPERTY OUAEXP_TEST_TARGETS ${test_name})
    endfunction()

    function(ouaexp_add_ui_test test_name source_file)
        ouaexp_add_test(${test_name} ${source_file})
        target_link_libraries(${test_name} PRIVATE ouaexp_ui)
    endfunction()

    ouaexp_configure_unit_tests()
    ouaexp_configure_ui_tests()
    ouaexp_configure_platform_tests()
    ouaexp_configure_integration_tests()

    get_property(test_targets GLOBAL PROPERTY OUAEXP_TEST_TARGETS)
    add_custom_target(ouaexp_check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L ouaexp
        DEPENDS ${test_targets}
        USES_TERMINAL
    )
    set_target_properties(ouaexp_check PROPERTIES FOLDER "Tests")
endfunction()
