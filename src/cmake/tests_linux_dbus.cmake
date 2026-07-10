function(ouaexp_configure_linux_dbus_tests)
    if(NOT Qt${QT_VERSION_MAJOR}DBus_FOUND)
        return()
    endif()

    ouaexp_add_ui_test(ouaexp_tests_theme_dbus test_theme_dbus.cpp)
    target_link_libraries(ouaexp_tests_theme_dbus PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
    target_compile_definitions(ouaexp_tests_theme_dbus PRIVATE HAS_QTDBUS)
endfunction()
