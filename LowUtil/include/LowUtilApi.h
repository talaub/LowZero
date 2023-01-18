#ifndef LOW_EXPORT_H
#define LOW_EXPORT_H

#ifdef lowutil_BUILT_AS_STATIC
#  define LOW_EXPORT
#  define LOWUTIL_NO_EXPORT
#else
#  ifndef LOW_EXPORT
#    ifdef lowutil_EXPORTS
        /* We are building this library */
#      define LOW_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define LOW_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef LOWUTIL_NO_EXPORT
#    define LOWUTIL_NO_EXPORT 
#  endif
#endif

#ifndef LOWUTIL_DEPRECATED
#  define LOWUTIL_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOWUTIL_DEPRECATED_EXPORT
#  define LOWUTIL_DEPRECATED_EXPORT LOW_EXPORT LOWUTIL_DEPRECATED
#endif

#ifndef LOWUTIL_DEPRECATED_NO_EXPORT
#  define LOWUTIL_DEPRECATED_NO_EXPORT LOWUTIL_NO_EXPORT LOWUTIL_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOWUTIL_NO_DEPRECATED
#    define LOWUTIL_NO_DEPRECATED
#  endif
#endif

#endif /* LOW_EXPORT_H */
