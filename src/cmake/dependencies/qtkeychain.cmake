include(FetchContent)

option(OUAEXP_FETCH_QTKEYCHAIN
    "Fetch and build QtKeychain when it is not installed" ON)

set(QTKEYCHAIN_VERSION "0.16.0")

find_package(Qt6Keychain CONFIG QUIET)
if(NOT TARGET qt6keychain AND OUAEXP_FETCH_QTKEYCHAIN)
    set(BUILD_WITH_QT6 ON CACHE BOOL "Build QtKeychain with Qt 6" FORCE)
    set(BUILD_TEST_APPLICATION OFF CACHE BOOL
        "Build QtKeychain test application" FORCE)
    # Keep QtKeychain's own autotests out of our CTest run.
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree" FORCE)
    set(BUILD_QTQUICK_DEMO OFF CACHE BOOL
        "Build QtKeychain Qt Quick demo" FORCE)
    set(BUILD_TRANSLATIONS OFF CACHE BOOL
        "Build QtKeychain translations" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL
        "Build dependencies as static libraries" FORCE)

    FetchContent_Declare(qtkeychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG ${QTKEYCHAIN_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(qtkeychain)
endif()

function(ouaexp_configure_qtkeychain target_name)
    if(NOT TARGET qt6keychain)
        message(FATAL_ERROR "QtKeychain target qt6keychain is required.")
    endif()

    target_link_libraries(${target_name} PRIVATE qt6keychain)
endfunction()
