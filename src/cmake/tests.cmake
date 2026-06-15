function(ouaexp_configure_test_environment test_name)
    if(WIN32)
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
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
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

function(ouaexp_configure_tests)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    function(ouaexp_add_test test_name source_file)
        add_executable(${test_name} tests/${source_file})
        target_link_libraries(${test_name} PRIVATE
            ouaexp_core
            Qt${QT_VERSION_MAJOR}::Test
        )
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

    ouaexp_add_test(ouaexp_tests           test_opcua.cpp)
    ouaexp_add_test(ouaexp_tests_profiles  test_profiles.cpp)
    ouaexp_add_test(ouaexp_tests_models    test_models.cpp)
    ouaexp_add_test(ouaexp_tests_secrets   test_secretstore.cpp)
    ouaexp_add_test(ouaexp_tests_formatter test_attributeformatter.cpp)
    ouaexp_add_test(ouaexp_tests_controller test_connectioncontroller.cpp)
    ouaexp_add_test(ouaexp_tests_connection_data test_connectiondata.cpp)
    ouaexp_add_ui_test(ouaexp_tests_connection_dialog test_connectiondialog.cpp)
    ouaexp_add_test(ouaexp_tests_integration test_opcua_integration.cpp)
    target_compile_definitions(ouaexp_tests_integration PRIVATE
        OUAEXP_TEST_SERVER_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/../tools/opcua_test_server.py")

    get_property(test_targets GLOBAL PROPERTY OUAEXP_TEST_TARGETS)
    add_custom_target(ouaexp_check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L ouaexp
        DEPENDS ${test_targets}
        USES_TERMINAL
    )
    set_target_properties(ouaexp_check PROPERTIES FOLDER "Tests")
endfunction()
