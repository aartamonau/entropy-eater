/**
 * @file   assert.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct  3 15:48:28 2010
 *
 * @brief  Assertions
 *
 *
 */


#ifndef _ASSERT_H_
#define _ASSERT_H_


#include "utils/trace.h"


/**
 * Asserts that expression holds true. Otherwise traces error using
 * #TRACE_EMERG and calls #BUG macro.
 *
 * @param expr expression that must be true
 */
#ifndef NOASSERT

#define ASSERT(expr)                                  \
  do {                                                \
    if (!(expr)) {                                    \
      TRACE_EMERG("Assertion failed: %s.", #expr);    \
      BUG();                                          \
    }                                                 \
  } while (0)

#else

#define ASSERT(expr)

#endif


/* Utility assertions */


/**
 * Asserts that given value is in the specific range.
 *
 * @param value value to check
 * @param min   minimum valid value
 * @param max   maximum valid value
 */
#define ASSERT_IN_RANGE(value, min, max) \
  ASSERT( (value) >= (min) && (value) <= max )


#endif /* _ASSERT_H_ */
