function(ouaexp_configure_tests)
    enable_testing()

    add_executable(ouaexp_tests
        tests/test_opcua.cpp
        opcua/connectionprofilestore.cpp
        opcua/opcuaclientservice.cpp
        opcua/pkimanager.cpp
        loggingcategories.cpp
        widgets/addressspacemodel.cpp
        widgets/attributesmodel.cpp
        opcua/connectionprofile.h
        opcua/connectionprofilestore.h
        opcua/opcuaclientservice.h
        opcua/opcuatypes.h
        opcua/pkimanager.h
        widgets/addressspaceitem.h
        widgets/addressspacemodel.h
        widgets/attributesmodel.h
    )

    target_include_directories(ouaexp_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    )

    target_link_libraries(ouaexp_tests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::OpcUa
        Qt${QT_VERSION_MAJOR}::Test
    )

    if(TARGET OpenSSL::Crypto)
        target_link_libraries(ouaexp_tests PRIVATE OpenSSL::Crypto)
        target_compile_definitions(ouaexp_tests PRIVATE OUAEXP_HAS_OPENSSL)
    endif()

    if(MSVC)
        target_compile_options(ouaexp_tests PRIVATE /utf-8)
    endif()

    add_test(NAME ouaexp_tests COMMAND ouaexp_tests)
endfunction()
