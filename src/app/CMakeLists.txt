add_executable(${PROJECT_NAME}
    main.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE ouaexp_ui)
ouaexp_copy_runtime(${PROJECT_NAME})

if(WIN32)
    ouaexp_configure_target_windows(${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/ouaexp.rc.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/light/app.ico"
    )
elseif(APPLE)
    ouaexp_configure_target_macos(${PROJECT_NAME})
elseif(LINUX)
    ouaexp_configure_target_linux(${PROJECT_NAME})
endif()
