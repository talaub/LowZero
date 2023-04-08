#ifndef LOW_CORE_EXPORT_H
#define LOW_CORE_EXPORT_H

#ifdef lowcore_BUILT_AS_STATIC
#define LOW_CORE_API
#define LOWCORE_NO_EXPORT
#else
#ifndef LOW_CORE_API
#ifdef lowcore_EXPORTS
/* We are building this library */
#define LOW_CORE_API __declspec(dllexport)
#else
/* We are using this library */
#define LOW_CORE_API __declspec(dllimport)
#endif
#endif

#ifndef LOWCORE_NO_EXPORT
#define LOWCORE_NO_EXPORT
#endif
#endif

#ifndef LOWCORE_DEPRECATED
#define LOWCORE_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWCORE_DEPRECATED_EXPORT
#define LOWCORE_DEPRECATED_EXPORT LOW_CORE_API LOWCORE_DEPRECATED
#endif

#ifndef LOWCORE_DEPRECATED_NO_EXPORT
#define LOWCORE_DEPRECATED_NO_EXPORT LOWCORE_NO_EXPORT LOWCORE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWCORE_NO_DEPRECATED
#define LOWCORE_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
