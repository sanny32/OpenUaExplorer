function(ouaexp_configure_tests)
    # Static library with the transport-neutral production sources exercised by
    # the test binaries. Keeping the list in one place avoids duplicating it
    # across the per-area test executables created below.
    add_library(ouaexp_testable STATIC
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

    # Propagate the optional backends (and their feature macros) to every test
    # executable that links the library, so the tests can branch on them too.
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

    # Instrument the shared production sources when coverage is requested.
    ouaexp_apply_coverage(ouaexp_testable)

    # Creates one Qt Test executable per area and registers it with CTest.
    function(ouaexp_add_test test_name source_file)
        add_executable(${test_name} tests/${source_file})
        target_link_libraries(${test_name} PRIVATE
            ouaexp_testable
            Qt${QT_VERSION_MAJOR}::Test
        )
        ouaexp_apply_coverage(${test_name})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    ouaexp_add_test(ouaexp_tests           test_opcua.cpp)
    ouaexp_add_test(ouaexp_tests_profiles  test_profiles.cpp)
    ouaexp_add_test(ouaexp_tests_models    test_models.cpp)
    ouaexp_add_test(ouaexp_tests_secrets   test_secretstore.cpp)
endfunction()
