include(FetchContent)

option(OUAEXP_FETCH_QTKEYCHAIN
    "Fetch and build QtKeychain when it is not installed" ON)

find_package(Qt${QT_VERSION_MAJOR}Keychain CONFIG QUIET)
if(NOT TARGET qt${QT_VERSION_MAJOR}keychain AND OUAEXP_FETCH_QTKEYCHAIN)
    set(BUILD_WITH_QT6 OFF CACHE BOOL "Build QtKeychain with Qt 6" FORCE)
    if(QT_VERSION_MAJOR EQUAL 6)
        set(BUILD_WITH_QT6 ON CACHE BOOL "Build QtKeychain with Qt 6" FORCE)
    endif()
    set(BUILD_TEST_APPLICATION OFF CACHE BOOL
        "Build QtKeychain test application" FORCE)
    set(BUILD_QTQUICK_DEMO OFF CACHE BOOL
        "Build QtKeychain Qt Quick demo" FORCE)
    set(BUILD_TRANSLATIONS OFF CACHE BOOL
        "Build QtKeychain translations" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL
        "Build dependencies as static libraries" FORCE)

    FetchContent_Declare(qtkeychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG 0.16.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(qtkeychain)
endif()

function(ouaexp_configure_qtkeychain target_name)
    if(TARGET qt${QT_VERSION_MAJOR}keychain)
        target_link_libraries(${target_name} PRIVATE qt${QT_VERSION_MAJOR}keychain)
        target_compile_definitions(${target_name} PRIVATE OUAEXP_HAS_QTKEYCHAIN)
    else()
        message(WARNING
            "QtKeychain was not found. Connection secrets will not be persisted.")
    endif()
endfunction()
