#include <linux/string.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "utils/trace.h"
#include "utils/assert.h"

#include "status/status.h"

#include "fsm/fsm.h"


/// Event that can be postponed.
struct postponed_event_t {
    int                 event; /**< Event type. */
    struct delayed_work work;  /**< Emits the event when time
                                * comes. */
};


/// FSM type.
struct fsm_t {
  /* read only */
  const char *name;             /**< FSM name */
  int   state_count;      /**< Number of states. */
  int   event_count;      /**< Number of events. */

  fsm_state_show_fn_t show_state; /**< Showing function for states. */
  fsm_event_show_fn_t show_event; /**< Showing function for events. */

  void *data;                   /**< Arbitrary data supplied by user. */

  const struct fsm_event_handler_t *handlers; /**< Event handlers. */

  /* read/write */
  int                  state;           /**< Current state. */
  struct status_attr_t state_attr;      /**< State sysfs attribute. */

  struct postponed_event_t postponed_event; /**< Postponed event (if any). */
};


/**
 * Checks whether supplied event is valid. Otherwise BUG()s.
 *
 * @param _fsm   FSM
 * @param _event event to check
 */
#define ASSERT_VALID_EVENT(_fsm, _event)                   \
  ASSERT( (_event) >= 0 && (_event) < (_fsm)->event_count )


/**
 * Checks whether given state seems to be valid. If not BUG()s.
 *
 * @param _fsm   FSM
 * @param _state state to check
 */
#define ASSERT_VALID_STATE(_fsm, _state)                   \
  ASSERT( (_state) >= 0 && (_state) < (_fsm)->state_count )


/**
 * Checks whether event does not require data to be provided. Otherwise BUG()s.
 *
 * @param _fsm   FSM
 * @param _event event type to check
 */
#define ASSERT_NO_DATA_EVENT(_fsm, _event)                              \
  ASSERT( (_fsm)->handlers[(_event)].type == FSM_EVENT_HANDLER_NO_DATA )


/// Shows current state of finite state machine.
static ssize_t
fsm_state_attr_show(const char *name, const struct fsm_t *fsm, char *buffer);


/**
 * Dispatches event to its handler.
 *
 * @param fsm   FSM
 * @param event event to dispatch
 * @param data  data to be fed to event
 *
 * @retval >=0 new state of FSM
 * @retval  <0 error code
 */
static int
fsm_event_dispatch(struct fsm_t *fsm, int event, void *data);


/**
 * Handles postponed events.
 *
 * @param work corresponding work
 */
static void
fsm_postponed_event_worker_fn(struct work_struct *work);


int
fsm_init(struct fsm_t *fsm,
         const char *name,
         int state_count,
         int event_count,
         fsm_state_show_fn_t show_state,
         fsm_event_show_fn_t show_event,
         void *data,
         const struct fsm_event_handler_t handlers[])
{
  int ret;

  char   *state_attr_name;
  size_t  state_attr_name_length;

  ASSERT( state_count >= 1 );
  ASSERT( event_count >= 1 );

  fsm->name        = name;
  fsm->state_count = state_count;
  fsm->event_count = event_count;
  fsm->show_state  = show_state;
  fsm->show_event  = show_event;
  fsm->data        = data;
  fsm->handlers    = handlers;

  fsm->state = 0;
  INIT_DELAYED_WORK(&fsm->postponed_event.work, fsm_postponed_event_worker_fn);


  state_attr_name_length = strlen(name) + strlen("_state") + 1;

  state_attr_name = kmalloc(state_attr_name_length, GFP_KERNEL);
  if (state_attr_name == NULL) {
    TRACE_ERR("Not enough memory");
    return -ENOMEM;
  }

  status_attr_init(&fsm->state_attr, state_attr_name,
                   (status_attr_show_t) fsm_state_attr_show, fsm);

  ret = status_create_file(&fsm->state_attr);
  if (ret != ret) {
    goto error;
  }

  return 0;

error:
  kfree(state_attr_name);
  return ret;
}


void
fsm_cleanup(struct fsm_t *fsm)
{
  fsm_cancel_postponed_event(fsm);
  status_remove_file(&fsm->state_attr);
  kfree(fsm->state_attr.attr.name);
}


static ssize_t
fsm_state_attr_show(const char *name, const struct fsm_t *fsm, char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, fsm->show_state(fsm->state));
}


int
fsm_emit(struct fsm_t *fsm, int event, void *data)
{
  int ret;

  ASSERT_VALID_EVENT( fsm, event );

  TRACE_DEBUG("FSM %s:\n\tstate: %s\n\tincoming event: %s",
              fsm->name,
              fsm->show_state(fsm->state),
              fsm->show_event(event));

  ret = fsm_event_dispatch(fsm, event, data);
  if (ret < 0) {
    TRACE_DEBUG("FSM %s: event handler reports an error: %d",
                fsm->name, ret);
    return ret;
  }

  fsm->state = ret;

  ASSERT_VALID_STATE( fsm, fsm->state );

  TRACE_DEBUG("FSM %s:\n\tnew state: %s",
              fsm->name, fsm->show_state(fsm->state));

  return 0;
}


int
fsm_emit_simple(struct fsm_t *fsm, int event)
{
  ASSERT_VALID_EVENT( fsm, event );
  ASSERT_NO_DATA_EVENT( fsm, event );

  return fsm_emit(fsm, event, NULL);
}


static int
fsm_event_dispatch(struct fsm_t *fsm, int event, void *data)
{
  const struct fsm_event_handler_t *handler;

  ASSERT_VALID_EVENT( fsm, event );

  handler = &fsm->handlers[event];

  if (handler->type == FSM_EVENT_HANDLER_NO_DATA) {
    return handler->h.no_data.fn(fsm->data);
  } else {                      /* FSM_EVENT_HANDLER_WITH_DATA */
    return handler->h.with_data.fn(fsm->data, data);
  }
}


void
fsm_postpone_event(struct fsm_t *fsm, int event, unsigned long delay)
{
  ASSERT_VALID_EVENT( fsm, event );
  ASSERT_NO_DATA_EVENT( fsm, event );

  TRACE_DEBUG("FSM %s: postponing event %s to the future (%us)",
              fsm->name, fsm->show_event(event),
              jiffies_to_msecs(delay) / 1000);

  fsm->postponed_event.event = event;
  schedule_delayed_work(&fsm->postponed_event.work, delay);
}


void
fsm_cancel_postponed_event(struct fsm_t *fsm)
{
  int ret;

  ret = cancel_delayed_work(&fsm->postponed_event.work);
  if (!ret) {
    /* waiting till work terminates */
    flush_delayed_work(&fsm->postponed_event.work);
  }
}


static void
fsm_postponed_event_worker_fn(struct work_struct *work)
{
  int ret;
  struct postponed_event_t *postponed_event;
  struct fsm_t             *fsm;
  int                       event;

  postponed_event = container_of(to_delayed_work(work),
                                 struct postponed_event_t, work);

  event = postponed_event->event;
  fsm = container_of(postponed_event, struct fsm_t, postponed_event);

  TRACE_DEBUG("FSM %s: emitting postponed event %s",
              fsm->name, fsm->show_event(event));

  ret = fsm_emit_simple(fsm, event);
  if (ret != 0) {
    TRACE_ERR("FSM %s: postponed event %s handled with error %d",
              fsm->name, fsm->show_event(event), ret);
  }
}
