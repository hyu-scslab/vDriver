
#ifndef REST_ROUTER_EXPORT_H
#define REST_ROUTER_EXPORT_H

#ifdef REST_ROUTER_STATIC_DEFINE
#  define REST_ROUTER_EXPORT
#  define REST_ROUTER_NO_EXPORT
#else
#  ifndef REST_ROUTER_EXPORT
#    ifdef rest_router_EXPORTS
        /* We are building this library */
#      define REST_ROUTER_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define REST_ROUTER_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef REST_ROUTER_NO_EXPORT
#    define REST_ROUTER_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef REST_ROUTER_DEPRECATED
#  define REST_ROUTER_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef REST_ROUTER_DEPRECATED_EXPORT
#  define REST_ROUTER_DEPRECATED_EXPORT REST_ROUTER_EXPORT REST_ROUTER_DEPRECATED
#endif

#ifndef REST_ROUTER_DEPRECATED_NO_EXPORT
#  define REST_ROUTER_DEPRECATED_NO_EXPORT REST_ROUTER_NO_EXPORT REST_ROUTER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef REST_ROUTER_NO_DEPRECATED
#    define REST_ROUTER_NO_DEPRECATED
#  endif
#endif

#endif
