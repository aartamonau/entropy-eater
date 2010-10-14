#include <linux/string.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "utils/trace.h"
#include "utils/assert.h"

#include "status/status.h"

#include "fsm/fsm.h"


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
 * Dispatches event to its handler. Lock should be acquired by the caller.
 *
 * @param fsm   FSM
 * @param event event to dispatch
 * @param data  data to be fed to event
 *
 * @retval >=0 new state of FSM
 * @retval  <0 error code
 */
static int
__fsm_event_dispatch(struct fsm_t *fsm, int event, void *data);


/**
 * Handles postponed events.
 *
 * @param work corresponding work
 */
static void
fsm_postponed_events_work_fn(struct work_struct *work);


/**
 * Initializes postponed events structure.
 *
 * @param postponed_events structure to initialize
 */
static void
fsm_postponed_events_init(struct fsm_postponed_events_t *postponed_events);


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

  rwlock_init(&fsm->lock);

  fsm->name        = name;
  fsm->state_count = state_count;
  fsm->event_count = event_count;
  fsm->show_state  = show_state;
  fsm->show_event  = show_event;
  fsm->data        = data;
  fsm->handlers    = handlers;

  fsm->state = 0;
  fsm_postponed_events_init(&fsm->postponed_events);

  state_attr_name_length = strlen(name) + strlen("_state") + 1;

  state_attr_name = kmalloc(state_attr_name_length, GFP_KERNEL);
  if (state_attr_name == NULL) {
    TRACE_ERR("Not enough memory");
    return -ENOMEM;
  }
  snprintf(state_attr_name, state_attr_name_length, "%s_state", name);

  status_attr_init(&fsm->state_attr, state_attr_name,
                   (status_attr_show_t) fsm_state_attr_show, fsm);

  ret = status_create_file(&fsm->state_attr);
  if (ret != 0) {
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
  fsm_cancel_postponed_events(fsm);
  status_remove_file(&fsm->state_attr);
  kfree(fsm->state_attr.attr.name);
}


static ssize_t
fsm_state_attr_show(const char *name, const struct fsm_t *fsm, char *buffer)
{
  int state;

  read_lock(&fsm->lock);
  state = fsm->state;
  read_unlock(&fsm->lock);

  return snprintf(buffer, PAGE_SIZE, "%s\n", fsm->show_state(state));
}


int
__fsm_emit(struct fsm_t *fsm, int event, void *data)
{
  int ret;

  ASSERT_VALID_EVENT( fsm, event );

  TRACE_DEBUG("FSM %s: tstate: %s, incoming event: %s",
              fsm->name,
              fsm->show_state(fsm->state),
              fsm->show_event(event));


  ret = __fsm_event_dispatch(fsm, event, data);
  if (ret < 0) {
    TRACE_DEBUG("FSM %s: event handler reports an error: %d",
                fsm->name, ret);
    return ret;
  }

  fsm->state = ret;

  ASSERT_VALID_STATE( fsm, fsm->state );

  TRACE_DEBUG("FSM %s: new state: %s",
              fsm->name, fsm->show_state(fsm->state));

  return 0;
}


int
fsm_emit(struct fsm_t *fsm, int event, void *data)
{
  int ret;

  write_lock(&fsm->lock);
  ret = __fsm_emit(fsm, event, data);
  write_unlock(&fsm->lock);

  return ret;
}


int
fsm_emit_simple(struct fsm_t *fsm, int event)
{
  int ret;

  ASSERT_VALID_EVENT( fsm, event );
  ASSERT_NO_DATA_EVENT( fsm, event );

  write_lock(&fsm->lock);
  ret = __fsm_emit(fsm, event, NULL);
  write_unlock(&fsm->lock);

  return ret;
}


static int
__fsm_event_dispatch(struct fsm_t *fsm, int event, void *data)
{
  const struct fsm_event_handler_t *handler;

  ASSERT_VALID_EVENT( fsm, event );

  handler = &fsm->handlers[event];

  if (handler->type == FSM_EVENT_HANDLER_NO_DATA) {
    return handler->h.no_data.fn(fsm->state, fsm->data);
  } else {                      /* FSM_EVENT_HANDLER_WITH_DATA */
    return handler->h.with_data.fn(fsm->state, fsm->data, data);
  }
}


int
fsm_postpone_event(struct fsm_t *fsm, int event, unsigned long delay)
{
  bool schedule;
  struct fsm_postponed_event_t *postponed_event;

  ASSERT_VALID_EVENT( fsm, event );
  ASSERT_NO_DATA_EVENT( fsm, event );

  postponed_event = kmalloc(sizeof(*postponed_event), GFP_KERNEL);
  if (postponed_event == NULL) {
    TRACE_ERR("Unable to allocate memory for a postponed event");
    return -ENOMEM;
  }

  postponed_event->event = event;
  postponed_event->time  = jiffies + delay;

  TRACE_DEBUG("FSM %s: postponing event %s to the future (%us)",
              fsm->name, fsm->show_event(event),
              jiffies_to_msecs(delay) / 1000);


  spin_lock(&fsm->postponed_events.lock);

  schedule = list_empty(&fsm->postponed_events.events);
  list_add_tail(&postponed_event->list, &fsm->postponed_events.events);

  if (schedule) {
    schedule_delayed_work(&fsm->postponed_events.work, delay);
  }

  spin_unlock(&fsm->postponed_events.lock);


  return 0;
}


void
fsm_cancel_postponed_events(struct fsm_t *fsm)
{
  int ret;
  struct fsm_postponed_event_t *event;
  struct fsm_postponed_event_t *tmp;


  atomic_set(&fsm->postponed_events.cancel, 1);
  ret = cancel_delayed_work(&fsm->postponed_events.work);
  if (!ret) {
    flush_delayed_work(&fsm->postponed_events.work);
  }

  spin_lock(&fsm->postponed_events.lock);

  list_for_each_entry_safe(event, tmp, &fsm->postponed_events.events, list) {
    list_del(&event->list);
    kfree(event);
  }

  spin_unlock(&fsm->postponed_events.lock);

  atomic_set(&fsm->postponed_events.cancel, 0);
}


void
fsm_cancel_postponed_event_by_type(struct fsm_t *fsm, int event_type)
{
  int ret;
  struct fsm_postponed_event_t *event;
  struct fsm_postponed_event_t *tmp;

  ASSERT_VALID_EVENT( fsm, event_type );

  atomic_set(&fsm->postponed_events.cancel, 1);

  ret = cancel_delayed_work(&fsm->postponed_events.work);
  if (!ret) {
    flush_delayed_work(&fsm->postponed_events.work);
  }

  spin_lock(&fsm->postponed_events.lock);

  list_for_each_entry_safe(event, tmp, &fsm->postponed_events.events, list) {
    if (event->event == event_type) {
      list_del(&event->list);
      kfree(event);
    }
  }

  if (!list_empty(&fsm->postponed_events.events)) {
    struct fsm_postponed_event_t *head;

    head = list_first_entry(&fsm->postponed_events.events,
                            struct fsm_postponed_event_t, list);

    schedule_delayed_work(&fsm->postponed_events.work, head->time - jiffies);
  }

  spin_unlock(&fsm->postponed_events.lock);

  atomic_set(&fsm->postponed_events.cancel, 0);
}


static void
fsm_postponed_events_work_fn(struct work_struct *work)
{
  int ret;

  struct fsm_t                  *fsm;
  struct fsm_postponed_events_t *postponed_events;
  struct fsm_postponed_event_t  *postponed_event;
  struct fsm_postponed_event_t  *next_event = NULL;

  postponed_events = container_of(to_delayed_work(work),
                                  struct fsm_postponed_events_t, work);

  spin_lock(&postponed_events->lock);

  ASSERT( !list_empty(&postponed_events->events) );

  postponed_event = list_first_entry(&postponed_events->events,
                                     struct fsm_postponed_event_t, list);
  list_del(&postponed_event->list);

  if (!list_empty(&postponed_events->events)) {
    next_event = list_first_entry(&postponed_events->events,
                                  struct fsm_postponed_event_t, list);
  }

  spin_unlock(&postponed_events->lock);

  fsm = container_of(postponed_events, struct fsm_t, postponed_events);

  TRACE_DEBUG("FSM %s: emitting postponed event %s",
              fsm->name, fsm->show_event(postponed_event->event));

  ret = fsm_emit_simple(fsm, postponed_event->event);
  if (ret != 0) {
    TRACE_ERR("FSM %s: postponed event %s handled with error %d",
              fsm->name, fsm->show_event(postponed_event->event), ret);
  }

  if ((next_event != NULL) && !atomic_read(&postponed_events->cancel)) {
    unsigned long now;
    unsigned long delay;

    now = jiffies;

    if (next_event->time < now) {
      delay = 0;
    } else {
      delay = next_event->time - now;
    }

    TRACE_DEBUG("Rescheduling postponed events work to %us in future",
                jiffies_to_msecs(delay) / 1000);

    schedule_delayed_work(&postponed_events->work, delay);
  }

  kfree(postponed_event);
}


static void
fsm_postponed_events_init(struct fsm_postponed_events_t *postponed_events)
{
  spin_lock_init(&postponed_events->lock);
  INIT_DELAYED_WORK(&postponed_events->work, fsm_postponed_events_work_fn);
  INIT_LIST_HEAD(&postponed_events->events);
  atomic_set(&postponed_events->cancel, 0);
}
