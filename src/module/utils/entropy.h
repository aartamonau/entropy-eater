/**
 * @file   entropy.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct  3 17:08:02 2010
 *
 * @brief Entropy estimation utility functions.
 *
 *
 */

#ifndef _ENTROPY_H_
#define _ENTROPY_H_


#include <linux/types.h>


/**
 * Estimates entropy of a data, i.e. estimates how much information (in bits)
 * each byte of data holds.
 *
 * @param data data
 * @param n    length of the data
 *
 * @return entropy estimation
 */
u8
entropy_estimate(u8 *data, size_t n);


#endif /* _ENTROPY_H_ */
