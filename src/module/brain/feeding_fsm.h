/**
 * @file   feeding_fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sat Oct  9 21:50:17 2010
 *
 * @brief  Food related behavior FSM.
 *
 *
 */

#ifndef _BRAIN__FEEDING_FSM_H_
#define _BRAIN__FEEDING_FSM_H_


/**
 * Initializes feeding FSM.
 *
 *
 * @retval  0 FSM initialized successfully
 * @retval <0 error occurred
 */
int
feeding_fsm_init(void);


/**
 * Cleanups resources held by feeding FSM.
 *
 */
void
feeding_fsm_cleanup(void);


/**
 * Feed entropy eater.
 *
 * @param food  food
 * @param count length of the food data
 */
void
feeding_fsm_feed(u8 *food, size_t count);


#endif /* _BRAIN__FEEDING_FSM_H_ */
