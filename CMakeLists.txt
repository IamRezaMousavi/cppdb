cmake_minimum_required(VERSION 3.10)
project(berry VERSION 0.4.2)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CPPDB_VERSION ${PROJECT_VERSION})
set(CPPDB_SOVERSION ${PROJECT_VERSION_MAJOR})

option(WITH_SQLITE  "Enable SQLite backend"     OFF)
option(WITH_MYSQL   "Enable MySQL backend"      ON)
option(WITH_PQ      "Enable PostgreSQL backend" OFF)
option(WITH_ODBC    "Enable ODBC backend"       OFF)
option(WITH_TEST    "Enable Tests"              OFF)
option(WITH_EXAMPLE "Enable Examples"           OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # for shared library

# General settings

add_definitions(-DCPPDB_LIBRARY_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}")
add_definitions(-DCPPDB_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
add_definitions(-DCPPDB_SOVERSION="${CPPDB_SOVERSION}")

# Basic common configuration

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Backend configuration

set(INTERNAL_SOURCES)
set(INTERNAL_LIBRARIES)

if(WITH_SQLITE)
	find_library(SQLITE3_LIB sqlite3)
	find_path(SQLITE3_PATH sqlite3.h)
	if(SQLITE3_LIB AND SQLITE3_PATH)
		include_directories(${SQLITE3_PATH})
		add_definitions(-DCPPDB_WITH_SQLITE3)
		set(INTERNAL_SOURCES ${INTERNAL_SOURCES} drivers/sqlite3_backend.cpp)
		set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${SQLITE3_LIB})
	else()
		message("-- sqlite3 library was not found, disabling sqlite3 backend")
	endif()
endif()

if(WITH_MYSQL)
	find_package(MySQL REQUIRED)
	if(MySQL_FOUND)
		include_directories(${MySQL_INCLUDE_DIRS})
		set(INTERNAL_SOURCES ${INTERNAL_SOURCES} drivers/mysql_backend.cpp)
		set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${MySQL_LIBRARIES})
		add_definitions(-DCPPDB_WITH_MYSQL)
	else()
		message("-- mysqlclient not found, disableing mysql backend")
	endif()
endif()

if(WITH_PQ)
	find_library(PQ_LIB pq)
	find_path(PQ_PATH libpq-fe.h PATH_SUFFIXES postgresql pgsql)
	if(PQ_LIB AND PQ_PATH)
		include_directories(${PQ_PATH})
		set(INTERNAL_SOURCES ${INTERNAL_SOURCES} drivers/postgres_backend.cpp)
		set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${PQ_LIB})
		add_definitions(-DCPPDB_WITH_PQ)
	else()
		message("-- libpq not found, disableing postgresql backend")
	endif()
endif()

if(WITH_ODBC)
	find_library(ODBC_LIB odbc NAMES odbc32)
	find_path(ODBC_PATH sqlext.h)
	if(ODBC_LIB AND ODBC_PATH)
		include_directories(${ODBC_PATH})
		set(INTERNAL_SOURCES ${INTERNAL_SOURCES} drivers/odbc_backend.cpp)
		set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${ODBC_LIB})
		add_definitions(-DCPPDB_WITH_ODBC)
	else()
		message("-- odbc library was not found, disabling odbc backend")
	endif()
endif()


# Main library configuration

set(CPPDB_SRC
	src/utils.cpp
	src/driver_manager.cpp
	src/conn_manager.cpp
	src/shared_object.cpp
	src/pool.cpp
	src/backend.cpp
	src/frontend.cpp
	${INTERNAL_SOURCES}
)

add_library(${PROJECT_NAME} STATIC ${CPPDB_SRC})
target_compile_definitions(${PROJECT_NAME} PRIVATE
	CPPDB_EXPORTS

	CPPDB_MAJOR=${PROJECT_VERSION_MAJOR}
	CPPDB_MINOR=${PROJECT_VERSION_MINOR}
	CPPDB_PATCH=${PROJECT_VERSION_PATCH}
	CPPDB_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${CPPDB_SOVERSION} SOVERSION ${CPPDB_SOVERSION})
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${CPPDB_VERSION} SOVERSION ${CPPDB_SOVERSION})
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wall -Wextra -pedantic -Werror
		-Wno-potentially-evaluated-expression
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wall -Wextra -pedantic -Werror
		-Wno-potentially-evaluated-expression
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wall
	)
elseif(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/EHsc /W3
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
	if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
		target_compile_options(${PROJECT_NAME} PRIVATE
			-mt
		)
	endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${INTERNAL_LIBRARIES}
)

if(NOT WIN32)
    find_library(DL_LIB dl)
    if(DL_LIB)
        target_link_libraries(${PROJECT_NAME} PUBLIC
            ${DL_LIB}
        )
    endif()
endif()

# tests
if(WITH_TEST)
    add_executable(test_perf test/test_perf.cpp)
    add_executable(test_basic test/test_basic.cpp)
    add_executable(test_backend test/test_backend.cpp)
    add_executable(test_caching test/test_caching.cpp)
    target_link_libraries(test_perf ${PROJECT_NAME})
    target_link_libraries(test_basic ${PROJECT_NAME})
    target_link_libraries(test_backend ${PROJECT_NAME})
    target_link_libraries(test_caching ${PROJECT_NAME})
endif()

# examples
if(WITH_EXAMPLE)
    add_executable(example examples/example1.cpp)
    target_link_libraries(example ${PROJECT_NAME} ${INTERNAL_LIBRARIES})
	add_executable(example_logging examples/examplelogging.cpp)
	target_link_libraries(example_logging ${PROJECT_NAME} ${INTERNAL_LIBRARIES})
endif()

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY
	${CMAKE_CURRENT_SOURCE_DIR}/cppdb
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
