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


/// Different classes of events which are sensible to postpone.
enum postponed_event_class_t {
    POSTPONED_EVENT_CLASS_HUNGER,     /**< Events notifying about hungriness
                                       * changes.*/
    POSTPONED_EVENT_CLASS_DAYTIME,    /**< Events notifying about day time
                                       * changes. */
    POSTPONED_EVENT_CLASS_GENERIC,    /**< All other events. */
    __POSTPONED_EVENT_CLASS_LAST,
};


/// Number of postponed events' classes
#define POSTPONED_EVENT_CLASSES_COUNT __POSTPONED_EVENT_CLASS_LAST


/**
 * Returns name for class of postponed events.
 *
 * \param class class
 *
 * \return string representing postponed event class' name
 */
static const char *
postponed_event_class_to_str(enum postponed_event_class_t class);


/// Event that can be postponed.
struct postponed_event_t {
    enum eater_fsm_event_type_t event_type; /**< Event type. */
    struct delayed_work         work;       /**< Emits the event when time
                                             * comes. */
};


/**
 * Returns class of postponed event by its pointer. For this to work the event
 * must be stored in fsm::postponed_events.
 *
 * \param event postponed event
 *
 * \return postponed event class
 */
static enum postponed_event_class_t
postponed_event_to_class(struct postponed_event_t *event);


/// FSM context.
struct eater_fsm_t {
  enum eater_fsm_state_t state; /**< Current state. */

  int  entropy_balance;          /**< Consumed entropy balance. Should be
                                  * close to zero. */

  /// Postponed events. One for each class.
  struct postponed_event_t postponed_events[POSTPONED_EVENT_CLASSES_COUNT];
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
  ASSERT( (enum eater_fsm_event_type_t) (type) < EATER_FSM_EVENT_TYPES_COUNT )


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
  ASSERT( (enum eater_fsm_state_t) (state) < EATER_FSM_STATES_COUNT )


/**
 * Checks whether event does not require data to be provided. Otherwise BUG()s.
 *
 * @param type event type to check
 */
#define ASSERT_NO_DATA_EVENT_TYPE(_type) \
  ASSERT( handlers[(_type)].type == EATER_FSM_EVENT_HANDLER_NO_DATA )


/**
 * Checks whether postponed event class is valid. Otherwise BUG()s.
 *
 * @param class postponed event class to check
 */
#define ASSERT_VALID_CLASS(class) \
  ASSERT( \
    (enum postponed_event_class_t) (class) < POSTPONED_EVENT_CLASSES_COUNT )


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
 * @param class      class to schedule event on
 * @param event_type event type
 * @param delay      delay
 */
static void
eater_fsm_postpone_event(enum postponed_event_class_t class,
                         enum eater_fsm_event_type_t event_type,
                         unsigned long delay);


/**
 * Cancels postponed event.
 *
 * @param class event class to cancel event on
 */
static void
eater_fsm_cancel_postponed_event(enum postponed_event_class_t class);


/**
 * Cancels all postponed events.
 *
 */
static void
eater_fsm_cancel_all_postponed_events(void);


/**
 * Dies loudly.
 */
static void __noreturn
eater_fsm_die(void);


/**
 * Called when 'fsm_state' attribute is read.
 *
 * @return status
 * @retval >=0 size of written data
 * @retval  <0 error code;
 */
static ssize_t
eater_fsm_state_attr_show(const char *, char *);


/**
 * Shows entropy balance.
 */
static ssize_t
eater_fsm_entropy_balance_attr_show(const char *, char *);


/// Exported via sysfs attributes.
static struct eater_status_attribute_t eater_fsm_status_attrs[] = {
  EATER_STATUS_ATTR(fsm_state,       eater_fsm_state_attr_show),
  EATER_STATUS_ATTR(entropy_balance, eater_fsm_entropy_balance_attr_show),
};


int
eater_fsm_init(void)
{
  int i;

  fsm.state = EATER_FSM_STATE_IDLE;

  for (i = 0; i < POSTPONED_EVENT_CLASSES_COUNT; ++i) {
    INIT_DELAYED_WORK(&fsm.postponed_events[i].work,
                      eater_fsm_postponed_event_worker_fn);
  }

  return eater_status_create_files(eater_fsm_status_attrs,
                                   ARRAY_SIZE(eater_fsm_status_attrs));
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
    fsm.entropy_balance = 0;

    eater_fsm_postpone_event(POSTPONED_EVENT_CLASS_HUNGER,
                             EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT,
                             EATER_HUNGER_TIMEOUT);
    break;
  default:
    TRACE_ERR("Invalid internal state (%s) for INIT event.",
              eater_fsm_state_to_str(fsm.state));
    ASSERT( false );
  }

  return EATER_FSM_STATE_IDLE;
}


static int
eater_fsm_die_nobly_handler(void)
{
  /* \todo */

  msg("It was a great experience, Sir!");
  eater_fsm_cancel_all_postponed_events();

  return EATER_FSM_STATE_DEAD;
}


static int
eater_fsm_hunger_timeout_handler(void)
{
  int old_balance = fsm.entropy_balance;

  msg("It's a good time to eat.");

  fsm.entropy_balance -= EATER_HUNGER_ENTROPY_REQUIRED;

  TRACE_INFO("Entropy balance changed from %d to %d",
             old_balance, fsm.entropy_balance);

  if (fsm.entropy_balance < EATER_ENTROPY_BALANCE_CRITICALLY_LOW) {
    TRACE_INFO("Entropy balance fell to %d. Critical level is %d.",
               fsm.entropy_balance, EATER_ENTROPY_BALANCE_CRITICALLY_LOW);

    eater_fsm_die();
  }

  /* rescheduling hungriness feeling */
  eater_fsm_postpone_event(POSTPONED_EVENT_CLASS_HUNGER,
                           EATER_FSM_EVENT_TYPE_HUNGER_TIMEOUT,
                           EATER_HUNGER_TIMEOUT);


  return EATER_FSM_STATE_HUNGRY;
}


static int
eater_fsm_feed_handler(const struct eater_fsm_feed_event_data_t *feed_data)
{
  int old_balance;
  int new_balance;
  unsigned int entropy =
    entropy_estimate(feed_data->food, feed_data->count) * feed_data->count;

  enum eater_fsm_state_t new_state;

  old_balance = fsm.entropy_balance;
  new_balance = old_balance + entropy;

  switch (fsm.state) {
  case EATER_FSM_STATE_IDLE:
    new_state = EATER_FSM_STATE_IDLE;
    break;
  case EATER_FSM_STATE_HUNGRY:
    if (new_balance >= 0) {
      new_state = EATER_FSM_STATE_IDLE;
    } else {
      new_state = EATER_FSM_STATE_HUNGRY;
    }
    break;
  default:
    TRACE_ERR("Invalid internal state (%s) for FEED event.",
              eater_fsm_state_to_str(fsm.state));
    ASSERT( false );
  }

  fsm.entropy_balance = new_balance;

  TRACE_INFO("Entropy balance changed from %d to %d", old_balance, new_balance);
  msg("Got %d bits of entropy. Thank you.", entropy);

  if (fsm.entropy_balance > EATER_ENTROPY_BALANCE_CRITICALLY_HIGH) {
    TRACE_INFO("Entropy balance rose to %d. Critical level is %d.",
               new_balance, EATER_ENTROPY_BALANCE_CRITICALLY_HIGH);

    eater_fsm_die();
  }

  return new_state;
}


static void
eater_fsm_postponed_event_worker_fn(struct work_struct *work)
{
  int ret;
  struct eater_fsm_event_t      event;
  struct postponed_event_t     *postponed_event;
  enum postponed_event_class_t  class;

  postponed_event = container_of(to_delayed_work(work),
                                 struct postponed_event_t, work);
  class           = postponed_event_to_class(postponed_event);

  event.type = postponed_event->event_type;

  TRACE_DEBUG("Processing postponed event %s of class %s",
              eater_fsm_event_type_to_str(event.type),
              postponed_event_class_to_str(class));


  ret = eater_fsm_emit(&event);
  if (ret != 0) {
    TRACE_ERR("Postponed event (%s) was handled with error: %d",
              eater_fsm_event_type_to_str(event.type), ret);
  }
}


static void
eater_fsm_postpone_event(enum postponed_event_class_t class,
                         enum eater_fsm_event_type_t event_type,
                         unsigned long delay)
{
  ASSERT_VALID_CLASS( class );
  ASSERT_VALID_EVENT_TYPE( event_type );
  ASSERT_NO_DATA_EVENT_TYPE( event_type );

  TRACE_DEBUG("Postponing event %s (class: %s) to the future (%us)",
              eater_fsm_event_type_to_str(event_type),
              postponed_event_class_to_str(class),
              jiffies_to_msecs(delay) / 1000);

  fsm.postponed_events[class].event_type = event_type;

  schedule_delayed_work(&fsm.postponed_events[class].work, delay);
}


static void
eater_fsm_cancel_postponed_event(enum postponed_event_class_t class)
{
  int ret;

  ASSERT_VALID_CLASS( class );

  ret = cancel_delayed_work(&fsm.postponed_events[class].work);
  if (!ret) {
    /* ensuring that work terminated */
    flush_delayed_work(&fsm.postponed_events[class].work);
  }
}


static void
eater_fsm_cancel_all_postponed_events(void)
{
  int i;

  for (i = 0; i < POSTPONED_EVENT_CLASSES_COUNT; ++i) {
    eater_fsm_cancel_postponed_event(i);
  }
}


static ssize_t
eater_fsm_state_attr_show(const char *attr, char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, "%s\n", eater_fsm_state_to_str(fsm.state));
}


static ssize_t
eater_fsm_entropy_balance_attr_show(const char *attr, char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, "%d\n", fsm.entropy_balance);
}


static const char *
postponed_event_class_to_str(enum postponed_event_class_t class)
{
  static const char *strs[] = {
    [POSTPONED_EVENT_CLASS_HUNGER]  = "POSTPONED_EVENT_CLASS_HUNGER",
    [POSTPONED_EVENT_CLASS_DAYTIME] = "POSTPONED_EVENT_CLASS_DAYTIME",
    [POSTPONED_EVENT_CLASS_GENERIC] = "POSTPONED_EVENT_CLASS_GENERIC",
  };

  ASSERT( class < POSTPONED_EVENT_CLASSES_COUNT );

  return strs[class];
}


static enum postponed_event_class_t
postponed_event_to_class(struct postponed_event_t *event)
{
  enum postponed_event_class_t class = event - fsm.postponed_events;

  ASSERT_VALID_CLASS( class );

  return class;
}


static void
eater_fsm_die(void)
{
  msg("You've been a bad owner. I'm dying in agony.");
  panic("DEAD");
}
