set(PROJECT_PRERELEASE "" CACHE STRING "pre-release suffix, e.g. beta1, rc1")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

string(TIMESTAMP CURRENT_YEAR "%Y")

function(ouaexp_apply_project_version_suffix)
    if(DEFINED ENV{GITHUB_REF})
        string(REGEX MATCH "refs/heads/(.*)" MATCH_RESULT $ENV{GITHUB_REF})
        if(MATCH_RESULT AND CMAKE_MATCH_1)
            set(GIT_BRANCH ${CMAKE_MATCH_1})
        endif()
    else()
        find_package(Git QUIET)
        if(GIT_FOUND)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        else()
            message(STATUS "Git not found - branch detection is not possible")
        endif()
    endif()

    if(DEFINED GIT_BRANCH AND "${GIT_BRANCH}" MATCHES "^(dev|release/.*)$")
        set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}-dev" PARENT_SCOPE)
    elseif(PROJECT_PRERELEASE)
        set(PROJECT_VERSION "${PROJECT_VERSION}-${PROJECT_PRERELEASE}" PARENT_SCOPE)
    endif()
endfunction()
