#ifndef LOW_CORE_EXPORT_H
#define LOW_CORE_EXPORT_H

/* Define LOW_CORE_API and related macros for both Windows and Unix-like compilers.
   Usage: prefix any symbol that should be exported from the shared library with LOW_CORE_API.
   Example:
       class LOW_CORE_API Foo { ... };
       LOW_CORE_API void do_stuff();
*/

/* If building/using a static library, no import/export attributes are needed. */
#ifdef lowcore_BUILT_AS_STATIC
#  define LOW_CORE_API
#  define LOWCORE_NO_EXPORT
#else

/* ----- Platform-specific export/import ----- */
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef lowcore_EXPORTS
       /* Building the DLL */
#      define LOW_CORE_API __declspec(dllexport)
#    else
       /* Using the DLL */
#      define LOW_CORE_API __declspec(dllimport)
#    endif
#    define LOWCORE_NO_EXPORT
#  elif defined(__GNUC__) || defined(__clang__)
     /* GCC/Clang: use ELF symbol visibility */
#    if __GNUC__ >= 4
#      define LOW_CORE_API __attribute__((visibility("default")))
#      define LOWCORE_NO_EXPORT __attribute__((visibility("hidden")))
#    else
#      define LOW_CORE_API
#      define LOWCORE_NO_EXPORT
#    endif
#  else
     /* Fallback: unknown compiler */
#    define LOW_CORE_API
#    define LOWCORE_NO_EXPORT
#  endif

#endif /* !lowcore_BUILT_AS_STATIC */

/* ----- Deprecation helpers ----- */
#if defined(_MSC_VER)
#  define LOWCORE_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#  define LOWCORE_DEPRECATED __attribute__((deprecated))
#else
#  define LOWCORE_DEPRECATED
#endif

#ifndef LOWCORE_DEPRECATED_EXPORT
#  define LOWCORE_DEPRECATED_EXPORT LOW_CORE_API LOWCORE_DEPRECATED
#endif

#ifndef LOWCORE_DEPRECATED_NO_EXPORT
#  define LOWCORE_DEPRECATED_NO_EXPORT LOWCORE_NO_EXPORT LOWCORE_DEPRECATED
#endif

/* Optionally allow projects to disable deprecated declarations by defining LOWCORE_NO_DEPRECATED. */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOWCORE_NO_DEPRECATED
#    define LOWCORE_NO_DEPRECATED
#  endif
#endif

#endif /* LOW_CORE_EXPORT_H */
