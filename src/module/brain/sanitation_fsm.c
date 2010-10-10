#include "utils/assert.h"

#include "fsm/fsm.h"
#include "brain/sanitation_fsm.h"


/// Sanitation FSM events.
enum sanitation_event_t {
  SANITATION_EVENT_JUST_EATEN,
  SANITATION_EVENT_GO_TO_BATHROOM,
  SANITATION_EVENT_SELF_CLEANUP,
  SANITATION_EVENT_SWEEP,
  SANITATION_EVENT_DISINFECTION,
  __SANITATION_EVENT_LAST
};


#define SANITATION_EVENTS_COUNT __SANITATION_EVENT_LAST


/// Ensures that event is valid.
#define ASSERT_VALID_EVENT(event) \
  ASSERT_IN_RANGE(event, 0, SANITATION_EVENTS_COUNT - 1)


static inline const char *
sanitation_event_to_str(enum sanitation_event_t event)
{
  static const char *strs[] = {
    "SANITATION_EVENT_JUST_EATEN",
    "SANITATION_EVENT_BATHROOM",
    "SANITATION_EVENT_SELF_CLEANUP",
    "SANITATION_EVENT_SWEEP",
    "SANITATION_EVENT_DISINFECTION",
  };

  ASSERT_VALID_EVENT(event);

  return strs[event];
}


/// Sanitation FSM states.
enum sanitation_state_t {
  SANITATION_STATE_NORMAL,
  SANITATION_STATE_DIRTY,
  SANITATION_STATE_INSANITARY,
  SANITATION_STATE_PLAGUE,
  __SANITATION_STATE_LAST
};


#define SANITATION_STATES_COUNT __SANITATION_STATE_LAST


/// Ensures that state is valid.
#define ASSERT_VALID_STATE(state) \
  ASSERT_IN_RANGE(state, 0, SANITATION_STATES_COUNT - 1);


static inline const char *
sanitation_state_to_str(enum sanitation_state_t state)
{
  static const char *strs[] = {
    "SANITATION_STATE_NORMAL",
    "SANITATION_STATE_DIRTY",
    "SANITATION_STATE_INSANITARY",
    "SANITATION_STATE_PLAGUE",
  };

  ASSERT_VALID_STATE(state);

  return strs[state];
}


/// Sanitation FSM type.
struct sanitation_fsm_t {
  struct fsm_t fsm;
};


/// Sanitation FSM.
static struct sanitation_fsm_t sanitation_fsm;


/// Handles #SANITATION_EVENT_JUST_EATEN event.
static int
sanitation_fsm_just_eaten_handler(enum sanitation_state_t state,
                                  struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_GO_TO_BATHROOM event.
static int
sanitation_fsm_go_to_bathroom_handler(enum sanitation_state_t state,
                                      struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_SELF_CLEANUP event.
static int
sanitation_fsm_self_cleanup_handler(enum sanitation_state_t state,
                                    struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_SEEP event.
static int
sanitation_fsm_sweep_handler(enum sanitation_state_t state,
                             struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_DISINFECTION event.
static int
sanitation_fsm_disinfection_handler(enum sanitation_state_t state,
                                    struct sanitation_fsm_t *sanitation_fsm);


struct fsm_event_handler_t sanitation_fsm_handlers[SANITATION_EVENTS_COUNT] = {
  EVENT_NO_DATA (SANITATION_EVENT_JUST_EATEN,
                 sanitation_fsm_just_eaten_handler),
  EVENT_NO_DATA (SANITATION_EVENT_GO_TO_BATHROOM,
                 sanitation_fsm_go_to_bathroom_handler),
  EVENT_NO_DATA (SANITATION_EVENT_SELF_CLEANUP,
                 sanitation_fsm_self_cleanup_handler),
  EVENT_NO_DATA (SANITATION_EVENT_SWEEP,
                 sanitation_fsm_sweep_handler),
  EVENT_NO_DATA (SANITATION_EVENT_DISINFECTION,
                 sanitation_fsm_disinfection_handler),
};


int
sanitation_fsm_init(void)
{
}


void
sanitation_fsm_cleanup(void)
{
}
