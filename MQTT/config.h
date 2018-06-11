#ifndef CONFIG_H
/* ============================================================
 * Control compile time options.
 * ============================================================
 *
 * Compile time options have moved to config.mk.
 */


/* ============================================================
 * Compatibility defines
 * ============================================================ */
#if defined(_MSC_VER) && _MSC_VER < 1900
#  define snprintf sprintf_s
#  define EPROTO ECONNABORTED
#endif

#ifdef WIN32
#  ifndef strcasecmp
#    define strcasecmp strcmpi
#  endif
#define strtok_r strtok_s
#define strerror_r(e, b, l) strerror_s(b, l, e)
#endif


#define uthash_malloc(sz) mosquitto__malloc(sz)
#define uthash_free(ptr,sz) mosquitto__free(ptr)

#ifdef __APPLE__
#  define __DARWIN_C_SOURCE
#else
#  define _DEFAULT_SOURCE 1
#  define _POSIX_C_SOURCE 200809L
#endif

#endif
