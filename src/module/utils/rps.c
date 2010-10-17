#include "utils/assert.h"
#include "utils/rps.h"


/// Checks whether sign is valid and BUG()s if it's not.
#define ASSERT_VALID_SIGN(sign) \
  ASSERT_IN_RANGE(sign, 0, RPS_SIGNS_COUNT - 1)


/// Determines whether one sign beats another one.
static enum rps_sign_t winning_table[RPS_SIGNS_COUNT] = {
  [RPS_SIGN_ROCK]     = RPS_SIGN_SCISSORS, /* rock beats scissors */
  [RPS_SIGN_PAPER]    = RPS_SIGN_ROCK,     /* paper beats rock */
  [RPS_SIGN_SCISSORS] = RPS_SIGN_PAPER,    /* scissors beats paper */
};


enum rps_result_t
rps_get_winner(enum rps_sign_t first, enum rps_sign_t second)
{
  ASSERT_VALID_SIGN( first );
  ASSERT_VALID_SIGN( second );

  if (first == second) {
    return RPS_DRAW;
  }

  return winning_table[first] == second ? RPS_WINNER_FIRST : RPS_WINNER_SECOND;
}


const char *
rps_sign_to_str(enum rps_sign_t sign)
{
  const char *strs[] = {
    "rock",
    "paper",
    "scissors",
  };

  ASSERT_VALID_SIGN(sign);

  return strs[sign];
}
