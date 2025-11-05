#ifndef LOW_EXPORT_H
#define LOW_EXPORT_H

/* Cross-platform export/import + visibility for the lowmath library.
   Conventions preserved:
     - lowmath_EXPORTS        defined when building the lowmath
   library
     - lowmath_BUILT_AS_STATIC defined when building/using a static
   lib
*/

#if defined(_WIN32) || defined(__CYGWIN__)

#if defined(lowmath_BUILT_AS_STATIC)
#define LOW_EXPORT
#define LOWMATH_NO_EXPORT
#else
#if defined(lowmath_EXPORTS)
/* Building the DLL */
#define LOW_EXPORT __declspec(dllexport)
#else
/* Using the DLL */
#define LOW_EXPORT __declspec(dllimport)
#endif
#define LOWMATH_NO_EXPORT
#endif

/* Deprecation (Windows) */
#ifndef LOWMATH_DEPRECATED
#define LOWMATH_DEPRECATED __declspec(deprecated)
#endif

#else /* Non-Windows (ELF/Mach-O: Linux/macOS, Clang/GCC) */

#if defined(lowmath_BUILT_AS_STATIC)
#define LOW_EXPORT
#define LOWMATH_NO_EXPORT
#else
/* Default-visible for API surface; hide others via
 * -fvisibility=hidden or CMake props */
#if defined(__GNUC__) || defined(__clang__)
#define LOW_EXPORT __attribute__((visibility("default")))
#define LOWMATH_NO_EXPORT __attribute__((visibility("hidden")))
#else
#define LOW_EXPORT
#define LOWMATH_NO_EXPORT
#endif
#endif

/* Deprecation (non-Windows) */
#ifndef LOWMATH_DEPRECATED
#if defined(__GNUC__) || defined(__clang__)
#define LOWMATH_DEPRECATED __attribute__((deprecated))
#else
#define LOWMATH_DEPRECATED
#endif
#endif

#endif /* platform split */

/* Convenience macros combining export/visibility with deprecation */
#ifndef LOWMATH_DEPRECATED_EXPORT
#define LOWMATH_DEPRECATED_EXPORT LOW_EXPORT LOWMATH_DEPRECATED
#endif

#ifndef LOWMATH_DEPRECATED_NO_EXPORT
#define LOWMATH_DEPRECATED_NO_EXPORT                                 \
  LOWMATH_NO_EXPORT LOWMATH_DEPRECATED
#endif

/* Optional global switch to compile out deprecated APIs */
#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWMATH_NO_DEPRECATED
#define LOWMATH_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
