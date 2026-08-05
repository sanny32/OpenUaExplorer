function(ouaexp_configure_opcua_integration_tests)
    ouaexp_add_test(ouaexp_tests_integration test_opcua_integration.cpp)
    ouaexp_add_test(ouaexp_tests_auth_integration test_opcua_auth_integration.cpp)
    ouaexp_add_ui_test(ouaexp_tests_mainwindow_session_integration
        test_mainwindow_session_integration.cpp)

    # These start the OPC UA test server on fixed ports that overlap between the
    # suites, so ctest must never run two of them at the same time.
    set_property(TEST
        ouaexp_tests_integration
        ouaexp_tests_auth_integration
        ouaexp_tests_mainwindow_session_integration
        APPEND PROPERTY RESOURCE_LOCK opcua_test_server)
endfunction()
