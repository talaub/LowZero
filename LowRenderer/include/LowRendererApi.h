// low_renderer_export.h
#ifndef LOW_RENDERER_EXPORT_H
#define LOW_RENDERER_EXPORT_H
// --- Build mode: static vs shared ---
#if defined(LOW_RENDERER_BUILT_AS_STATIC)
// Building/using a static lib: export/import attributes are no-ops.
#define LOW_RENDERER_API
#define LOW_RENDERER_NO_EXPORT
#else
// --- Platform-specific export/import ---
#if defined(_WIN32) || defined(_WIN64)
// When building the DLL define LOW_RENDERER_EXPORTS in your build
// system.
#ifdef LOW_RENDERER_EXPORTS
#define LOW_RENDERER_API __declspec(dllexport)
#else
#define LOW_RENDERER_API __declspec(dllimport)
#endif
#define LOW_RENDERER_NO_EXPORT
#else
// ELF platforms (Linux, *BSD, etc.)
#if __GNUC__ >= 4 || defined(__clang__)
#define LOW_RENDERER_API __attribute__((visibility("default")))
#define LOW_RENDERER_NO_EXPORT __attribute__((visibility("hidden")))
#else
#define LOW_RENDERER_API
#define LOW_RENDERER_NO_EXPORT
#endif
#endif
#endif // LOW_RENDERER_BUILT_AS_STATIC

// --- Calling convention (optional) ---
// Keep stdcall for legacy 32-bit Windows if needed; no-op elsewhere.
#if defined(_WIN32) && !defined(_WIN64)
#define LOW_RENDERER_CALL __stdcall
#else
#define LOW_RENDERER_CALL
#endif

// --- Deprecation helpers ---
#if defined(_MSC_VER)
#define LOW_RENDERER_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#define LOW_RENDERER_DEPRECATED __attribute__((deprecated))
#else
#define LOW_RENDERER_DEPRECATED
#endif

#define LOW_RENDERER_DEPRECATED_EXPORT                               \
  LOW_RENDERER_API LOW_RENDERER_DEPRECATED
#define LOW_RENDERER_DEPRECATED_NO_EXPORT                            \
  LOW_RENDERER_NO_EXPORT LOW_RENDERER_DEPRECATED

// If you ever want to disable deprecated entirely, you can define
// this in your build: #define LOW_RENDERER_NO_DEPRECATED 1
#if defined(LOW_RENDERER_NO_DEPRECATED)
#undef LOW_RENDERER_DEPRECATED
#define LOW_RENDERER_DEPRECATED
#undef LOW_RENDERER_DEPRECATED_EXPORT
#define LOW_RENDERER_DEPRECATED_EXPORT LOW_RENDERER_API
#undef LOW_RENDERER_DEPRECATED_NO_EXPORT
#define LOW_RENDERER_DEPRECATED_NO_EXPORT LOW_RENDERER_NO_EXPORT
#endif

#endif // LOW_RENDERER_EXPORT_H
