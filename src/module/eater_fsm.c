#include <linux/string.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "utils/trace.h"
#include "utils/entropy.h"

#include "eater_fsm.h"
#include "eater_params.h"
#include "eater_status.h"


/**
 * Used to send notification messages to user.
 *
 * @param format printf-like format string
 */
#define msg(format, ...) \
  printk(KERN_ALERT "Entropy Eater: " format "\n", ##__VA_ARGS__)


/// Eater's FSM states.
enum eater_fsm_state_t {
  EATER_FSM_STATE_IDLE,
  EATER_FSM_STATE_DEAD,
  EATER_FSM_STATE_HUNGRY,
  __EATER_FSM_STATE_LAST
};


/// Number of states in eater's FSM.
#define EATER_FSM_STATES_COUNT __EATER_FSM_STATE_LAST


/// Event handler taking event-specific data.
typedef int (*eater_fsm_event_handler_t)(void *);


/// Event handler that does not take event-specific data.
typedef int (*eater_fsm_event_handler_no_data_t)(void);


/// Event handler type.
enum eater_fsm_event_handler_type_t {
  EATER_FSM_EVENT_HANDLER_INVALID,    /**< Here to be able to check
                                       * non-initialized handlers. */
  EATER_FSM_EVENT_HANDLER_NO_DATA,    /**< Handler without arguments. */
  EATER_FSM_EVENT_HANDLER_WITH_DATA   /**< Handler taking argument. */
};


/// Event handler.
struct eater_fsm_event_handler_t {
  enum eater_fsm_event_handler_type_t type; /**< Handler type. */

  union {
    /// Handler without data.
    struct {
      int (*fn)(void);          /**< Function to be called. */
    } no_data;

    /// Handler with data.
    struct {
      ptrdiff_t offset;         /**< Offset of handlers data in
                                 * #eater_fsm_event_data_t union. */
      int (*fn)(void *);        /**< Function to be called. */
    } with_data;
  } h;
};


#define _EVENT_HANDLER(_handler, _data_member)              \
  {                                                         \
    .with_data = {                                          \
      offsetof(union eater_fsm_event_data_t, _data_member), \
      (eater_fsm_event_handler_t) _handler                  \
    }                                                       \
  }                                                         \


#define _EVENT_HANDLER_NO_DATA(_handler) { .no_data = { _handler } }


/**
 * Static initializer for handler taking data argument.
 *
 * @param _type        type of event that triggers this handler
 * @param _handler     function to be called when event arrives
 * @param _data_member name of the event-specific data in
 *                     #eater_fsm_event_data_t union
 */
#define EVENT(_type, _handler, _data_member)                   \
  [_type] = { .type = EATER_FSM_EVENT_HANDLER_WITH_DATA,       \
              .h    = _EVENT_HANDLER(_handler, _data_member) }


/**
 * Static initializer for handler that does not take data argument.
 *
 * @param _type    type of event that triggers this handler
 * @param _handler function to be called when event arrives
 */
#define EVENT_NO_DATA(_type, _handler)                          \
  [_type] = { .type = EATER_FSM_EVENT_HANDLER_NO_DATA,          \
              .h    = _EVENT_HANDLER_NO_DATA(_handler) }


/**
 * Initializes entropy eater.
 *
 * @retval >=0 new state
 * @retval  <0 error code
 */
static int
eater_fsm_init_handler(void);


/**
 * Called when entropy eater dies but should not call a panic.
 *
 *
 * @retval >=0 new state
 * @retval  <0 error code
 */
static int
eater_fsm_die_nobly_handler(void);


/**
 * Called when entropy eater feels like eating.
 *
 * @retval >=0 new state
 * @retval  <0 error code
 */
static int
eater_fsm_hunger_timeout_handler(void);


/**
 * Handles feeding events.
 *
 * @param feed_data "food"
 *
 * @retval >=0 new state
 * @retval  <0 error code
 */
static int
eater_fsm_feed_handler(const struct eater_fsm_feed_event_data_t *feed_data);


/// Event handlers.
static const
struct eater_fsm_event_handler_t handlers[EATER_FSM_EVENT_TYPES_COUNT] = {
  EVENT_NO_DATA (EATER_FSM_EVENT_TYPE_INIT,
                 eater_fsm_init_handler),

  EVENT_NO_DATA (EATER_FSM_EVENT_TYPE_DIE_NOBLY,
                 eater_fsm_die_nobly_handler),

  EVENT_NO_DATA (EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT,
                 eater_fsm_hunger_timeout_handler),

  EVENT         (EATER_FSM_EVENT_TYPE_FEED,
                 eater_fsm_feed_handler, feed_data),
};


/**
 * Dispatches generic event to specific handlers.
 *
 * @param event event to dispatch
 *
 * @retval >=0 new state of FSM
 * @retval  <0 error code
 */
static int
eater_fsm_event_dispatch(struct eater_fsm_event_t *event);


/// FSM context.
struct eater_fsm_t {
  enum eater_fsm_state_t state; /**< Current state. */

  enum eater_fsm_event_type_t postponed_event;      /**< Postponed event
                                                     * type. */
  struct delayed_work         postponed_event_work; /**< Handles postponed
                                                     * events. */
};


/// FSM.
static struct eater_fsm_t fsm;


/* Auxiliary functions and macros */


/**
 * Checks whether supplied event type is valid. Otherwise BUG()s.
 *
 * @param type event type to check
 */
#define ASSERT_VALID_EVENT_TYPE(type) \
  ASSERT( (type) < EATER_FSM_EVENT_TYPES_COUNT )


/**
 * Checks whether event seems to be valid. If not BUG()s.
 *
 * @param event event type to check
 */
#define ASSERT_VALID_EVENT(event) \
  ASSERT_VALID_EVENT_TYPE( (event)->type )


/**
 * Checks whether given state seems to be valid. If not BUG()s.
 *
 * @param state state to check
 */
#define ASSERT_VALID_STATE(state) \
  ASSERT( (state) < EATER_FSM_STATES_COUNT )


/**
 * Checks whether event does not require data to be provided. Otherwise BUG()s.
 *
 * @param type event type to check
 */
#define ASSERT_NO_DATA_EVENT_TYPE(_type) \
  ASSERT( handlers[(_type)].type == EATER_FSM_EVENT_HANDLER_NO_DATA )


/**
 * Converts event types to strings.
 *
 * @param type event type
 *
 * @return string representation of event type
 */
static const char *
eater_fsm_event_type_to_str(enum eater_fsm_event_type_t type);


/**
 * Converts FSM state to its name.
 *
 * @param state state
 *
 * @return string representing state name
 */
static const char *
eater_fsm_state_to_str(enum eater_fsm_state_t state);


/**
 * Handles postponed events.
 *
 * @param work corresponding work
 */
static void
eater_fsm_postponed_event_worker_fn(struct work_struct *work);


/**
 * Postpones event to the future. Handler for the event must not take
 * arguments.
 *
 * @param event_type event type
 * @param delay      delay
 */
static void
eater_fsm_postpone_event(enum eater_fsm_event_type_t event_type,
                         unsigned long delay);


/**
 * Cancels postponed event.
 *
 */
static void
eater_fsm_cancel_postponed_event(void);


/**
 * Called when 'fsm_state' attribute is read.
 *
 * @return status
 * @retval >=0 size of written data
 * @retval  <0 error code;
 */
static ssize_t
eater_fsm_state_attr_show(const char *, char *);


/// Exported via sysfs state of fsm.
EATER_STATUS_ATTR(fsm_state, eater_fsm_state_attr_show);


int
eater_fsm_init(void)
{
  fsm.state = EATER_FSM_STATE_IDLE;
  INIT_DELAYED_WORK(&fsm.postponed_event_work,
                    eater_fsm_postponed_event_worker_fn);

  return eater_status_create_file(&eater_attr_fsm_state);
}


int
eater_fsm_emit(struct eater_fsm_event_t *event)
{
  int ret;

  ASSERT_VALID_EVENT(event);
  TRACE_DEBUG("Incoming event: %s [current state: %s]",
              eater_fsm_event_type_to_str(event->type),
              eater_fsm_state_to_str(fsm.state));

  ret = eater_fsm_event_dispatch(event);
  if (ret < 0) {
    TRACE_DEBUG("Event handler reports an error: %d", ret);
    return ret;
  }

  fsm.state = ret;

  ASSERT_VALID_STATE( fsm.state );

  TRACE_DEBUG("New state: %s", eater_fsm_state_to_str(fsm.state));

  return 0;
}


int
eater_fsm_emit_simple(enum eater_fsm_event_type_t event_type)
{
  struct eater_fsm_event_t event;

  ASSERT_VALID_EVENT_TYPE( event_type );
  ASSERT_NO_DATA_EVENT_TYPE( event_type );

  event.type = event_type;

  return eater_fsm_emit(&event);
}


static const char *
eater_fsm_event_type_to_str(enum eater_fsm_event_type_t type)
{
  static const char *strs[EATER_FSM_EVENT_TYPES_COUNT] = {
    "EATER_FSM_EVENT_TYPE_INIT",
    "EATER_FSM_EVENT_TYPE_DIE_NOBLY",
    "EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT",
    "EATER_FSM_EVENT_TYPE_FEED",
  };

  return strs[type];
}


static const char *
eater_fsm_state_to_str(enum eater_fsm_state_t state)
{
  static const char *strs[EATER_FSM_STATES_COUNT] = {
    "EATER_FSM_STATE_IDLE",
    "EATER_FSM_STATE_DEAD",
    "EATER_FSM_STATE_HUNGRY",
  };

  return strs[state];
}


static int
eater_fsm_event_dispatch(struct eater_fsm_event_t *event)
{
  const struct eater_fsm_event_handler_t *handler = &handlers[event->type];

  ASSERT( handler->type != EATER_FSM_EVENT_HANDLER_INVALID );

  if (handler->type == EATER_FSM_EVENT_HANDLER_NO_DATA) {
    return handler->h.no_data.fn();
  } else {                      /**< EATER_FSM_EVENT_HANDLER_WITH_DATA */
    void *data = (char *) &event->data + handler->h.with_data.offset;

    return handler->h.with_data.fn(data);
  }
}


static int
eater_fsm_init_handler(void)
{
  switch (fsm.state) {
  case EATER_FSM_STATE_IDLE:
    eater_fsm_postpone_event(EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT,
                             EATER_HUNGER_TIMEOUT);
    break;
  default:
    TRACE_ERR("Invalid internal state (%s) for INIT event.",
              eater_fsm_state_to_str(fsm.state));
    return -EINVAL;
  }

  return EATER_FSM_STATE_IDLE;
}


static int
eater_fsm_die_nobly_handler(void)
{
  /* \todo */

  msg("It was a great experience, Sir");
  eater_fsm_cancel_postponed_event();

  return EATER_FSM_STATE_DEAD;
}


static int
eater_fsm_hunger_timeout_handler(void)
{
  msg("It's a good time to eat.");

  return EATER_FSM_STATE_HUNGRY;
}


static int
eater_fsm_feed_handler(const struct eater_fsm_feed_event_data_t *feed_data)
{
  TRACE_DEBUG("Feed handler triggered. Message entropy: %u",
              (unsigned int) entropy_estimate(feed_data->food,
                                              feed_data->count));

  return fsm.state;
}


static void
eater_fsm_postponed_event_worker_fn(struct work_struct *work)
{
  int ret;
  struct eater_fsm_event_t event = { .type = fsm.postponed_event, };

  ret = eater_fsm_emit(&event);
  if (ret != 0) {
    TRACE_ERR("Postponed event (%s) was handled with error: %d",
              eater_fsm_event_type_to_str(event.type), ret);
  }
}


static void
eater_fsm_postpone_event(enum eater_fsm_event_type_t event_type,
                         unsigned long delay)
{
  ASSERT_VALID_EVENT_TYPE( event_type );
  ASSERT_NO_DATA_EVENT_TYPE( event_type );

  TRACE_DEBUG("Postponing event %s to the future (%us)",
              eater_fsm_event_type_to_str(event_type),
              jiffies_to_msecs(delay) / 1000);

  fsm.postponed_event = event_type;
  schedule_delayed_work(&fsm.postponed_event_work, delay);
}


static void
eater_fsm_cancel_postponed_event(void)
{
  int ret;

  ret = cancel_delayed_work(&fsm.postponed_event_work);
  if (!ret) {
    /* ensuring that work terminated */
    flush_work(&fsm.postponed_event_work.work);
  }
}


static ssize_t
eater_fsm_state_attr_show(const char *attr, char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, "%s\n", eater_fsm_state_to_str(fsm.state));
}
