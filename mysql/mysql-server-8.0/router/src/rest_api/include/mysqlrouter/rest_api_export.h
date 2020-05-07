
#ifndef REST_API_EXPORT_H
#define REST_API_EXPORT_H

#ifdef REST_API_STATIC_DEFINE
#  define REST_API_EXPORT
#  define REST_API_NO_EXPORT
#else
#  ifndef REST_API_EXPORT
#    ifdef rest_api_EXPORTS
        /* We are building this library */
#      define REST_API_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define REST_API_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef REST_API_NO_EXPORT
#    define REST_API_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef REST_API_DEPRECATED
#  define REST_API_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef REST_API_DEPRECATED_EXPORT
#  define REST_API_DEPRECATED_EXPORT REST_API_EXPORT REST_API_DEPRECATED
#endif

#ifndef REST_API_DEPRECATED_NO_EXPORT
#  define REST_API_DEPRECATED_NO_EXPORT REST_API_NO_EXPORT REST_API_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef REST_API_NO_DEPRECATED
#    define REST_API_NO_DEPRECATED
#  endif
#endif

#endif
