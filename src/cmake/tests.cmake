function(ouaexp_configure_tests)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    add_library(ouaexp_testable STATIC
        opcua/attributeformatter.cpp
        opcua/connectionprofilestore.cpp
        opcua/opcuaclientservice.cpp
        opcua/pkimanager.cpp
        opcua/secretstore.cpp
        loggingcategories.cpp
        widgets/addressspacemodel.cpp
        widgets/attributesmodel.cpp
        widgets/dataaccessmodel.cpp
        widgets/eventsmodel.cpp
        widgets/historymodel.cpp
        widgets/logmodel.cpp
        widgets/nodeinfomodel.cpp
        widgets/referencesmodel.cpp
        widgets/subscriptionsmodel.cpp
        opcua/attributeformatter.h
        opcua/connectionprofile.h
        opcua/connectionprofilestore.h
        opcua/opcuaclientservice.h
        opcua/opcuatypes.h
        opcua/pkimanager.h
        opcua/secretstore.h
        widgets/addressspaceitem.h
        widgets/addressspacemodel.h
        widgets/attributesmodel.h
        widgets/dataaccessitem.h
        widgets/dataaccessmodel.h
        widgets/eventsmodel.h
        widgets/historymodel.h
        widgets/logitem.h
        widgets/logmodel.h
        widgets/nodeinfomodel.h
        widgets/nodeitem.h
        widgets/referencesmodel.h
        widgets/subscriptionitem.h
        widgets/subscriptionsmodel.h
    )

    target_include_directories(ouaexp_testable PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    )

    target_link_libraries(ouaexp_testable PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpcUa
    )

    if(TARGET OpenSSL::Crypto)
        target_link_libraries(ouaexp_testable PUBLIC OpenSSL::Crypto)
        target_compile_definitions(ouaexp_testable PUBLIC OUAEXP_HAS_OPENSSL)
    endif()

    if(TARGET qt${QT_VERSION_MAJOR}keychain)
        target_link_libraries(ouaexp_testable PUBLIC qt${QT_VERSION_MAJOR}keychain)
        target_compile_definitions(ouaexp_testable PUBLIC OUAEXP_HAS_QTKEYCHAIN)
    endif()

    if(MSVC)
        target_compile_options(ouaexp_testable PUBLIC /utf-8)
    endif()

    ouaexp_apply_coverage(ouaexp_testable)
    set_target_properties(ouaexp_testable PROPERTIES FOLDER "Tests")

    function(ouaexp_add_test test_name source_file)
        add_executable(${test_name} tests/${source_file})
        target_link_libraries(${test_name} PRIVATE
            ouaexp_testable
            Qt${QT_VERSION_MAJOR}::Test
        )
        ouaexp_apply_coverage(${test_name})
        set_target_properties(${test_name} PROPERTIES FOLDER "Tests")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    ouaexp_add_test(ouaexp_tests           test_opcua.cpp)
    ouaexp_add_test(ouaexp_tests_profiles  test_profiles.cpp)
    ouaexp_add_test(ouaexp_tests_models    test_models.cpp)
    ouaexp_add_test(ouaexp_tests_secrets   test_secretstore.cpp)
    ouaexp_add_test(ouaexp_tests_formatter test_attributeformatter.cpp)

    ouaexp_add_test(ouaexp_tests_integration test_opcua_integration.cpp)
    target_compile_definitions(ouaexp_tests_integration PRIVATE
        OUAEXP_TEST_SERVER_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/../tools/opcua_test_server.py")
endfunction()
