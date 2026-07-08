add_library(ouaexp_plugins STATIC
    servicemodule.h
    servicecontext.cpp
    servicecontext.h
    servicemodulemanager.cpp
    servicemodulemanager.h
    servermodule.cpp
    servermodule.h
    addressspacemodule.cpp
    addressspacemodule.h
    attributemodule.cpp
    attributemodule.h
    referencemodule.cpp
    referencemodule.h
    dataaccessmodule.cpp
    dataaccessmodule.h
    eventsmodule.cpp
    eventsmodule.h
)

target_include_directories(ouaexp_plugins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ouaexp_plugins PUBLIC ouaexp_core)

ouaexp_apply_coverage(ouaexp_plugins)
ouaexp_apply_pch(ouaexp_plugins)

if(MSVC)
    target_compile_options(ouaexp_plugins PUBLIC /utf-8)
endif()
