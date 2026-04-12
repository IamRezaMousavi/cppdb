cmake_minimum_required(VERSION 3.10)
project(berry VERSION 0.3.1)

set(CPPDB_VERSION ${PROJECT_VERSION})
set(CPPDB_SOVERSION ${PROJECT_VERSION_MAJOR})

add_definitions(
    -DCPPDB_MAJOR=${PROJECT_VERSION_MAJOR}
    -DCPPDB_MINOR=${PROJECT_VERSION_MINOR}
    -DCPPDB_PATCH=${PROJECT_VERSION_PATCH}
    -DCPPDB_VERSION="${PROJECT_VERSION}"
)

option(WITH_SQLITE "Enable SQLite backend" OFF)
option(WITH_MYSQL "Enable MySQL backend" ON)
option(WITH_PQ "Enable PostgreSQL backend" OFF)
option(WITH_ODBC "Enable ODBC backend" OFF)

set(LIBDIR lib CACHE STRING "Library installation directory")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # for shared library

#add_compile_options(-Wall -Wextra -pedantic -Werror)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CXX_FLAGS "-Wall -Wextra")
	if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
		set(CXX_FLAGS "${CXX_FLAGS} -pthreads")
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	set(CXX_FLAGS "-Wall")
elseif(MSVC)
	set(CXX_FLAGS "/EHsc /W3")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
	if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
		set(CXX_FLAGS "${CXX_FLAGS} -mt")
	endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

# General settings

include_directories(.)

add_definitions(-DCPPDB_LIBRARY_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}")
add_definitions(-DCPPDB_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
add_definitions(-DCPPDB_SOVERSION="${CPPDB_SOVERSION}")

# Basic common configuration

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	      FORCE)
endif(NOT CMAKE_BUILD_TYPE)


include(CheckCXXSourceCompiles)

check_cxx_source_compiles(
	"int main() { volatile int v=0; return __sync_bool_compare_and_swap(&v,0,1); }"
	CPPDB_HAS_GCC_SYNC)
if(CPPDB_HAS_GCC_SYNC)
	add_definitions(-DCPPDB_HAS_GCC_SYNC)
endif()

check_cxx_source_compiles(
	"#include <bits/atomicity.h> 
	using __gnu_cxx::__exchange_and_add;
	int main(){ volatile int x=0; return __exchange_and_add(&x,1);}"
	CPPDB_HAVE_GCC_BITS_EXCHANGE_AND_ADD)
if(CPPDB_HAVE_GCC_BITS_EXCHANGE_AND_ADD)
	add_definitions(-DCPPDB_HAVE_GCC_BITS_EXCHANGE_AND_ADD)
endif()

check_cxx_source_compiles(
	"#include <ext/atomicity.h> 
	using __gnu_cxx::__exchange_and_add;
	int main(){ volatile int x=0; return __exchange_and_add(&x,1);}"
	CPPDB_HAVE_GCC_EXT_EXCHANGE_AND_ADD)
if(CPPDB_HAVE_GCC_EXT_EXCHANGE_AND_ADD)
	add_definitions(-DCPPDB_HAVE_GCC_EXT_EXCHANGE_AND_ADD)
endif()

check_cxx_source_compiles(
	"#include <sys/types.h>
	#include <machine/atomic.h>
	int main() { volatile unsigned v=0; return atomic_cmpset_int(&v,1,0); }"
	CPPDB_HAVE_FREEBSD_ATOMIC)

if(CPPDB_HAVE_FREEBSD_ATOMIC)
	add_definitions(-DCPPDB_HAVE_FREEBSD_ATOMIC)
endif()

check_cxx_source_compiles(
	"#include <atomic.h>
	int main() { volatile unsigned v=0; return atomic_add_int_nv(&v,1); }"
	CPPDB_HAVE_SOLARIS_ATOMIC)
if(CPPDB_HAVE_SOLARIS_ATOMIC)
	add_definitions(-DCPPDB_HAVE_SOLARIS_ATOMIC)
endif()


check_cxx_source_compiles(
	"#include <libkern/OSAtomic.h>
	int main() { volatile int v=0; return OSAtomicAdd32(1,&v); }"
	CPPDB_HAVE_MAC_OS_X_ATOMIC)

if(CPPDB_HAVE_MAC_OS_X_ATOMIC)
	add_definitions(-DCPPDB_HAVE_MAC_OS_X_ATOMIC)
endif()

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
	find_library(MYSQL_LIB mysqlclient)
	find_path(MYSQL_PATH mysql.h PATH_SUFFIXES mysql)
	if(MYSQL_LIB AND MYSQL_PATH)
		include_directories(${MYSQL_PATH})
		set(INTERNAL_SOURCES ${INTERNAL_SOURCES} drivers/mysql_backend.cpp)
		set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${MYSQL_LIB})
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
	src/mutex.cpp
	src/driver_manager.cpp
	src/conn_manager.cpp
	src/shared_object.cpp
	src/pool.cpp
	src/backend.cpp
	src/frontend.cpp
	src/atomic_counter.cpp
	${INTERNAL_SOURCES}
)

add_library(${PROJECT_NAME} STATIC ${CPPDB_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS CPPDB_EXPORTS)

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
    find_library(PTHREAD_LIB pthread NAMES thr kse)
    if(PTHREAD_LIB)
        target_link_libraries(${PROJECT_NAME} PUBLIC
            ${PTHREAD_LIB}
        )
    endif()
endif()

# tests
option(WITH_TEST "Enable Tests" OFF)
option(WITH_TEST "Enable Tests" OFF)
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
option(WITH_EXAMPLE "Enable Examples" OFF)
if(WITH_EXAMPLE)
    add_executable(example examples/example1.cpp)
    target_link_libraries(example ${PROJECT_NAME} ${INTERNAL_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIBDIR}
	ARCHIVE DESTINATION ${LIBDIR}
	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)

install(DIRECTORY ${PROJECT_NAME} DESTINATION include)

