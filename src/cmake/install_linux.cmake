set(APP_EXECUTABLE_NAME "${PROJECT_NAME}")

# Most distributions do not package Qt OpcUa, so cmake/qtopcua.cmake builds it and
# the install tree has to carry it. Two layouts do that, and they cannot be merged.
#
# The default one keeps the bundled files out of the system library directory: the
# real binary is installed next to the bundled library, which an $ORIGIN RPATH
# resolves, and Qt reads /proc/self/exe, so applicationDirPath() points at that
# private directory even when the program is started through the symlink in bindir -
# the opcua plugin beside it is found with no QT_PLUGIN_PATH.
#
# build.sh turns the option off when it hands the install tree to linuxdeployqt.
# That tool assumes an FHS AppDir: it patches the executable at its canonical path
# but derives the new RPATH from bindir, so a symlinked bindir entry leaves the real
# binary unable to find the Qt libraries deployed into usr/lib. In that layout the
# bundled library goes to lib and the plugin to plugins/opcua, which is both where
# the qt.conf linuxdeployqt writes looks for it and where the plugin's own
# $ORIGIN/../../lib RPATH resolves libQt6OpcUa - without the latter linuxdeployqt
# aborts, because it refuses to deploy a library that ldd cannot resolve.
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
