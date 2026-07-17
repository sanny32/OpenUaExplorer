include("${PROJECT_SOURCE_DIR}/cmake/target_windows.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/target_macos.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/target_linux.cmake")

add_executable(${PROJECT_NAME}
    main.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE ouaexp_ui)
ouaexp_copy_runtime(${PROJECT_NAME})

set(QT_SKIP_STORE_LANGUAGES_FROM_TS_FILES ON)

qt_add_translations(${PROJECT_NAME}
    TS_FILES translations/ouaexp_ru.ts
             translations/ouaexp_de.ts
             translations/ouaexp_zh_CN.ts
    RESOURCE_PREFIX "/translations"
    SOURCE_TARGETS ${PROJECT_NAME} ouaexp_ui ouaexp_core ouaexp_modules ouaexp_charting
    LUPDATE_OPTIONS -no-obsolete -locations none
)

if(WIN32)
    ouaexp_configure_target_windows(${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/ouaexp.rc.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/light/app.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/mime/file-ouas.ico"
    )
elseif(APPLE)
    ouaexp_configure_target_macos(${PROJECT_NAME})
elseif(LINUX)
    ouaexp_configure_target_linux(${PROJECT_NAME})
endif()
