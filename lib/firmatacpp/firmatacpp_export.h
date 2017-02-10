
#ifndef FIRMATACPP_EXPORT_H
#define FIRMATACPP_EXPORT_H

#ifdef FIRMATACPP_STATIC_DEFINE
#  define FIRMATACPP_EXPORT
#  define FIRMATACPP_NO_EXPORT
#else
#  ifndef FIRMATACPP_EXPORT
#    ifdef firmatacpp_EXPORTS
        /* We are building this library */
#      define FIRMATACPP_EXPORT 
#    else
        /* We are using this library */
#      define FIRMATACPP_EXPORT 
#    endif
#  endif

#  ifndef FIRMATACPP_NO_EXPORT
#    define FIRMATACPP_NO_EXPORT 
#  endif
#endif

#ifndef FIRMATACPP_DEPRECATED
#  define FIRMATACPP_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef FIRMATACPP_DEPRECATED_EXPORT
#  define FIRMATACPP_DEPRECATED_EXPORT FIRMATACPP_EXPORT FIRMATACPP_DEPRECATED
#endif

#ifndef FIRMATACPP_DEPRECATED_NO_EXPORT
#  define FIRMATACPP_DEPRECATED_NO_EXPORT FIRMATACPP_NO_EXPORT FIRMATACPP_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef FIRMATACPP_NO_DEPRECATED
#    define FIRMATACPP_NO_DEPRECATED
#  endif
#endif

#endif
