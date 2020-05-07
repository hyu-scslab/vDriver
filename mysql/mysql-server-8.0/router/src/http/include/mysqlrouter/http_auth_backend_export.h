
#ifndef HTTP_AUTH_BACKEND_EXPORT_H
#define HTTP_AUTH_BACKEND_EXPORT_H

#ifdef HTTP_AUTH_BACKEND_STATIC_DEFINE
#  define HTTP_AUTH_BACKEND_EXPORT
#  define HTTP_AUTH_BACKEND_NO_EXPORT
#else
#  ifndef HTTP_AUTH_BACKEND_EXPORT
#    ifdef http_auth_backend_EXPORTS
        /* We are building this library */
#      define HTTP_AUTH_BACKEND_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define HTTP_AUTH_BACKEND_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef HTTP_AUTH_BACKEND_NO_EXPORT
#    define HTTP_AUTH_BACKEND_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef HTTP_AUTH_BACKEND_DEPRECATED
#  define HTTP_AUTH_BACKEND_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef HTTP_AUTH_BACKEND_DEPRECATED_EXPORT
#  define HTTP_AUTH_BACKEND_DEPRECATED_EXPORT HTTP_AUTH_BACKEND_EXPORT HTTP_AUTH_BACKEND_DEPRECATED
#endif

#ifndef HTTP_AUTH_BACKEND_DEPRECATED_NO_EXPORT
#  define HTTP_AUTH_BACKEND_DEPRECATED_NO_EXPORT HTTP_AUTH_BACKEND_NO_EXPORT HTTP_AUTH_BACKEND_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef HTTP_AUTH_BACKEND_NO_DEPRECATED
#    define HTTP_AUTH_BACKEND_NO_DEPRECATED
#  endif
#endif

#endif
