cmake_minimum_required(VERSION 3.14)
project(cppdb VERSION 0.5.2)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(WITH_SQLITE "Enable SQLite backend" OFF)
option(WITH_MYSQL "Enable MySQL backend" ON)
option(WITH_PQ "Enable PostgreSQL backend" OFF)
option(WITH_ODBC "Enable ODBC backend" OFF)
option(WITH_TEST "Enable Tests" OFF)
option(WITH_EXAMPLE "Enable Examples" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for shared library

# Basic common configuration

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Main library configuration

set(CPPDB_SRC
    src/utils.cpp
    src/driver_manager.cpp
    src/conn_manager.cpp
    src/pool.cpp
    src/backend.cpp
    src/frontend.cpp)

add_library(${PROJECT_NAME} STATIC ${CPPDB_SRC})
add_library(cppdb::cppdb ALIAS ${PROJECT_NAME})

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

target_compile_definitions(
  ${PROJECT_NAME} PRIVATE CPPDB_MAJOR=${PROJECT_VERSION_MAJOR} CPPDB_MINOR=${PROJECT_VERSION_MINOR}
                          CPPDB_PATCH=${PROJECT_VERSION_PATCH} CPPDB_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -Wno-potentially-evaluated-expression)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -Wno-potentially-evaluated-expression)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /W3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
  if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    target_compile_options(${PROJECT_NAME} PRIVATE -mt)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Backend configuration

if(WITH_SQLITE)
  find_package(SQLite3 REQUIRED)
  if(NOT TARGET SQLite3::SQLite3) # CMake < 4.3
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
  endif()

  add_library(${PROJECT_NAME}_sqlite3 INTERFACE)
  target_link_libraries(${PROJECT_NAME}_sqlite3 INTERFACE SQLite3::SQLite3)
  add_library(cppdb::sqlite3 ALIAS ${PROJECT_NAME}_sqlite3)
endif()

if(WITH_MYSQL)
  find_package(MYSQL REQUIRED)
  add_library(${PROJECT_NAME}_mysql INTERFACE)
  target_link_libraries(${PROJECT_NAME}_mysql INTERFACE MYSQL::MYSQL)
  add_library(cppdb::mysql ALIAS ${PROJECT_NAME}_mysql)
endif()

if(WITH_PQ)
  find_package(PostgreSQL REQUIRED)
  add_library(${PROJECT_NAME}_postgresql INTERFACE)
  target_link_libraries(${PROJECT_NAME}_postgresql INTERFACE PostgreSQL::PostgreSQL)
  add_library(cppdb::postgresql ALIAS ${PROJECT_NAME}_postgresql)
endif()

if(WITH_PQ)
  find_package(ODBC REQUIRED)
  add_library(${PROJECT_NAME}_odbc INTERFACE)
  target_link_libraries(${PROJECT_NAME}_odbc INTERFACE ODBC::ODBC)
  add_library(cppdb::odbc ALIAS ${PROJECT_NAME}_odbc)
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
  target_link_libraries(example cppdb::cppdb cppdb::sqlite3)
  add_executable(example_logging examples/examplelogging.cpp)
  target_link_libraries(example_logging cppdb::cppdb cppdb::mysql)
endif()

include(GNUInstallDirs)

install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cppdb DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
