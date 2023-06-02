#ifndef MISTEDA_EXPORT_H
#define MISTEDA_EXPORT_H

#ifdef lowcore_BUILT_AS_STATIC
#define MISTEDA_API
#define MISTEDA__NO_EXPORT
#else
#ifndef MISTEDA_API
#ifdef mistedaplugin_EXPORTS
/* We are building this library */
#define MISTEDA_API __declspec(dllexport)
#else
/* We are using this library */
#define MISTEDA_API __declspec(dllimport)
#endif
#endif

#ifndef MISTEDA_NO_EXPORT
#define MISTEDA_NO_EXPORT
#endif
#endif

#ifndef MISTEDA_DEPRECATED
#define MISTEDA_DEPRECATED __declspec(deprecated)
#endif

#ifndef MISTEDA_DEPRECATED_EXPORT
#define MISTEDA_DEPRECATED_EXPORT MISTEDA_API MISTEDA_DEPRECATED
#endif

#ifndef LOWCORE_DEPRECATED_NO_EXPORT
#define MISTEDA_DEPRECATED_NO_EXPORT MISTEDA_NO_EXPORT MISTEDA_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef MISTEDA_NO_DEPRECATED
#define MISTEDA_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
