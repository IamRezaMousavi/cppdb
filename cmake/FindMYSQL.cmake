# Once done this will define:
# MYSQL_FOUND			- If false, do not try to use MySQL.
# MYSQL_INCLUDE_DIRS	- Where to find mysql.h, etc.
# MYSQL_LIBRARIES		- The libraries to link against.
# MYSQL_VERSION_STRING	- Version in a string of MySQL.
#

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(PC_MySQL QUIET libmariadb)
endif()

if(WIN32)
	find_path(MYSQL_INCLUDE_DIR
		NAMES "mysql.h"
		PATHS "$ENV{PROGRAMFILES}/MySQL/*/include"
			  "$ENV{PROGRAMFILES(x86)}/MySQL/*/include"
			  "$ENV{SYSTEMDRIVE}/MySQL/*/include"
    )

	find_library(MYSQL_LIBRARY
		NAMES "mysqlclient" "mysqlclient_r"
		PATHS "$ENV{PROGRAMFILES}/MySQL/*/lib"
			  "$ENV{PROGRAMFILES(x86)}/MySQL/*/lib"
			  "$ENV{SYSTEMDRIVE}/MySQL/*/lib"
    )
else()
	find_path(MYSQL_INCLUDE_DIR
        NAMES "mysql.h" "mysql/mysql.h"
		PATHS "/usr/include/mysql"
			  "/usr/local/include/mysql"
			  "/usr/mysql/include/mysql"
        HINTS ${PC_MySQL_INCLUDE_DIRS}
    )

	find_library(MYSQL_LIBRARY
        NAMES "mysql" "mysqlclient" "libmysql" "mariadb" "mariadbclient" "libmariadb"
		PATHS "/lib/mysql"
			  "/lib/arm-linux-gnueabihf"
			  "/lib64/mysql"
			  "/usr/lib/mysql"
			  "/usr/lib64/mysql"
			  "/usr/lib/arm-linux-gnueabihf"
			  "/usr/local/lib/mysql"
			  "/usr/local/lib64/mysql"
			  "/usr/local/lib/arm-linux-gnueabihf"
			  "/usr/mysql/lib/mysql"
			  "/usr/mysql/lib64/mysql"
        HINTS ${PC_MySQL_LIBRARY_DIRS}
    )
endif()


set(MYSQL_VERSION_STRING "0.0.0")
if( MYSQL_INCLUDE_DIR AND EXISTS "${MYSQL_INCLUDE_DIR}/mysql_version.h" )
    set(regex_version "^#define[ \t]+MYSQL_SERVER_VERSION[ \t]+\"([^\"]+)\".*")
    file(STRINGS "${MYSQL_INCLUDE_DIR}/mysql_version.h" mysql_version REGEX "${regex_version}")
    string(REGEX REPLACE "${regex_version}" "\\1" MYSQL_VERSION_STRING "${mysql_version}")
    unset(regex_version)
    unset(mysql_version)
endif()

# handle the QUIETLY and REQUIRED arguments and set MYSQL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MYSQL
    REQUIRED_VARS	MYSQL_LIBRARY MYSQL_INCLUDE_DIR
	VERSION_VAR		MYSQL_VERSION_STRING
)

if (MYSQL_FOUND)
    set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})
    set(MYSQL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
    if (NOT TARGET MYSQL::MYSQL)
        add_library(MYSQL::MYSQL UNKNOWN IMPORTED)
        set_target_properties(MYSQL::MYSQL PROPERTIES
            IMPORTED_LOCATION "${MYSQL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARY)
