#ifndef debug_h
#define debug_h

#ifdef DEBUG
# if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define debug(...)    g_log (G_LOG_DOMAIN,         \
                               G_LOG_LEVEL_DEBUG,    \
                               __VA_ARGS__)
# elif defined (__GNUC__)
#  define debug(format...)      g_log (G_LOG_DOMAIN,         \
                                       G_LOG_LEVEL_DEBUG,    \
                                       format)
# endif
#else
# define debug
#endif

#endif
