set(APP_EXECUTABLE_NAME "${PROJECT_NAME}")
set(OUAEXP_LINUX_DATA_DIR "${PROJECT_SOURCE_DIR}/../.github/linux/usr/share")

install(FILES
    "${OUAEXP_LINUX_DATA_DIR}/applications/io.github.sanny32.OpenUaExplorer.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(DIRECTORY
    "${OUAEXP_LINUX_DATA_DIR}/icons/hicolor"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons")
install(FILES
    "${OUAEXP_LINUX_DATA_DIR}/mime/packages/io.github.sanny32.OpenUaExplorer.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")

option(OUAEXP_INSTALL_PRIVATE_LIBDIR
    "Install the executable and the bundled Qt OpcUa into a private directory under libdir"
    ON)

set(OUAEXP_PRIVATE_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")

# linuxdeployqt hard-codes usr/lib, so the multiarch libdir must not be used there.
set(OUAEXP_APPDIR_LIBDIR "lib")

if(NOT IS_DIRECTORY "${QTOPCUA_INSTALL_DIR}")
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
elseif(OUAEXP_INSTALL_PRIVATE_LIBDIR)
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
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${OUAEXP_APPDIR_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH FALSE)

    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

    install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::OpcUa>"
        DESTINATION "${OUAEXP_APPDIR_LIBDIR}")

    install(CODE "
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink
            \"$<TARGET_FILE_NAME:Qt${QT_VERSION_MAJOR}::OpcUa>\"
            \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${OUAEXP_APPDIR_LIBDIR}/$<TARGET_SONAME_FILE_NAME:Qt${QT_VERSION_MAJOR}::OpcUa>\")
    ")

    if(TARGET Qt${QT_VERSION_MAJOR}::QOpen62541Plugin)
        install(FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QOpen62541Plugin>"
            DESTINATION "plugins/opcua")
    endif()
endif()
