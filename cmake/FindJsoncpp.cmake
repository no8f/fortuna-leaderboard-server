# Find jsoncpp
#
# Updated version that avoids exec_program (deprecated in CMake 3.28, removed in 4.0).
#
# This module defines:
#   JSONCPP_INCLUDE_DIRS  - include directories for jsoncpp
#   JSONCPP_LIBRARIES     - libraries to link against
#   JSONCPP_FOUND         - true if jsoncpp was found
#   Jsoncpp_lib           - the imported target library

find_path(JSONCPP_INCLUDE_DIRS
          NAMES json/json.h
          DOC "jsoncpp include dir"
          PATH_SUFFIXES jsoncpp)

find_library(JSONCPP_LIBRARIES NAMES jsoncpp DOC "jsoncpp library")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jsoncpp
                                  DEFAULT_MSG
                                  JSONCPP_INCLUDE_DIRS
                                  JSONCPP_LIBRARIES)
mark_as_advanced(JSONCPP_INCLUDE_DIRS JSONCPP_LIBRARIES)

if(Jsoncpp_FOUND)
    if(NOT EXISTS "${JSONCPP_INCLUDE_DIRS}/json/version.h")
        message(FATAL_ERROR "jsoncpp version.h not found – library may be too old")
    endif()

    # Read version string using execute_process instead of exec_program
    file(STRINGS "${JSONCPP_INCLUDE_DIRS}/json/version.h" _jsoncpp_ver_line
         REGEX "#define JSONCPP_VERSION_STRING")
    if(_jsoncpp_ver_line)
        string(REGEX REPLACE ".*JSONCPP_VERSION_STRING[ \t]+\"([^\"]+)\".*"
               "\\1" jsoncpp_ver "${_jsoncpp_ver_line}")
        message(STATUS "jsoncpp version: ${jsoncpp_ver}")
        if(jsoncpp_ver VERSION_LESS "1.7")
            message(FATAL_ERROR "jsoncpp is too old (${jsoncpp_ver}). Need >= 1.7")
        endif()
    endif()

    if(NOT TARGET Jsoncpp_lib)
        add_library(Jsoncpp_lib INTERFACE IMPORTED)
    endif()
    set_target_properties(Jsoncpp_lib
                          PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_INCLUDE_DIRS}"
                          INTERFACE_LINK_LIBRARIES      "${JSONCPP_LIBRARIES}")
endif()
