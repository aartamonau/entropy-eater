#include <linux/types.h>

#include "trace.h"
#include "eater_fsm.h"


typedef int (*eater_fsm_event_handler_t)(void *);
typedef int (*eater_fsm_event_handler_no_data_t)(void);


enum eater_fsm_event_handler_type_t {
  EATER_FSM_EVENT_HANDLER_NO_DATA,
  EATER_FSM_EVENT_HANDLER_WITH_DATA
};

struct eater_fsm_event_handler_t {
  enum eater_fsm_event_handler_type_t type;

  union {
    struct {
      int (*fn)(void);
    } no_data;

    struct {
      ptrdiff_t offset;
      int (*fn)(void *);
    } with_data;
  } h;
};


#define _EVENT_HANDLER(_handler, _data_member)              \
  {                                                         \
    .with_data = {                                          \
      offsetof(union eater_fsm_event_data_t, _data_member), \
      _handler                                              \
    }                                                       \
  }                                                         \

#define _EVENT_HANDLER_NO_DATA(_handler) { .no_data = { _handler } }

#define EVENT(_type, _handler, _data_member)                   \
  [_type] = { .type = EATER_FSM_EVENT_HANDLER_WITH_DATA,       \
              .h    = _EVENT_HANDLER(_handler, _data_member) }

#define EVENT_NO_DATA(_type, _handler)                          \
  [_type] = { .type = EATER_FSM_EVENT_HANDLER_NO_DATA,          \
              .h    = _EVENT_HANDLER_NO_DATA(_handler) }


int
eater_fsm_hello_handler(void);


static const
struct eater_fsm_event_handler_t handlers[EATER_FSM_EVENT_TYPES_COUNT] = {
  EVENT_NO_DATA(EATER_FSM_EVENT_TYPE_HELLO, eater_fsm_hello_handler),
};


int
eater_fsm_event_dispatch(struct eater_fsm_event_t *event);


struct eater_fsm_t {
  enum eater_fsm_state_t state;
};


static struct eater_fsm_t fsm = { .state = EATER_FSM_STATE_IDLE };


/* Auxiliary functions and macros */

#define ASSERT_VALID_EVENT(event) \
  ASSERT( (event)->type < EATER_FSM_EVENT_TYPES_COUNT )


const char *
eater_fsm_event_type_to_str(enum eater_fsm_event_type_t type);


int
eater_fsm_emit(struct eater_fsm_event_t *event)
{
  int ret;

  ASSERT_VALID_EVENT(event);
  TRACE_DEBUG("Incoming event: %s", eater_fsm_event_type_to_str(event->type));

  ret = eater_fsm_event_dispatch(event);
  if (ret < 0) {
    TRACE_DEBUG("Event handler reports an error: %d", ret);
    return ret;
  }

  fsm.state = ret;
  return 0;
}


const char *
eater_fsm_event_type_to_str(enum eater_fsm_event_type_t type)
{
  static const char *strs[EATER_FSM_EVENT_TYPES_COUNT] = {
    "EATER_FSM_EVENT_TYPE_HELLO",
  };

  return strs[type];
}


int eater_fsm_event_dispatch(struct eater_fsm_event_t *event)
{
  const struct eater_fsm_event_handler_t *handler = &handlers[event->type];

  if (handler->type == EATER_FSM_EVENT_HANDLER_NO_DATA) {
    return handler->h.no_data.fn();
  } else {                      /**< EATER_FSM_EVENT_HANDLER_WITH_DATA */
    void *data = (char *) &event->data + handler->h.with_data.offset;

    return handler->h.with_data.fn(data);
  }
}


int
eater_fsm_hello_handler(void)
{
  printk(KERN_INFO "Hello from eater fsm\n");

  return EATER_FSM_STATE_IDLE;
}
