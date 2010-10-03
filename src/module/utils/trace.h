/**
 * @file   trace.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sat Sep 11 18:29:16 2010
 *
 * @brief  Tracing utility macros.
 *
 *
 */

#ifndef _TRACE_H_
#define _TRACE_H_


#include <linux/kernel.h>


#include "config.h"


static inline const char *
strip_prefix(const char *str, const char *prefix)
{
    const char *ps = str;
    const char *pp = prefix;

    while (*ps == *pp && *ps != '\0' && *pp != '\0') {
        ++ps;
        ++pp;
    }

    return ps;
}


#define __RELATIVE_FILE__ (strip_prefix(__FILE__, CONFIG_BUILD_ROOT))


/**
 * Common trace utility macro.
 *
 * @param level  loglevel
 * @param format printf-format
 *
 */
#define TRACE(level, format, ...)                             \
  printk(level "%s:%s:%d | " format "\n",                     \
         __RELATIVE_FILE__, __func__, __LINE__, ##__VA_ARGS__)


/**
 * Traces a message with KERN_EMERG loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_EMERG(format, ...) TRACE(KERN_EMERG, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_ALERT loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_ALERT(format, ...) TRACE(KERN_ALERT, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_CRIT loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_CRIT(format, ...) TRACE(KERN_CRIT, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_ERR loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_ERR(format, ...) TRACE(KERN_ERR, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_WARNING loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_WARNING(format, ...) TRACE(KERN_WARNING, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_NOTICE loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_NOTICE(format, ...) TRACE(KERN_NOTICE, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_INFO loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_INFO(format, ...) TRACE(KERN_INFO, format, ##__VA_ARGS__)


/**
 * Traces a message with KERN_DEBUG loglevel
 *
 * @param format printf-format
 *
 */
#define TRACE_DEBUG(format, ...) TRACE(KERN_DEBUG, format, ##__VA_ARGS__)


#endif /* _TRACE_H_ */
