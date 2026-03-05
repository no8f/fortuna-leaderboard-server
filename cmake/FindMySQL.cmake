# FindMySQL.cmake - Lightweight replacement that does not abort if MySQL is absent.
# We only need SQLite, so MySQL is optional for this project.

find_path(MYSQL_INCLUDE_DIRS
          NAMES mysql.h
          PATH_SUFFIXES mysql mariadb
          PATHS /usr/include /usr/local/include)

find_library(MYSQL_LIBRARIES
             NAMES mysqlclient_r mysqlclient mariadbclient
             PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
             PATH_SUFFIXES mysql mariadb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_INCLUDE_DIRS)
mark_as_advanced(MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES)

if(MySQL_FOUND)
    if(NOT TARGET MySQL_lib)
        add_library(MySQL_lib INTERFACE IMPORTED)
    endif()
    set_target_properties(MySQL_lib
                          PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIRS}"
                          INTERFACE_LINK_LIBRARIES      "${MYSQL_LIBRARIES}")
else()
    set(MYSQL_LIBRARIES)
    set(MYSQL_INCLUDE_DIRS)
endif()
