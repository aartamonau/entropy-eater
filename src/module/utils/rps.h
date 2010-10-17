/**
 * @file   rps.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 17 21:43:29 2010
 *
 * @brief  Rock-paper-scissors game.
 *
 *
 */

#ifndef _RPS_H_
#define _RPS_H_


/// Possible signs.
enum rps_sign_t {
  RPS_SIGN_ROCK,                /**< Rock. */
  RPS_SIGN_PAPER,               /**< Paper. */
  RPS_SIGN_SCISSORS,            /**< Scissors. */
  __RPS_SIGN_LAST
};


/// Number of signs.
#define RPS_SIGNS_COUNT __RPS_SIGN_LAST


#ifdef __KERNEL__


/**
 * Transforms rps_sign_t:: value to string.
 *
 * @param sign sign
 *
 * @return string
 */
const char *
rps_sign_to_str(enum rps_sign_t sign);


/// Result of rock-paper-scissors game.
enum rps_result_t {
  RPS_WINNER_FIRST,             /**< First player won. */
  RPS_WINNER_SECOND,            /**< Second player won. */
  RPS_DRAW,                     /**< Draw. */
};


/**
 * Determines who is the winner in rock-paper-scissors game.
 *
 * @param first  a sign shown by the first participant
 * @param second a sign shown by the second participant
 *
 * @return result
 */
enum rps_result_t
rps_get_winner(enum rps_sign_t first, enum rps_sign_t second);


#endif  /* __KERNEL__ */


#endif /* _RPS_H_ */
