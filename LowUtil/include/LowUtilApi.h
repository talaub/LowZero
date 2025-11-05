#ifndef LOW_EXPORT_H
#define LOW_EXPORT_H

// Detect Windows (MSVC/MinGW/Cygwin) vs. everything else
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(lowutil_BUILT_AS_STATIC)
#define LOW_EXPORT
#define LOWUTIL_NO_EXPORT
#else
#if defined(lowutil_EXPORTS)
// Building the DLL
#define LOW_EXPORT __declspec(dllexport)
#else
// Using the DLL
#define LOW_EXPORT __declspec(dllimport)
#endif
#define LOWUTIL_NO_EXPORT
#endif

// Deprecated annotations (Windows)
#ifndef LOWUTIL_DEPRECATED
#define LOWUTIL_DEPRECATED __declspec(deprecated)
#endif
#ifndef LOWUTIL_DEPRECATED_EXPORT
#define LOWUTIL_DEPRECATED_EXPORT LOW_EXPORT LOWUTIL_DEPRECATED
#endif
#ifndef LOWUTIL_DEPRECATED_NO_EXPORT
#define LOWUTIL_DEPRECATED_NO_EXPORT                                 \
  LOWUTIL_NO_EXPORT LOWUTIL_DEPRECATED
#endif

#else // Non-Windows (Linux/macOS, Clang/GCC)

#if defined(lowutil_BUILT_AS_STATIC)
#define LOW_EXPORT
#define LOWUTIL_NO_EXPORT
#else
// On ELF/Mach-O, use visibility attributes
#if __GNUC__ >= 4 || defined(__clang__)
#define LOW_EXPORT __attribute__((visibility("default")))
#define LOWUTIL_NO_EXPORT __attribute__((visibility("hidden")))
#else
// Very old compilers: no visibility support
#define LOW_EXPORT
#define LOWUTIL_NO_EXPORT
#endif
#endif

// Deprecated annotations (non-Windows)
#ifndef LOWUTIL_DEPRECATED
#if __GNUC__ >= 4 || defined(__clang__)
#define LOWUTIL_DEPRECATED __attribute__((deprecated))
#else
#define LOWUTIL_DEPRECATED
#endif
#endif
#ifndef LOWUTIL_DEPRECATED_EXPORT
#define LOWUTIL_DEPRECATED_EXPORT LOW_EXPORT LOWUTIL_DEPRECATED
#endif
#ifndef LOWUTIL_DEPRECATED_NO_EXPORT
#define LOWUTIL_DEPRECATED_NO_EXPORT                                 \
  LOWUTIL_NO_EXPORT LOWUTIL_DEPRECATED
#endif

#endif // platform split

// Optional switch to disable deprecated APIs project-wide
#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWUTIL_NO_DEPRECATED
#define LOWUTIL_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
