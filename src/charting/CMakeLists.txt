# Self-contained charting library.
#
# The Qt Charts dependency is intentionally confined to this target and linked
# PRIVATE so it never propagates to consumers (ouaexp_ui and below). Swapping the
# backend means editing only this directory and chartviewfactory.cpp.

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Charts REQUIRED)

add_library(ouaexp_charting STATIC
    charttypes.h
    ichartview.h
    chartviewfactory.cpp
    chartviewfactory.h
    backends/qtcharts/qtchartsview.cpp
    backends/qtcharts/qtchartsview.h
)

target_include_directories(ouaexp_charting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ouaexp_charting
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Charts
)

if(MSVC)
    target_compile_options(ouaexp_charting PRIVATE /utf-8)
endif()
