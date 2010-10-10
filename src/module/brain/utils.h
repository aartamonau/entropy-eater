/**
 * @file   utils.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 10 10:25:29 2010
 *
 * @brief  Utility functions shared between all brain's FSMs.
 *
 *
 */

#ifndef _BRAIN__UTILS_H_
#define _BRAIN__UTILS_H_

#include <linux/kernel.h>


/**
 * Prints message from entropy eater's brain.
 *
 * @param format printf-like format string
 */
#define brain_msg(format, ...) \
  printk(KERN_ALERT "Entropy eater: " format "\n", ##__VA_ARGS__)


#endif /* _BRAIN__UTILS_H_ */
