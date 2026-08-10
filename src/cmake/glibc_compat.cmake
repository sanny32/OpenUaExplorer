# Keeps the Linux packages loadable on the oldest distribution they target.
#
# The packages are built in a container whose glibc is newer than that of the
# oldest target, so the linker is free to pick symbol versions the target has
# never had. glibc_compat.h pins the ones glibc has revisioned back to the
# baseline; it is forced into every translation unit compiled from here on,
# which is why this module is included before any target is defined.
#
# The package workflows check the result: their Inspect step refuses a bundle
# that asks for a newer glibc than the minimum supported.

if(LINUX AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-include "${CMAKE_CURRENT_LIST_DIR}/glibc_compat.h")

    # The header names math symbols in translation units that would otherwise not
    # need libm, and an indirect DT_NEEDED does not satisfy the linker.
    link_libraries(m)

    # GCC hands the top-level asm of a translation unit to one LTO partition, so
    # the directives would cover only part of the program. A single partition puts
    # them ahead of every reference again.
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-flto-partition=none)
        add_link_options(-flto-partition=none)
    endif()
endif()
