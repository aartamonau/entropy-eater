/**
 * @file   utils.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Tue Sep 28 22:02:41 2010
 *
 * @brief  Various utility functions.
 *
 *
 */

#ifndef _UTILS_H_
#define _UTILS_H_


/**
 * Returns random u8 value.
 *
 * @return random u8 value
 */
u8
get_random_u8(void);


/**
 * Returns random u16 value.
 *
 * @return random u16 value
 */
u16
get_random_u16(void);


/**
 * Returns random u32 value.
 *
 * @return random u32 value
 */
u32
get_random_u32(void);


/**
 * Returns random boolean value.
 *
 * @return random boolean value
 */
static inline bool
get_random_bool(void)
{
  return get_random_u8() & 0x1 ? true : false;
}


/**
 * Returns random integer value.
 *
 * @return random integer value
 */
unsigned int
get_random_int(void);


#endif /* _UTILS_H_ */
