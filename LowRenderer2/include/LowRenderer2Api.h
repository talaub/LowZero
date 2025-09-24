#ifndef LOW_RENDERER2_EXPORT_H
#define LOW_RENDERER2_EXPORT_H

#ifdef lowrderer2_BUILT_AS_STATIC
#define LOW_RENDERER_API
#define LOWRENDERER_NO_EXPORT
#else
#ifndef LOW_RENDERER2_API
#ifdef lowrenderer2_EXPORTS
/* We are building this library */
#define LOW_RENDERER2_API __declspec(dllexport)
#else
/* We are using this library */
#define LOW_RENDERER2_API __declspec(dllimport)
#endif
#endif

#ifndef LOWRENDERER2_NO_EXPORT
#define LOWRENDERER2_NO_EXPORT
#endif
#endif

#ifndef LOWRENDERER2_DEPRECATED
#define LOWRENDERER2_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWRENDERER2_DEPRECATED_EXPORT
#define LOWRENDERER2_DEPRECATED_EXPORT                               \
  LOW_RENDERER_API LOWRENDERER2_DEPRECATED
#endif

#ifndef LOWRENDERER2_DEPRECATED_NO_EXPORT
#define LOWRENDERER2_DEPRECATED_NO_EXPORT                            \
  LOWRENDERER2_NO_EXPORT LOWRENDERER2_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef LOWRENDERER_NO_DEPRECATED
#define LOWRENDERER_NO_DEPRECATED
#endif
#endif

#define LOW_RENDERER_API LOW_RENDERER2_API

#endif /* LOW_EXPORT_H */
