#ifndef _EATER_FSM_H_
#define _EATER_FSM_H_


/// Eater's FSM states.
enum eater_fsm_state_t {
  EATER_FSM_STATE_IDLE,
  __EATER_FSM_STATE_LAST
};


/// Number of states in eater's FSM.
#define EATER_FSM_STATES_COUNT __EATER_FSM_STATE_LAST


/// Event types that eater's FSM can handle.
enum eater_fsm_event_type_t {
  EATER_FSM_EVENT_TYPE_HELLO,
  __EATER_FSM_EVENT_TYPE_LAST
};


/// Number of event types.
#define EATER_FSM_EVENT_TYPES_COUNT __EATER_FSM_EVENT_TYPE_LAST


/// Event specific data encapsulated in a single union.
union eater_fsm_event_data_t {
  int data;
};


/// Event.
struct eater_fsm_event_t {
  enum eater_fsm_event_type_t  type; /**< Event type. */
  union eater_fsm_event_data_t data; /**< Event data. */
};


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


#endif /* _EATER_FSM_H_ */
