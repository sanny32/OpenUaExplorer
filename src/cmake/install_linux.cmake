set(APP_EXECUTABLE_NAME "${PROJECT_NAME}")

# Most distributions do not package Qt OpcUa, so cmake/qtopcua.cmake builds it and
# the install tree has to carry it. The real binary is installed next to the bundled
# library, which an $ORIGIN RPATH resolves, and Qt reads /proc/self/exe, so
# applicationDirPath() points at that private directory even when the program is
# started through the symlink in bindir - the opcua plugin beside it is found with
# no QT_PLUGIN_PATH.
set(OUAEXP_PRIVATE_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")

if(IS_DIRECTORY "${QTOPCUA_INSTALL_DIR}")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE)

    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${OUAEXP_PRIVATE_LIBDIR}")

    install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::OpcUa>"
        DESTINATION "${OUAEXP_PRIVATE_LIBDIR}")

    install(CODE "
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink
            \"$<TARGET_FILE_NAME:Qt${QT_VERSION_MAJOR}::OpcUa>\"
            \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${OUAEXP_PRIVATE_LIBDIR}/$<TARGET_SONAME_FILE_NAME:Qt${QT_VERSION_MAJOR}::OpcUa>\")
    ")

    if(TARGET Qt${QT_VERSION_MAJOR}::QOpen62541Plugin)
        install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QOpen62541Plugin>"
            DESTINATION "${OUAEXP_PRIVATE_LIBDIR}/opcua")
    endif()

    install(DIRECTORY DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(CODE "
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink
            \"../${OUAEXP_PRIVATE_LIBDIR}/${APP_EXECUTABLE_NAME}\"
            \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${APP_EXECUTABLE_NAME}\")
    ")
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
