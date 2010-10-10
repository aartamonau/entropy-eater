/**
 * @file   params.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Tue Sep 21 21:06:58 2010
 *
 * @brief  All the parameters adjusting entropy eaters behavior gathered in
 * one place.
 *
 *
 */


#ifndef _PARAMS_H_
#define _PARAMS_H_


#include "utils/assert.h"
#include "utils/random.h"


static inline int __deviate_value(int value, unsigned int deviation)
{
  int range;
  int randint;
  int sign;

  ASSERT( deviation <= 100 );

  range = value * deviation / 100;

  randint = get_random_int() % range;
  sign    = get_random_int() & 0x1 ? 1 : -1;

  return value + sign * (randint % range);
}


/// In which bounds (in per cents) to randomize all the time parameters
/// specified here.
#define EATER_TIME_DEVIATION 10


/// Periods between eaters' meals.
#define __EATER_FEEDING_TIME_PERIOD (30 * 60)
#define EATER_FEEDING_TIME_PERIOD \
  __deviate_value(__EATER_FEEDING_TIME_PERIOD, EATER_TIME_DEVIATION)


/// Bounds deviation of entropy quantity that should be eaten when eater's
/// hungry.
#define EATER_ENTROPY_DEVIATION 10


/// How much entropy should be consumed after eater got hungry.
#define __EATER_HUNGER_ENTROPY_REQUIRED 1024
#define EATER_HUNGER_ENTROPY_REQUIRED \
  __deviate_value(__EATER_HUNGER_ENTROPY_REQUIRED, EATER_ENTROPY_DEVIATION)


/// Critically low entropy balance level that causes eater's death.
#define EATER_ENTROPY_BALANCE_CRITICALLY_LOW -10000


/// Critically high entropy balance level that causes eater's death.
#define EATER_ENTROPY_BALANCE_CRITICALLY_HIGH 10000


#endif /* _PARAMS_H_ */
