#include "fsm/fsm.h"

#include "utils/assert.h"
#include "utils/random.h"

#include "brain/params.h"
#include "brain/utils.h"
#include "brain/social_fsm.h"
#include "brain/living_fsm.h"


/// Social FSM events.
enum social_event_t {
  SOCIAL_EVENT_REVISE_STATE,    /**< Emitted when it's time to become less
                                 * happy. */
  SOCIAL_EVENT_PLAY_RPS,        /**< Emitted when user wants to play in
                                 * rock-paper-scissors. */
  __SOCIAL_EVENT_LAST
};


#define SOCIAL_EVENTS_COUNT __SOCIAL_EVENT_LAST


/// Checks that event is valid.
#define ASSERT_VALID_EVENT(event) \
  ASSERT_IN_RANGE(event, 0, SOCIAL_EVENTS_COUNT - 1)


static inline const char *
social_event_to_str(enum social_event_t event)
{
  static const char *strs[] = {
    "SOCIAL_EVENT_REVISE_STATE",
    "SOCIAL_EVENT_PLAY_RPS",
  };

  ASSERT_VALID_EVENT(event);

  return strs[event];
}


/// Social FSM states.
enum social_state_t {
  SOCIAL_STATE_NORMAL,
  SOCIAL_STATE_HAPPY,
  SOCIAL_STATE_DEPRESSED,
  __SOCIAL_STATE_LAST
};


#define SOCIAL_STATES_COUNT __SOCIAL_STATE_LAST


/// Checks that state is valid.
#define ASSERT_VALID_STATE(state) \
  ASSERT_IN_RANGE(state, 0, SOCIAL_STATES_COUNT - 1)


static inline const char *
social_state_to_str(enum social_state_t state)
{
  static const char *strs[] = {
    "SOCIAL_STATE_NORMAL",
    "SOCIAL_STATE_HAPPY",
    "SOCIAL_STATE_DEPRESSED",
  };

  ASSERT_VALID_STATE(state);

  return strs[state];
}


struct social_fsm_t {
  int rps_count;

  struct fsm_t fsm;
};


/// Social FSM.
static struct social_fsm_t social_fsm;


/// Exports rps_count via sysfs.
static ssize_t
social_fsm_rps_count_attr_show(const char *name,
                               const struct social_fsm_t *social_fsm,
                               char *buffer);


/// Sysfs attributes.
static struct status_attr_t social_fsm_attrs[] = {
  STATUS_ATTR(rps_count,
              (status_attr_show_t) social_fsm_rps_count_attr_show,
              &social_fsm),
};


/// Handles #SOCIAL_EVENT_REVISE_STATE event.
static int
social_fsm_revise_state_handler(enum social_state_t state,
                                struct social_fsm_t *social_fsm);


/// Data for #SOCIAL_EVENT_PLAY_RPS event.
struct social_event_play_rps_data_t {
  enum rps_sign_t user_sign;         /**< User's sign. */
};


/// Handles #SOCIAL_EVENT_PLAY_RPS event.
static int
social_fsm_play_rps_handler(enum social_state_t state,
                            struct social_fsm_t *social_fsm,
                            struct social_event_play_rps_data_t *play_rps_data);


struct fsm_event_handler_t social_fsm_handlers[SOCIAL_EVENTS_COUNT] = {
  EVENT_NO_DATA (
    SOCIAL_EVENT_REVISE_STATE,
    (fsm_event_handler_no_data_t) social_fsm_revise_state_handler
  ),
  EVENT         (
    SOCIAL_EVENT_PLAY_RPS,
    (fsm_event_handler_t)         social_fsm_play_rps_handler
  ),
};


/**
 * Performs actual play in RPS.
 *
 * @param user_sign user's choice
 */
static void
social_fsm_do_play_rps(enum rps_sign_t user_sign);


int
social_fsm_init(void)
{
  int ret;

  social_fsm.rps_count = 0;

  ret = fsm_init(&social_fsm.fsm, "social_fsm",
                 SOCIAL_STATES_COUNT, SOCIAL_EVENTS_COUNT,
                 (fsm_state_show_fn_t) social_state_to_str,
                 (fsm_event_show_fn_t) social_event_to_str,
                 &social_fsm, social_fsm_handlers);
  if (ret != 0) {
    return ret;
  }

  ret = status_create_files(social_fsm_attrs, ARRAY_SIZE(social_fsm_attrs));
  if (ret != 0) {
    TRACE_ERR("Failed to create social FSM sysfs attributes: %d", ret);
    goto error;
  }

  return 0;

error:
  fsm_cleanup(&social_fsm.fsm);
  return ret;
}


void
social_fsm_cleanup(void)
{
  status_remove_files(social_fsm_attrs, ARRAY_SIZE(social_fsm_attrs));
  fsm_cleanup(&social_fsm.fsm);
}


void
social_fsm_play_rps(enum rps_sign_t user_sign)
{
  int ret;

  struct social_event_play_rps_data_t data = {
    .user_sign = user_sign,
  };

  ret = fsm_emit(&social_fsm.fsm, SOCIAL_EVENT_PLAY_RPS, &data);
  ASSERT( ret == 0 );
}


static int
social_fsm_revise_state_handler(enum social_state_t state,
                                struct social_fsm_t *social_fsm)
{
  int ret;
  enum social_state_t new_state;

  switch (state) {
  case SOCIAL_STATE_HAPPY:
    new_state = SOCIAL_STATE_NORMAL;
    break;
  case SOCIAL_STATE_NORMAL:
    new_state = SOCIAL_STATE_DEPRESSED;
    break;
  case SOCIAL_STATE_DEPRESSED:
    brain_msg("Depression killed me.");
    living_fsm_die();
    break;
  default:
    ASSERT( !"impossible happened" );
  }


  ret = fsm_postpone_event(&social_fsm->fsm,
                           SOCIAL_EVENT_REVISE_STATE,
                           EATER_SOCIAL_STATE_DEMOTION_TIME);
  if (ret != 0) {
    return ret;
  }

  return new_state;
}


static int
social_fsm_play_rps_handler(enum social_state_t state,
                            struct social_fsm_t *social_fsm,
                            struct social_event_play_rps_data_t *play_rps_data)
{
  int ret;
  enum social_state_t new_state;

  social_fsm_do_play_rps(play_rps_data->user_sign);

  fsm_cancel_postponed_events_by_type(&social_fsm->fsm,
                                      SOCIAL_EVENT_REVISE_STATE);
  ret = fsm_postpone_event(&social_fsm->fsm,
                           SOCIAL_EVENT_REVISE_STATE,
                           EATER_SOCIAL_STATE_DEMOTION_TIME);
  if (ret != 0) {
    return ret;
  }

  social_fsm->rps_count += 1;
  if (social_fsm->rps_count == EATER_RPS_COUNT_SOCIAL_STATE_PROMOTE)
  {
    switch (state) {
    case SOCIAL_STATE_HAPPY:
      new_state = SOCIAL_STATE_HAPPY;
      social_fsm->rps_count = EATER_RPS_COUNT_SOCIAL_STATE_PROMOTE;
      break;
    case SOCIAL_STATE_NORMAL:
      new_state = SOCIAL_STATE_HAPPY;
      social_fsm->rps_count = 0;
      brain_msg("Life is a miracle");
      break;
    case SOCIAL_STATE_DEPRESSED:
      new_state = SOCIAL_STATE_NORMAL;
      social_fsm->rps_count = 0;
      brain_msg("I'm much better now");
      break;
    default:
      ASSERT( !"impossible happened" );
    }
  } else {
    new_state = state;
  }

  return new_state;
}


static ssize_t
social_fsm_rps_count_attr_show(const char *name,
                               const struct social_fsm_t *social_fsm,
                               char *buffer)
{
  int rps_count;

  fsm_read_lock(&social_fsm->fsm);
  rps_count = social_fsm->rps_count;
  fsm_read_unlock(&social_fsm->fsm);

  return snprintf(buffer, PAGE_SIZE, "%d\n", rps_count);
}


static void
social_fsm_do_play_rps(enum rps_sign_t user_sign)
{
  enum rps_sign_t   eater_sign = get_random_u8() % RPS_SIGNS_COUNT;

  brain_msg("your choice: %s", rps_sign_to_str(user_sign));
  brain_msg("my choice:   %s", rps_sign_to_str(eater_sign));

  switch (rps_get_winner(user_sign, eater_sign)) {
  case RPS_WINNER_FIRST:
    brain_msg("you won");
    break;
  case RPS_WINNER_SECOND:
    brain_msg("I won");
    break;
  case RPS_DRAW:
    brain_msg("draw");
    break;
  default:
    ASSERT( !"impossible happened" );
  }
}
