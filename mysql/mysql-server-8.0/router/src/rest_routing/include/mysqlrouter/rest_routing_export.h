
#ifndef REST_ROUTING_EXPORT_H
#define REST_ROUTING_EXPORT_H

#ifdef REST_ROUTING_STATIC_DEFINE
#  define REST_ROUTING_EXPORT
#  define REST_ROUTING_NO_EXPORT
#else
#  ifndef REST_ROUTING_EXPORT
#    ifdef rest_routing_EXPORTS
        /* We are building this library */
#      define REST_ROUTING_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define REST_ROUTING_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef REST_ROUTING_NO_EXPORT
#    define REST_ROUTING_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef REST_ROUTING_DEPRECATED
#  define REST_ROUTING_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef REST_ROUTING_DEPRECATED_EXPORT
#  define REST_ROUTING_DEPRECATED_EXPORT REST_ROUTING_EXPORT REST_ROUTING_DEPRECATED
#endif

#ifndef REST_ROUTING_DEPRECATED_NO_EXPORT
#  define REST_ROUTING_DEPRECATED_NO_EXPORT REST_ROUTING_NO_EXPORT REST_ROUTING_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef REST_ROUTING_NO_DEPRECATED
#    define REST_ROUTING_NO_DEPRECATED
#  endif
#endif

#endif
