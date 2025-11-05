#ifndef LOW_EDITOR_EXPORT_H
#define LOW_EDITOR_EXPORT_H

/* Define LOW_EDITOR_API and related macros for both Windows and Unix-like compilers.
   Usage: prefix any symbol that should be exported from the shared library with LOW_EDITOR_API.
*/

/* If building/using a static library, no import/export attributes are needed. */
#ifdef loweditor_BUILT_AS_STATIC
#  define LOW_EDITOR_API
#  define LOWEDITOR_NO_EXPORT
#else

/* ----- Platform-specific export/import ----- */
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef loweditor_EXPORTS
       /* Building the DLL */
#      define LOW_EDITOR_API __declspec(dllexport)
#    else
       /* Using the DLL */
#      define LOW_EDITOR_API __declspec(dllimport)
#    endif
#    define LOWEDITOR_NO_EXPORT
#  elif defined(__GNUC__) || defined(__clang__)
     /* GCC/Clang: use ELF symbol visibility */
#    if __GNUC__ >= 4
#      define LOW_EDITOR_API __attribute__((visibility("default")))
#      define LOWEDITOR_NO_EXPORT __attribute__((visibility("hidden")))
#    else
#      define LOW_EDITOR_API
#      define LOWEDITOR_NO_EXPORT
#    endif
#  else
     /* Fallback: unknown compiler */
#    define LOW_EDITOR_API
#    define LOWEDITOR_NO_EXPORT
#  endif

#endif /* !loweditor_BUILT_AS_STATIC */

/* ----- Deprecation helpers ----- */
#if defined(_MSC_VER)
#  define LOWEDITOR_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#  define LOWEDITOR_DEPRECATED __attribute__((deprecated))
#else
#  define LOWEDITOR_DEPRECATED
#endif

#ifndef LOWEDITOR_DEPRECATED_EXPORT
#  define LOWEDITOR_DEPRECATED_EXPORT LOW_EDITOR_API LOWEDITOR_DEPRECATED
#endif

#ifndef LOWEDITOR_DEPRECATED_NO_EXPORT
#  define LOWEDITOR_DEPRECATED_NO_EXPORT LOWEDITOR_NO_EXPORT LOWEDITOR_DEPRECATED
#endif

/* Optionally allow projects to disable deprecated declarations by defining LOWEDITOR_NO_DEPRECATED. */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOWEDITOR_NO_DEPRECATED
#    define LOWEDITOR_NO_DEPRECATED
#  endif
#endif

#endif /* LOW_EDITOR_EXPORT_H */
