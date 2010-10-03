/**
 * @file   eater_fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Tue Sep 21 21:05:53 2010
 *
 * @brief  Finite state machine describing creature's behavior.
 *
 *
 */
#ifndef _EATER_FSM_H_
#define _EATER_FSM_H_


/// Event types that eater's FSM can handle.
enum eater_fsm_event_type_t {
  EATER_FSM_EVENT_TYPE_INIT,
  EATER_FSM_EVENT_TYPE_DIE_NOBLY,
  EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT,
  EATER_FSM_EVENT_TYPE_FEED,
  __EATER_FSM_EVENT_TYPE_LAST
};


/// Number of event types.
#define EATER_FSM_EVENT_TYPES_COUNT __EATER_FSM_EVENT_TYPE_LAST



/// Data for #EATER_FSM_EVENT_TYPE_FEED event.
struct eater_fsm_feed_event_data_t {
  size_t count;                 /**< Length of the food data. */
  u8    *food;                  /**< Food. */
};


/// Event specific data encapsulated in a single union.
union eater_fsm_event_data_t {
  struct eater_fsm_feed_event_data_t feed_data;
};


/// Event.
struct eater_fsm_event_t {
  enum eater_fsm_event_type_t  type; /**< Event type. */
  union eater_fsm_event_data_t data; /**< Event data. */
};


/**
 * Initializes entropy eater's FSM.
 *
 * @return operation status
 * @retval  0 success
 * @retval <0 error code
 */
int
eater_fsm_init(void);


/**
 * Feeds event to FSM.
 *
 * @param event event
 *
 * @retval  0 success
 * @retval <0 error code
 */
int
eater_fsm_emit(struct eater_fsm_event_t *event);


/**
 * Simplified version of eater_fsm_emit() function. This function does not
 * take a pointer to event structure (where simplification is). Instead just
 * event type if taken. As a drawback only events that do not take data
 * parameter can be emitted.
 *
 * @param event event to emit.
 *
 * @retval  0 success
 * @retval <0 error code
 */
int
eater_fsm_emit_simple(enum eater_fsm_event_type_t event);


#endif /* _EATER_FSM_H_ */
