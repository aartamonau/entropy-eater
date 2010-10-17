/**
 * @file   sanitation_fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 10 12:42:36 2010
 *
 * @brief  FSM describing sanitary state of entropy eater.
 *
 *
 */
#ifndef _BRAIN__SANITATION_FSM_H_
#define _BRAIN__SANITATION_FSM_H_


/**
 * Initializes sanitation FSM.
 *
 *
 * @return execution status
 */
int
sanitation_fsm_init(void);


/**
 * Cleanups the resources held by sanitation FSM.
 *
 */
void
sanitation_fsm_cleanup(void);


/**
 * Says to the sanitation FSM that eater has just eaten.
 *
 */
void
sanitation_fsm_just_eaten(void);


/**
 * Sweep the "room" entropy eater's living in.
 *
 *
 * @return execution status
 */
int
sanitation_fsm_sweep(void);


/**
 * Disinfect the "room".
 *
 *
 * @return execution status
 */
int
sanitation_fsm_disinfect(void);


#endif /* _BRAIN__SANITATION_FSM_H_ */
