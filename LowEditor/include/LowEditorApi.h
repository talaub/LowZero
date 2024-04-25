#ifndef LOW_EDITOR_EXPORT_H
#define LOW_EDITOR_EXPORT_H

#ifdef loweditor_BUILT_AS_STATIC
#define LOW_EDITOR_API
#define LOWEDITOR_NO_EXPORT
#else
#ifndef LOW_EDITOR_API
#ifdef loweditor_EXPORTS
/* We are building this library */
#define LOW_EDITOR_API __declspec(dllexport)
#else
/* We are using this library */
#define LOW_EDITOR_API __declspec(dllimport)
#endif
#endif

#ifndef LOWEDITOR_NO_EXPORT
#define LOWEDITOR_NO_EXPORT
#endif
#endif

#ifndef LOWEDITOR_DEPRECATED
#define LOWEDITOR_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWEDITOR_DEPRECATED_EXPORT
#define LOWEDITOR_DEPRECATED_EXPORT                                  \
  LOW_EDITOR_API LOWEDITOR_DEPRECATED
#endif

#ifndef LOWEDITOR_DEPRECATED_NO_EXPORT
#define LOWEDITOR_DEPRECATED_NO_EXPORT                               \
  LOWEDITOR_NO_EXPORT LOWEDITOR_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWEDITOR_NO_DEPRECATED
#define LOWEDITOR_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
