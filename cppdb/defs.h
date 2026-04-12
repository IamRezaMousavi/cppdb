#ifndef CPPDB_DEFS_H
#define CPPDB_DEFS_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
#  if defined(DLL_EXPORT) || defined(CPPDB_EXPORTS) || defined(CPPDB_DRIVER_EXPORTS)
#    ifdef CPPDB_SOURCE
#      define CPPDB_API __declspec(dllexport)
#    else
#      define CPPDB_API __declspec(dllimport)
#    endif
#  endif
#  if defined(DLL_EXPORT) || defined(CPPDB_DRIVER_EXPORTS)
#    ifdef CPPDB_DRIVER_SOURCE
#      define CPPDB_DRIVER_API __declspec(dllexport)
#    else
#      define CPPDB_DRIVER_API __declspec(dllimport)
#    endif
#  endif
#endif


#ifndef CPPDB_API
#  define CPPDB_API
#endif

#ifndef CPPDB_DRIVER_API
#  define CPPDB_DRIVER_API
#endif

#ifndef CPPDB_VERSION
#define CPPDB_VERSION ""
#endif

#ifndef CPPDB_MAJOR
#define CPPDB_MAJOR 0
#endif

#ifndef CPPDB_MINOR
#define CPPDB_MINOR 0
#endif

#ifndef CPPDB_PATCH
#define CPPDB_PATCH 0
#endif

#ifndef CPPDB_SOVERSION
#define CPPDB_SOVERSION ""
#endif

#ifndef CPPDB_LIBRARY_PREFIX
#define CPPDB_LIBRARY_PREFIX ""
#endif

#ifndef CPPDB_LIBRARY_SUFFIX
#define CPPDB_LIBRARY_SUFFIX ""
#endif

#endif
