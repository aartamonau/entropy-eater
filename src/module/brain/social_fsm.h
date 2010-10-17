/**
 * @file   social_fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 17 21:18:54 2010
 *
 * @brief  Social behavior FSM.
 *
 *
 */

#ifndef _SOCIAL_FSM_H_
#define _SOCIAL_FSM_H_


#include "utils/rps.h"


/**
 * Initializes social FSM.
 *
 *
 * @retval  0 FSM initialized successfully
 * @retval <0 error occurred
 */
int
social_fsm_init(void);


/**
 * Cleanups the resources held by social FSM.
 *
 */
void
social_fsm_cleanup(void);


/**
 * Models playing in rock-paper-scissors game.
 *
 * @param user_sign the sign chosen by user
 */
void
social_fsm_play_rps(enum rps_sign_t user_sign);


#endif /* _SOCIAL_FSM_H_ */
