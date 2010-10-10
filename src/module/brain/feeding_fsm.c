#include "fsm/fsm.h"

#include "utils/assert.h"
#include "utils/entropy.h"

#include "brain/utils.h"
#include "brain/params.h"
#include "brain/feeding_fsm.h"
#include "brain/living_fsm.h"


/// Feeding FSM events.
enum feeding_event_t {
  FEEDING_EVENT_INIT,
  FEEDING_EVENT_FEED,
  FEEDING_EVENT_FEEDING_TIME,
  __FEEDING_EVENT_LAST
};


#define FEEDING_EVENTS_COUNT __FEEDING_EVENT_LAST


/// Checks that event is valid.
#define ASSERT_VALID_EVENT(event) \
  ASSERT_IN_RANGE(event, 0, FEEDING_EVENTS_COUNT - 1)


static inline const char *
feeding_event_to_str(enum feeding_event_t event)
{
  static const char *strs[] = {
    "FEEDING_EVENT_INIT",
    "FEEDING_EVENT_FEED",
    "FEEDING_EVENT_FEEDING_TIME",
  };

  ASSERT_VALID_EVENT(event);

  return strs[event];
}


/// Feeding FSM states.
enum feeding_state_t {
  FEEDING_STATE_NORMAL,
  FEEDING_STATE_HUNGRY,
  FEEDING_STATE_OVEREATEN,
  __FEEDING_STATE_LAST
};


#define FEEDING_STATES_COUNT __FEEDING_STATE_LAST


/// Checks that state is valid.
#define ASSERT_VALID_STATE(state) \
  ASSERT_IN_RANGE(state, 0, FEEDING_STATES_COUNT - 1)


static inline const char *
feeding_state_to_str(enum feeding_state_t state)
{
  static const char *strs[] = {
    "FEEDING_STATE_NORMAL",
    "FEEDING_STATE_HUNGRY",
    "FEEDING_STATE_OVEREATEN",
  };

  ASSERT_VALID_STATE(state);

  return strs[state];
}


/// Feeding FSM type.
struct feeding_fsm_t {
  int entropy_balance;           /**< Consumed entropy balance. Should be
                                  * close to zero. */

  struct fsm_t fsm;
};


/// Feeding FSM.
static struct feeding_fsm_t feeding_fsm;


/// Exports entropy_balance via sysfs.
static ssize_t
feeding_fsm_entropy_balance_attr_show(const char *name,
                                      const struct feeding_fsm_t *feeding_fsm,
                                      char *buffer);


/// Sysfs attributes.
static struct status_attr_t feeding_fsm_attrs[] = {
  STATUS_ATTR(entropy_balance,
              (status_attr_show_t) feeding_fsm_entropy_balance_attr_show,
              &feeding_fsm),
};


/**
 * Returns feeding FSM state by current entropy balance.
 *
 * @param entropy_balance entropy balance
 *
 * @return feeding state
 */
static enum feeding_state_t
classify_entropy_balance(int entropy_balance);


/// Handles #FEEDING_EVENT_INIT event.
static int
feeding_fsm_init_handler(enum feeding_state_t state,
                         struct feeding_fsm_t *feeding_fsm);


/// Handles #FEEDING_EVENT_FEEDING_TIME event.
static int
feeding_fsm_feeding_time_handler(enum feeding_state_t state,
                                 struct feeding_fsm_t *feeding_fsm);


/// Data for #FEEDING_EVENT_FEED event.
struct feeding_event_feed_data_t {
  size_t count;                 /**< Length of the food data. */
  u8    *food;                  /**< Food. */
};


/// Handles #FEEDING_EVENT_FEED event.
static int
feeding_fsm_feed_handler(enum feeding_state_t state,
                         struct feeding_fsm_t *feeding_fsm,
                         struct feeding_event_feed_data_t *feed_data);


/// Feeding FSM event handlers.
struct fsm_event_handler_t feeding_fsm_handlers[FEEDING_EVENTS_COUNT] = {
  EVENT_NO_DATA (FEEDING_EVENT_INIT,         feeding_fsm_init_handler),
  EVENT_NO_DATA (FEEDING_EVENT_FEEDING_TIME, feeding_fsm_feeding_time_handler),
  EVENT         (FEEDING_EVENT_FEED,         feeding_fsm_feed_handler)
};


int
feeding_fsm_init(void)
{
  int ret;

  ret = fsm_init(&feeding_fsm.fsm, "feeding_fsm",
                 FEEDING_STATES_COUNT, FEEDING_EVENTS_COUNT,
                 (fsm_state_show_fn_t) feeding_state_to_str,
                 (fsm_event_show_fn_t) feeding_event_to_str,
                 &feeding_fsm, feeding_fsm_handlers);

  if (ret != 0) {
    return ret;
  }

  ret = fsm_emit_simple(&feeding_fsm.fsm, FEEDING_EVENT_INIT);
  if (ret != 0) {
    goto error;
  }

  ret = status_create_files(feeding_fsm_attrs, ARRAY_SIZE(feeding_fsm_attrs));
  if (ret != 0) {
    TRACE_ERR("Failed to create feeding FSM sysfs attributes: %d", ret);
    goto error;
  }

  return 0;

error:
  fsm_cleanup(&feeding_fsm.fsm);
  return ret;
}


void
feeding_fsm_cleanup(void)
{
  status_remove_files(feeding_fsm_attrs, ARRAY_SIZE(feeding_fsm_attrs));
  fsm_cleanup(&feeding_fsm.fsm);
}


static int
feeding_fsm_init_handler(enum feeding_state_t state,
                         struct feeding_fsm_t *feeding_fsm)
{
  feeding_fsm->entropy_balance = 0;
  fsm_postpone_event(&feeding_fsm->fsm,
                     FEEDING_EVENT_FEEDING_TIME, EATER_FEEDING_TIME_PERIOD);
  return FEEDING_STATE_NORMAL;
}


static int
feeding_fsm_feeding_time_handler(enum feeding_state_t state,
                                 struct feeding_fsm_t *feeding_fsm)
{
  int old_balance = feeding_fsm->entropy_balance;

  brain_msg("it's a good time to get some food");

  feeding_fsm->entropy_balance -= EATER_HUNGER_ENTROPY_REQUIRED;

  TRACE_INFO("Entropy balance changed from %d to %d",
             old_balance, feeding_fsm->entropy_balance);

  /* rescheduling hungriness feeling */
  fsm_postpone_event(&feeding_fsm->fsm,
                     FEEDING_EVENT_FEEDING_TIME, EATER_FEEDING_TIME_PERIOD);

  if (feeding_fsm->entropy_balance <= EATER_ENTROPY_BALANCE_CRITICALLY_LOW) {
    TRACE_INFO("Entropy balance has fallen to the critically low level: %d",
               feeding_fsm->entropy_balance);
    living_fsm_die();
  }

  return classify_entropy_balance(feeding_fsm->entropy_balance);
}


static int
feeding_fsm_feed_handler(enum feeding_state_t state,
                         struct feeding_fsm_t *feeding_fsm,
                         struct feeding_event_feed_data_t *feed_data)
{
  int old_balance;
  unsigned int entropy =
    entropy_estimate(feed_data->food, feed_data->count) * feed_data->count;

  old_balance                   = feeding_fsm->entropy_balance;
  feeding_fsm->entropy_balance += entropy;

  brain_msg("thank you for all the food");

  TRACE_INFO("Entropy balance changed from %d to %d",
             old_balance, feeding_fsm->entropy_balance);

  if (feeding_fsm->entropy_balance >= EATER_ENTROPY_BALANCE_CRITICALLY_HIGH) {
    TRACE_INFO("Entropy balance has risen to critically high level: %d",
               feeding_fsm->entropy_balance);
    living_fsm_die();
  }

  return classify_entropy_balance(feeding_fsm->entropy_balance);
}


static enum feeding_state_t
classify_entropy_balance(int entropy_balance)
{
  if (entropy_balance > EATER_ENTROPY_BALANCE_CRITICALLY_HIGH / 2) {
    return FEEDING_STATE_OVEREATEN;
  } else if (entropy_balance < EATER_ENTROPY_BALANCE_CRITICALLY_LOW / 2) {
    return FEEDING_STATE_HUNGRY;
  } else {
    return FEEDING_STATE_NORMAL;
  }
}


void
feeding_fsm_feed(u8 *food, size_t count)
{
  int ret;
  struct feeding_event_feed_data_t data = {
    .food  = food,
    .count = count,
  };

  ret = fsm_emit(&feeding_fsm.fsm, FEEDING_EVENT_FEED, &data);

  ASSERT( ret == 0 );
}


static ssize_t
feeding_fsm_entropy_balance_attr_show(const char *name,
                                      const struct feeding_fsm_t *feeding_fsm,
                                      char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, "%d\n", feeding_fsm->entropy_balance);
}
