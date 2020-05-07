
#ifndef HARNESS_EXPORT_H
#define HARNESS_EXPORT_H

#ifdef HARNESS_STATIC_DEFINE
#  define HARNESS_EXPORT
#  define HARNESS_NO_EXPORT
#else
#  ifndef HARNESS_EXPORT
#    ifdef harness_library_EXPORTS
        /* We are building this library */
#      define HARNESS_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define HARNESS_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef HARNESS_NO_EXPORT
#    define HARNESS_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef HARNESS_DEPRECATED
#  define HARNESS_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef HARNESS_DEPRECATED_EXPORT
#  define HARNESS_DEPRECATED_EXPORT HARNESS_EXPORT HARNESS_DEPRECATED
#endif

#ifndef HARNESS_DEPRECATED_NO_EXPORT
#  define HARNESS_DEPRECATED_NO_EXPORT HARNESS_NO_EXPORT HARNESS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef HARNESS_NO_DEPRECATED
#    define HARNESS_NO_DEPRECATED
#  endif
#endif

#endif
