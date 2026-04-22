find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(PC_MySQL QUIET libmariadb)
endif()

find_path(MySQL_INCLUDE_DIR
    NAMES mysql.h mysql/mysql.h
    HINTS ${PC_MySQL_INCLUDE_DIRS}
)

find_library(MySQL_LIBRARY
    NAMES libmysql mysql mysqlclient mariadb libmariadb
    HINTS ${PC_MySQL_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MySQL_LIBRARY MySQL_INCLUDE_DIR)

if(MySQL_FOUND)
	set(MySQL_INCLUDE_DIRS "${MySQL_INCLUDE_DIR}")
	set(MySQL_LIBRARIES "${MySQL_LIBRARY}")
    if(NOT TARGET MySQL::MySQL)
        add_library(MySQL::MySQL UNKNOWN IMPORTED)
        set_target_properties(MySQL::MySQL PROPERTIES
            IMPORTED_LOCATION "${MYSQL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(MySQL_INCLUDE_DIR MySQL_LIBRARY)

