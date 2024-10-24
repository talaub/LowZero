#ifndef LOW_RENDERER_EXPORT_H
#define LOW_RENDERER_EXPORT_H

#ifdef lowrderer_BUILT_AS_STATIC
#define LOW_RENDERER_API
#define LOWRENDERER_NO_EXPORT
#else
#ifndef LOW_RENDERER_API
#ifdef lowrenderer_EXPORTS
/* We are building this library */
#define LOW_RENDERER_API __declspec(dllexport)
#else
/* We are using this library */
#define LOW_RENDERER_API __declspec(dllimport)
#endif
#endif

#ifndef LOWRENDERER_NO_EXPORT
#define LOWRENDERER_NO_EXPORT
#endif
#endif

#ifndef LOWRENDERER_DEPRECATED
#define LOWRENDERER_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWRENDERER_DEPRECATED_EXPORT
#define LOWRENDERER_DEPRECATED_EXPORT LOW_RENDERER_API LOWRENDERER_DEPRECATED
#endif

#ifndef LOWRENDERER_DEPRECATED_NO_EXPORT
#define LOWRENDERER_DEPRECATED_NO_EXPORT                                       \
  LOWRENDERER_NO_EXPORT LOWRENDERER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWRENDERER_NO_DEPRECATED
#define LOWRENDERER_NO_DEPRECATED
#endif
#endif

#endif /* LOW_EXPORT_H */
