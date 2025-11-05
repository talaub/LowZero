#ifndef FLODE_EXPORT_H
#define FLODE_EXPORT_H

/* Cross-platform symbol visibility for FLODE.
   Usage: annotate public API with FLODE_API.
*/

/* Static builds: no export/import attributes needed. */
#ifdef flode_BUILT_AS_STATIC
#  define FLODE_API
#  define FLODE_NO_EXPORT
#else

/* ----- Platform/compiler specifics ----- */
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef flode_EXPORTS
       /* Building the DLL */
#      define FLODE_API __declspec(dllexport)
#    else
       /* Using the DLL */
#      define FLODE_API __declspec(dllimport)
#    endif
#    define FLODE_NO_EXPORT
#  elif defined(__GNUC__) || defined(__clang__)
     /* GCC/Clang: use ELF visibility */
#    if __GNUC__ >= 4
#      define FLODE_API __attribute__((visibility("default")))
#      define FLODE_NO_EXPORT __attribute__((visibility("hidden")))
#    else
#      define FLODE_API
#      define FLODE_NO_EXPORT
#    endif
#  else
     /* Fallback: unknown compiler */
#    define FLODE_API
#    define FLODE_NO_EXPORT
#  endif

#endif /* !flode_BUILT_AS_STATIC */

/* ----- Deprecation helpers ----- */
#if defined(_MSC_VER)
#  define FLODE_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#  define FLODE_DEPRECATED __attribute__((deprecated))
#else
#  define FLODE_DEPRECATED
#endif

#ifndef FLODE_DEPRECATED_EXPORT
#  define FLODE_DEPRECATED_EXPORT FLODE_API FLODE_DEPRECATED
#endif

#ifndef FLODE_DEPRECATED_NO_EXPORT
#  define FLODE_DEPRECATED_NO_EXPORT FLODE_NO_EXPORT FLODE_DEPRECATED
#endif

/* Optional: allow disabling deprecated via FLODE_NO_DEPRECATED. */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef FLODE_NO_DEPRECATED
#    define FLODE_NO_DEPRECATED
#  endif
#endif

#endif /* FLODE_EXPORT_H */
