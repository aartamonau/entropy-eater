/**
 * @file   living_fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 10 09:54:52 2010
 *
 * @brief  FSM determining whether entropy eater is alive or dead.
 *
 *
 */
#ifndef _BRAIN__LIVING_FSM_H_
#define _BRAIN__LIVING_FSM_H_


#include <linux/compiler.h>


/**
 * Initializes living FSM.
 *
 *
 * @return execution status
 */
int
living_fsm_init(void);


/**
 * Cleanups resources held by living FSM.
 *
 */
void
living_fsm_cleanup(void);


/**
 * Makes entropy eater to die nobly (not causing kernel panic).
 *
 * @return execution status
 */
void
living_fsm_die_nobly(void);


/**
 * Kills entropy eater. Causes kernel panic.
 */
void __noreturn
living_fsm_die(void);


#endif /* _BRAIN__LIVING_FSM_H_ */
