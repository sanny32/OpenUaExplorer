add_library(ouaexp_plugins STATIC
    plugin.h
    plugincontext.cpp
    plugincontext.h
    pluginmanager.cpp
    pluginmanager.h
    serverplugin.cpp
    serverplugin.h
    addressspaceplugin.cpp
    addressspaceplugin.h
    attributeplugin.cpp
    attributeplugin.h
    referenceplugin.cpp
    referenceplugin.h
    dataaccessplugin.cpp
    dataaccessplugin.h
)

target_include_directories(ouaexp_plugins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ouaexp_plugins PUBLIC ouaexp_core)

ouaexp_apply_coverage(ouaexp_plugins)

if(MSVC)
    target_compile_options(ouaexp_plugins PUBLIC /utf-8)
endif()
