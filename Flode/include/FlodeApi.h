#ifndef FLODE_EXPORT_H
#define FLODE_EXPORT_H

#ifdef flode_BUILT_AS_STATIC
#define FLODE_API
#define FLODE_NO_EXPORT
#else
#ifndef FLODE_API
#ifdef flode_EXPORTS
/* We are building this library */
#define FLODE_API __declspec(dllexport)
#else
/* We are using this library */
#define FLODE_API __declspec(dllimport)
#endif
#endif

#ifndef FLODE_NO_EXPORT
#define FLODE_NO_EXPORT
#endif
#endif

#ifndef FLODE_DEPRECATED
#define FLODE_DEPRECATED __declspec(deprecated)
#endif

#ifndef FLODE_DEPRECATED_EXPORT
#define FLODE_DEPRECATED_EXPORT FLODE_API FLODE_DEPRECATED
#endif

#ifndef FLODE_DEPRECATED_NO_EXPORT
#define FLODE_DEPRECATED_NO_EXPORT FLODE_NO_EXPORT FLODE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef FLODE_NO_DEPRECATED
#define FLODE_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
