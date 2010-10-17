#include "utils/assert.h"

#include "fsm/fsm.h"
#include "brain/params.h"
#include "brain/utils.h"
#include "brain/sanitation_fsm.h"
#include "brain/living_fsm.h"


/// Sanitation FSM events.
enum sanitation_event_t {
  SANITATION_EVENT_JUST_EATEN,
  SANITATION_EVENT_GO_TO_BATHROOM,
  SANITATION_EVENT_SWEEP,
  SANITATION_EVENT_DISINFECT,
  SANITATION_EVENT_INFECTION_DICE_ROLL,
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
    "SANITATION_EVENT_GO_TO_BATHROOM",
    "SANITATION_EVENT_SWEEP",
    "SANITATION_EVENT_DISINFECT",
    "SANITATION_EVENT_INFECTION_DICE_ROLL"
  };

  ASSERT_VALID_EVENT(event);

  return strs[event];
}


/// Sanitation FSM states.
enum sanitation_state_t {
  SANITATION_STATE_NORMAL,
  SANITATION_STATE_DIRTY,
  SANITATION_STATE_INSANITARY,
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
  };

  ASSERT_VALID_STATE(state);

  return strs[state];
}


/// Sanitation FSM type.
struct sanitation_fsm_t {
  unsigned int bathroom_count;  /**< Number of times eater went to "bathroom"
                                 * that were not swept by the user. */
  bool         infected;        /**< Infection is all around. */
  struct fsm_t fsm;
};


/// Sanitation FSM.
static struct sanitation_fsm_t sanitation_fsm;


/// Exports bathroom_count via sysfs.
static ssize_t
sanitation_fsm_bathroom_count_attr_show(
  const char *name,
  const struct sanitation_fsm_t *sanitation_fsm,
  char *buffer);


/// Exports 'infected' flag via sysfs.
static ssize_t
sanitation_fsm_infected_attr_show(const char *name,
                                  const struct sanitation_fsm_t *sanitation_fsm,
                                  char *buffer);


/// Sysfs attributes.
static struct status_attr_t sanitation_fsm_attrs[] = {
  STATUS_ATTR(bathroom_count,
              (status_attr_show_t) sanitation_fsm_bathroom_count_attr_show,
              &sanitation_fsm),
  STATUS_ATTR(infected,
              (status_attr_show_t) sanitation_fsm_infected_attr_show,
              &sanitation_fsm),
};


/// Handles #SANITATION_EVENT_JUST_EATEN event.
static int
sanitation_fsm_just_eaten_handler(enum sanitation_state_t state,
                                  struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_GO_TO_BATHROOM event.
static int
sanitation_fsm_go_to_bathroom_handler(enum sanitation_state_t state,
                                      struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_SEEP event.
static int
sanitation_fsm_sweep_handler(enum sanitation_state_t state,
                             struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_DISINFECT event.
static int
sanitation_fsm_disinfect_handler(enum sanitation_state_t state,
                                 struct sanitation_fsm_t *sanitation_fsm);


/// Handles #SANITATION_EVENT_INFECTION_DICE_ROLL event.
static int
sanitation_fsm_infection_dice_roll_handler(
  enum sanitation_state_t state,
  struct sanitation_fsm_t *sanitation_fsm);


struct fsm_event_handler_t sanitation_fsm_handlers[SANITATION_EVENTS_COUNT] = {
  EVENT_NO_DATA (
    SANITATION_EVENT_JUST_EATEN,
    (fsm_event_handler_no_data_t) sanitation_fsm_just_eaten_handler
  ),
  EVENT_NO_DATA (
    SANITATION_EVENT_GO_TO_BATHROOM,
    (fsm_event_handler_no_data_t) sanitation_fsm_go_to_bathroom_handler
  ),
  EVENT_NO_DATA (
    SANITATION_EVENT_SWEEP,
    (fsm_event_handler_no_data_t) sanitation_fsm_sweep_handler
  ),
  EVENT_NO_DATA (
    SANITATION_EVENT_DISINFECT,
    (fsm_event_handler_no_data_t) sanitation_fsm_disinfect_handler
  ),
  EVENT_NO_DATA (
    SANITATION_EVENT_INFECTION_DICE_ROLL,
    (fsm_event_handler_no_data_t) sanitation_fsm_infection_dice_roll_handler
  ),
};


/// Returns FSM state by bathroom count.
static inline enum sanitation_state_t
classify_bathroom_count(unsigned int count)
{
  if (count >= EATER_BATHROOM_COUNT_INSANITARY) {
    return SANITATION_STATE_INSANITARY;
  } else if (count >= EATER_BATHROOM_COUNT_DIRTY) {
    return SANITATION_STATE_DIRTY;
  } else {
    return SANITATION_STATE_NORMAL;
  }
}


int
sanitation_fsm_init(void)
{
  int ret;

  sanitation_fsm.bathroom_count = 0;

  ret = fsm_init(&sanitation_fsm.fsm, "sanitation_fsm",
                 SANITATION_STATES_COUNT, SANITATION_EVENTS_COUNT,
                 (fsm_state_show_fn_t) sanitation_state_to_str,
                 (fsm_event_show_fn_t) sanitation_event_to_str,
                 &sanitation_fsm, sanitation_fsm_handlers);
  if (ret != 0) {
    return ret;
  }

  ret = status_create_files(sanitation_fsm_attrs,
                            ARRAY_SIZE(sanitation_fsm_attrs));
  if (ret != 0) {
    TRACE_ERR("Failed to create sanitation FSM sysfs attributes: %d", ret);
    goto error;
  }

  return 0;

error:
  fsm_cleanup(&sanitation_fsm.fsm);
  return ret;
}


void
sanitation_fsm_cleanup(void)
{
  fsm_cleanup(&sanitation_fsm.fsm);
}


void
sanitation_fsm_just_eaten(void)
{
  int ret;

  ret = fsm_emit_simple(&sanitation_fsm.fsm, SANITATION_EVENT_JUST_EATEN);
  ASSERT( ret == 0 );
}


int
sanitation_fsm_sweep(void)
{
  return fsm_emit_simple(&sanitation_fsm.fsm, SANITATION_EVENT_SWEEP);
}


int
sanitation_fsm_disinfect(void)
{
  return fsm_emit_simple(&sanitation_fsm.fsm, SANITATION_EVENT_DISINFECT);
}


static int
sanitation_fsm_sweep_handler(enum sanitation_state_t state,
                             struct sanitation_fsm_t *sanitation_fsm)
{
  switch (state) {
  case SANITATION_STATE_NORMAL:
    brain_msg("thanks, but this is not needed now");
    break;
  case SANITATION_STATE_DIRTY:
  case SANITATION_STATE_INSANITARY:
    brain_msg("thank you; you're just in time here");
    --sanitation_fsm->bathroom_count;
    break;
  default:
    ASSERT( !"impossible happened" );
  }

  return classify_bathroom_count(sanitation_fsm->bathroom_count);
}


static int
sanitation_fsm_disinfect_handler(enum sanitation_state_t state,
                                 struct sanitation_fsm_t *sanitation_fsm)
{
  switch (state) {
  case SANITATION_STATE_INSANITARY:
    brain_msg("this will not help");
    break;
  case SANITATION_STATE_NORMAL:
  case SANITATION_STATE_DIRTY:
    if (sanitation_fsm->infected) {
      brain_msg("thank you; it was just what I needed");
      sanitation_fsm->infected = false;

      fsm_cancel_postponed_events_by_type(&sanitation_fsm->fsm,
                                          SANITATION_EVENT_INFECTION_DICE_ROLL);
    } else {
      brain_msg("it's not needed");
    }

    break;
  default:
    ASSERT( !"impossible happened" );
  }


  return state;
}


static int
sanitation_fsm_go_to_bathroom_handler(enum sanitation_state_t state,
                                      struct sanitation_fsm_t *sanitation_fsm)
{
  int ret;
  enum sanitation_state_t new_state;

  ++sanitation_fsm->bathroom_count;

  new_state = classify_bathroom_count(sanitation_fsm->bathroom_count);
  if ((state != new_state) && (new_state == SANITATION_STATE_INSANITARY)) {
    sanitation_fsm->infected = true;

    ret = fsm_postpone_event(&sanitation_fsm->fsm,
                             SANITATION_EVENT_INFECTION_DICE_ROLL,
                             EATER_INFECTION_DICE_ROLL_DELAY);
    if (ret != 0) {
      return ret;
    }
  }

  return new_state;
}


static int
sanitation_fsm_just_eaten_handler(enum sanitation_state_t state,
                                  struct sanitation_fsm_t *sanitation_fsm)
{
  int ret;

  ret = fsm_postpone_event(&sanitation_fsm->fsm,
                           SANITATION_EVENT_GO_TO_BATHROOM,
                           EATER_GO_TO_BATHROOM_DELAY);
  if (ret != 0) {
    return ret;
  }

  return state;
}


static int
sanitation_fsm_infection_dice_roll_handler(
  enum sanitation_state_t state,
  struct sanitation_fsm_t *sanitation_fsm)
{
  int  ret;
  bool fall_ill = get_random_bool();

  if (fall_ill) {
    brain_msg("I fell ill in this insanitary conditions. "
              "You should have taken care of me better.");
    living_fsm_fall_ill();
  }


  ret = fsm_postpone_event(&sanitation_fsm->fsm,
                           SANITATION_EVENT_INFECTION_DICE_ROLL,
                           EATER_INFECTION_DICE_ROLL_DELAY);
  if (ret != 0) {
    return ret;
  }

  return state;
}


static ssize_t
sanitation_fsm_bathroom_count_attr_show(
  const char *name,
  const struct sanitation_fsm_t *sanitation_fsm,
  char *buffer)
{
  unsigned int bathroom_count;

  fsm_read_lock(&sanitation_fsm->fsm);
  bathroom_count = sanitation_fsm->bathroom_count;
  fsm_read_unlock(&sanitation_fsm->fsm);

  return snprintf(buffer, PAGE_SIZE, "%d\n", bathroom_count);
}


static ssize_t
sanitation_fsm_infected_attr_show(const char *name,
                                  const struct sanitation_fsm_t *sanitation_fsm,
                                  char *buffer)
{
  bool infected;

  fsm_read_lock(&sanitation_fsm->fsm);
  infected = sanitation_fsm->infected;
  fsm_read_unlock(&sanitation_fsm->fsm);

  return snprintf(buffer, PAGE_SIZE, "%s\n",
                  infected ? "true" : "false");
}
