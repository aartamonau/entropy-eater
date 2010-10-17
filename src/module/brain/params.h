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


/// Determines how long entropy eater can live without cure when he's very ill.
#define __EATER_VERY_ILL_LIVING_PERIOD (30 * 60)
#define EATER_VERY_ILL_LIVING_PERIOD \
  __deviate_value(__EATER_VERY_ILL_LIVING_PERIOD, EATER_TIME_DEVIATION)


/// Determines how fast entropy eater moves from ill to very ill state
/// without a cure.
#define __EATER_ILL_TO_VERY_ILL_PERIOD (50 * 60)
#define EATER_ILL_TO_VERY_ILL_PERIOD \
  __deviate_value(__EATER_ILL_TO_VERY_ILL_PERIOD, EATER_TIME_DEVIATION)


/// Determines normal sanitation FSM state by bathroom count.
#define EATER_BATHROOM_COUNT_NORMAL     0


/// Determines dirty sanitation FSM state by bathroom count.
#define EATER_BATHROOM_COUNT_DIRTY      1


/// Determines insanitary sanitation FSM state by bathroom count.
#define EATER_BATHROOM_COUNT_INSANITARY 3


/// Delay between a meal and a need to go to bathroom.
#define __EATER_GO_TO_BATHROOM_DELAY (20 * 60)
#define EATER_GO_TO_BATHROOM_DELAY \
  __deviate_value(__EATER_GO_TO_BATHROOM_DELAY, EATER_TIME_DEVIATION)


/// Determines how often there will a chance for eater to become infected in
/// insanitary conditions.
#define __EATER_INFECTION_DICE_ROLL_DELAY (20 * 60)
#define EATER_INFECTION_DICE_ROLL_DELAY \
  __deviate_value(__EATER_INFECTION_DICE_ROLL_DELAY, EATER_TIME_DEVIATION)


/// Time needed for entropy eater to become less happy.
#define __EATER_SOCIAL_STATE_DEMOTION_TIME (100 * 60)
#define EATER_SOCIAL_STATE_DEMOTION_TIME \
  __deviate_value(__EATER_SOCIAL_STATE_DEMOTION_TIME, EATER_TIME_DEVIATION)


/// Number of times to play in rock-paper-scissors with eater to make it more
/// happy.
#define EATER_RPS_COUNT_SOCIAL_STATE_PROMOTE 5


#endif /* _PARAMS_H_ */
