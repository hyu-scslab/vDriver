
#ifndef ROUTING_EXPORT_H
#define ROUTING_EXPORT_H

#ifdef ROUTING_STATIC_DEFINE
#  define ROUTING_EXPORT
#  define ROUTING_NO_EXPORT
#else
#  ifndef ROUTING_EXPORT
#    ifdef routing_EXPORTS
        /* We are building this library */
#      define ROUTING_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define ROUTING_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef ROUTING_NO_EXPORT
#    define ROUTING_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef ROUTING_DEPRECATED
#  define ROUTING_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ROUTING_DEPRECATED_EXPORT
#  define ROUTING_DEPRECATED_EXPORT ROUTING_EXPORT ROUTING_DEPRECATED
#endif

#ifndef ROUTING_DEPRECATED_NO_EXPORT
#  define ROUTING_DEPRECATED_NO_EXPORT ROUTING_NO_EXPORT ROUTING_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ROUTING_NO_DEPRECATED
#    define ROUTING_NO_DEPRECATED
#  endif
#endif

#endif
