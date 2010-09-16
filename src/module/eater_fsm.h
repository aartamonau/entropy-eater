#ifndef _EATER_FSM_H_
#define _EATER_FSM_H_


enum eater_fsm_state_t {
  EATER_FSM_STATE_IDLE,
  __EATER_FSM_STATE_LAST
};

#define EATER_FSM_STATES_COUNT __EATER_FSM_STATE_LAST


enum eater_fsm_event_type_t {
  EATER_FSM_EVENT_TYPE_HELLO,
  __EATER_FSM_EVENT_TYPE_LAST
};

#define EATER_FSM_EVENT_TYPES_COUNT __EATER_FSM_EVENT_TYPE_LAST


union eater_fsm_event_data_t {
  int data;
};

struct eater_fsm_event_t {
  enum eater_fsm_event_type_t type;

  union eater_fsm_event_data_t data;
};


int
eater_fsm_emit(struct eater_fsm_event_t *event);


#endif /* _EATER_FSM_H_ */
