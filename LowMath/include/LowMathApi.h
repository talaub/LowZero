#ifndef LOW_EXPORT_H
#define LOW_EXPORT_H

#ifdef lowmath_BUILT_AS_STATIC
#define LOW_EXPORT
#define LOWMATH_NO_EXPORT
#else
#ifndef LOW_EXPORT
#ifdef lowmath_EXPORTS
/* We are building this library */
#define LOW_EXPORT __declspec(dllexport)
#else
/* We are using this library */
#define LOW_EXPORT __declspec(dllimport)
#endif
#endif

#ifndef LOWMATH_NO_EXPORT
#define LOWMATH_NO_EXPORT
#endif
#endif

#ifndef LOWMATH_DEPRECATED
#define LOWMATH_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWMATH_DEPRECATED_EXPORT
#define LOWMATH_DEPRECATED_EXPORT LOW_EXPORT LOWMATH_DEPRECATED
#endif

#ifndef LOWMATH_DEPRECATED_NO_EXPORT
#define LOWMATH_DEPRECATED_NO_EXPORT LOWMATH_NO_EXPORT LOWMATH_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWMATH_NO_DEPRECATED
#define LOWMATH_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
