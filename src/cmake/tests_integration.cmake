function(ouaexp_configure_opcua_integration_tests)
    ouaexp_add_test(ouaexp_tests_integration test_opcua_integration.cpp)
    target_compile_definitions(ouaexp_tests_integration PRIVATE
        OUAEXP_TEST_SERVER_SCRIPT="${PROJECT_SOURCE_DIR}/../tools/opcua_test_server.py")

    ouaexp_add_test(ouaexp_tests_auth_integration test_opcua_auth_integration.cpp)
    target_compile_definitions(ouaexp_tests_auth_integration PRIVATE
        OUAEXP_TEST_SERVER_SCRIPT="${PROJECT_SOURCE_DIR}/../tools/opcua_test_server.py")

    ouaexp_add_ui_test(ouaexp_tests_mainwindow_session_integration
        test_mainwindow_session_integration.cpp)
    target_compile_definitions(ouaexp_tests_mainwindow_session_integration PRIVATE
        OUAEXP_TEST_SERVER_SCRIPT="${PROJECT_SOURCE_DIR}/../tools/opcua_test_server.py")
endfunction()
