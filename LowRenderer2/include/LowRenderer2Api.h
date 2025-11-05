#ifndef LOW_RENDERER2_EXPORT_H
#define LOW_RENDERER2_EXPORT_H

/* Cross-platform export/import + visibility for the lowrenderer2
   library.

   Conventions:
     - lowrenderer2_EXPORTS             defined when building the
   shared library
     - lowrenderer2_BUILT_AS_STATIC     defined when building/using a
   static lib

   Compatibility:
     - Accepts legacy typo: lowrderer2_BUILT_AS_STATIC
     - Provides alias LOW_RENDERER_API for older code
*/

#if defined(lowrenderer2_BUILT_AS_STATIC) ||                         \
    defined(lowrderer2_BUILT_AS_STATIC)
/* Static library: no import/export decorations */
#define LOW_RENDERER2_API
#if defined(__GNUC__) || defined(__clang__)
#define LOWRENDERER2_NO_EXPORT __attribute__((visibility("hidden")))
#else
#define LOWRENDERER2_NO_EXPORT
#endif

#else /* Shared library */

/* Windows family */
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(lowrenderer2_EXPORTS)
/* Building the DLL */
#define LOW_RENDERER2_API __declspec(dllexport)
#else
/* Using the DLL */
#define LOW_RENDERER2_API __declspec(dllimport)
#endif
#define LOWRENDERER2_NO_EXPORT

/* Non-Windows (ELF/Mach-O) */
#else
#if defined(__GNUC__) || defined(__clang__)
#define LOW_RENDERER2_API __attribute__((visibility("default")))
#define LOWRENDERER2_NO_EXPORT __attribute__((visibility("hidden")))
#else
#define LOW_RENDERER2_API
#define LOWRENDERER2_NO_EXPORT
#endif
#endif

#endif /* shared vs static */

/* Deprecation helpers */
#ifndef LOWRENDERER2_DEPRECATED
#if defined(_WIN32) || defined(__CYGWIN__)
#define LOWRENDERER2_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#define LOWRENDERER2_DEPRECATED __attribute__((deprecated))
#else
#define LOWRENDERER2_DEPRECATED
#endif
#endif

#ifndef LOWRENDERER2_DEPRECATED_EXPORT
#define LOWRENDERER2_DEPRECATED_EXPORT                               \
  LOW_RENDERER2_API LOWRENDERER2_DEPRECATED
#endif

#ifndef LOWRENDERER2_DEPRECATED_NO_EXPORT
#define LOWRENDERER2_DEPRECATED_NO_EXPORT                            \
  LOWRENDERER2_NO_EXPORT LOWRENDERER2_DEPRECATED
#endif

/* Optional global switch to drop deprecated APIs at compile time */
#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWRENDERER2_NO_DEPRECATED
#define LOWRENDERER2_NO_DEPRECATED
#endif
#endif

/* Backward-compat aliases */
#ifndef LOW_RENDERER_API
#define LOW_RENDERER_API LOW_RENDERER2_API
#endif
#ifndef LOWRENDERER_NO_EXPORT
#define LOWRENDERER_NO_EXPORT LOWRENDERER2_NO_EXPORT
#endif

#endif /* LOW_RENDERER2_EXPORT_H */
