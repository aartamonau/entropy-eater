#include "utils/assert.h"
#include "utils/random.h"

#include "fsm/fsm.h"

#include "brain/params.h"
#include "brain/living_fsm.h"
#include "brain/utils.h"


/// Living FSM events
enum living_event_t {
  LIVING_EVENT_DIE,
  LIVING_EVENT_DIE_NOBLY,
  LIVING_EVENT_FALL_ILL,
  LIVING_EVENT_REVISE_ILLNESS,
  LIVING_EVENT_CURE_ILLNESS,
  __LIVING_EVENT_LAST
};


#define LIVING_EVENTS_COUNT __LIVING_EVENT_LAST


/// Ensures that event is valid one.
#define ASSERT_VALID_EVENT(event) \
  ASSERT_IN_RANGE(event, 0, LIVING_EVENTS_COUNT - 1)


static inline const char *
living_event_to_str(enum living_event_t event)
{
  static const char *strs[] = {
    "LIVING_EVENT_DIE",
    "LIVING_EVENT_DIE_NOBLY",
    "LIVING_EVENT_FALL_ILL",
    "LIVING_EVENT_REVISE_ILLNESS",
    "LIVING_EVENT_CURE_ILLNESS",
  };

  ASSERT_VALID_EVENT(event);

  return strs[event];
}


/// Living FSM states.
enum living_state_t {
  LIVING_STATE_ALIVE,
  LIVING_STATE_ILL,
  LIVING_STATE_VERY_ILL,
  LIVING_STATE_DEAD,
  __LIVING_STATE_LAST
};


#define LIVING_STATES_COUNT __LIVING_STATE_LAST


/// Ensures that state is valid.
#define ASSERT_VALID_STATE(state) \
  ASSERT_IN_RANGE(state, 0, LIVING_STATES_COUNT - 1)


static inline const char *
living_state_to_str(enum living_state_t state)
{
  static const char *strs[] = {
    "LIVING_STATE_ALIVE",
    "LIVING_STATE_ILL",
    "LIVING_STATE_VERY_ILL",
    "LIVING_STATE_DEAD",
  };

  ASSERT_VALID_STATE(state);

  return strs[state];
}


/// Living FSM type.
struct living_fsm_t {
  struct fsm_t fsm;
};


/// Living FSM.
static struct living_fsm_t living_fsm;


/// Handles #LIVING_EVENT_DIE_NOBLY event.
static int
living_fsm_die_nobly_handler(enum living_state_t state,
                             struct living_fsm_t *living_fsm);


/// Handles #LIVING_EVENT_DIE event.
static int __noreturn
living_fsm_die_handler(enum living_state_t state,
                       struct living_fsm_t *living_fsm);


/// Handles #LIVING_EVENT_FALL_ILL event.
static int
living_fsm_fall_ill_handler(enum living_state_t state,
                            struct living_fsm_t *living_fsm);


/// Handles #LIVING_EVENT_REVISE_ILLNESS event.
static int
living_fsm_revise_illness_handler(enum living_state_t state,
                                  struct living_fsm_t *living_fsm);


/// Handles #LIVING_EVENT_CURE_ILLNESS event.
static int
living_fsm_cure_illness_handler(enum living_state_t state,
                                struct living_fsm_t *living_fsm);


struct fsm_event_handler_t living_fsm_handlers[LIVING_EVENTS_COUNT] = {
  EVENT_NO_DATA (
    LIVING_EVENT_DIE_NOBLY,
    (fsm_event_handler_no_data_t) living_fsm_die_nobly_handler
  ),
  EVENT_NO_DATA (
    LIVING_EVENT_DIE,
    (fsm_event_handler_no_data_t) living_fsm_die_handler
  ),
  EVENT_NO_DATA (
    LIVING_EVENT_FALL_ILL,
    (fsm_event_handler_no_data_t) living_fsm_fall_ill_handler
  ),
  EVENT_NO_DATA (
    LIVING_EVENT_REVISE_ILLNESS,
    (fsm_event_handler_no_data_t) living_fsm_revise_illness_handler
  ),
  EVENT_NO_DATA (
    LIVING_EVENT_CURE_ILLNESS,
    (fsm_event_handler_no_data_t) living_fsm_cure_illness_handler
  ),
};


int
living_fsm_init(void)
{
  return fsm_init(&living_fsm.fsm, "living_fsm",
                  LIVING_STATES_COUNT, LIVING_EVENTS_COUNT,
                  (fsm_state_show_fn_t) living_state_to_str,
                  (fsm_event_show_fn_t) living_event_to_str,
                  &living_fsm, living_fsm_handlers);
}


void
living_fsm_cleanup(void)
{
  fsm_cleanup(&living_fsm.fsm);
}


void
living_fsm_die_nobly(void)
{
  int ret;

  ret = fsm_emit_simple(&living_fsm.fsm, LIVING_EVENT_DIE_NOBLY);
  ASSERT( ret == 0 );
}


void __noreturn
living_fsm_die(void)
{
  fsm_emit_simple(&living_fsm.fsm, LIVING_EVENT_DIE);

  /* suppressing gcc warning about noreturn function that does return */
  panic("not really needed here");
}


static int
living_fsm_die_nobly_handler(enum living_state_t state,
                             struct living_fsm_t *living_fsm)
{
  brain_msg("it was a great experience, Sir!");
  return LIVING_STATE_DEAD;
}


static int __noreturn
living_fsm_die_handler(enum living_state_t state,
                       struct living_fsm_t *living_fsm)
{
  brain_msg("you've been an awful owner; I'm dying in agony.");
  panic("DEAD");
}


static int
living_fsm_fall_ill_handler(enum living_state_t state,
                            struct living_fsm_t *living_fsm)
{
  enum living_state_t new_state;

  switch (state) {
  case LIVING_STATE_ILL:
    new_state = LIVING_STATE_VERY_ILL;
    brain_msg("another illness makes me very ill");
    fsm_postpone_event(&living_fsm->fsm,
                       LIVING_EVENT_DIE, EATER_VERY_ILL_LIVING_PERIOD);
    break;
  case LIVING_STATE_VERY_ILL:
    brain_msg("I'm already very ill; another illness just kills me");
    living_fsm_die();
    break;
  default:
    new_state = LIVING_STATE_ILL;
    brain_msg("you're no the best owner possible; I got ill.");
    fsm_postpone_event(&living_fsm->fsm,
                       LIVING_EVENT_REVISE_ILLNESS,
                       EATER_ILL_TO_VERY_ILL_PERIOD);
  }

  return new_state;
}


static int
living_fsm_revise_illness_handler(enum living_state_t state,
                                  struct living_fsm_t *living_fsm)
{
  bool self_cure;

  switch (state) {
  case LIVING_STATE_ILL:
    break;
  default:
    ASSERT( !"invalid state" );
  }

  /* do we cured without any help */
  self_cure = get_random_bool();
  if (self_cure) {
    brain_msg("you're lucky; somehow I got better without your help");
    return LIVING_STATE_ALIVE;
  } else {
    brain_msg("damn you; I'm getting worse");
    fsm_postpone_event(&living_fsm->fsm,
                       LIVING_EVENT_DIE, EATER_VERY_ILL_LIVING_PERIOD);
    return LIVING_STATE_VERY_ILL;
  }
}


static int
living_fsm_cure_illness_handler(enum living_state_t state,
                                struct living_fsm_t *living_fsm)
{
  enum living_state_t new_state;

  switch (state) {
  case LIVING_STATE_ILL:
    brain_msg("thank you for your help; I'm just fine now");
    new_state = LIVING_STATE_ALIVE;
    break;
  case LIVING_STATE_VERY_ILL:
    brain_msg("finally you gave me some remedies; It feels much better now");
    new_state = LIVING_STATE_ILL;
    fsm_postpone_event(&living_fsm->fsm,
                       LIVING_EVENT_REVISE_ILLNESS,
                       EATER_ILL_TO_VERY_ILL_PERIOD);
    break;
  default:
    brain_msg("thanks for you care but I don't require this help now");
    new_state = state;
  }

  return new_state;
}


void
living_fsm_fall_ill(void)
{
  int ret;

  ret = fsm_emit_simple(&living_fsm.fsm, LIVING_EVENT_FALL_ILL);
  ASSERT( ret == 0 );
}


int
living_fsm_cure_illness(void)
{
  return fsm_emit_simple(&living_fsm.fsm, LIVING_EVENT_CURE_ILLNESS);
}
