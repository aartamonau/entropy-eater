/**
 * @file   fsm.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Thu Oct  7 22:10:24 2010
 *
 * @brief Reusable finite state machines.
 *
 *
 */

#ifndef _FSM_H_
#define _FSM_H_


#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/list.h>

#include "status/status.h"


/// Event handler taking event-specific data.
typedef int (*fsm_event_handler_t)(int, void *, void *);


/// Event handler that does not take event-specific data.
typedef int (*fsm_event_handler_no_data_t)(int, void *);


/// Different types of event handlers.
enum fsm_event_handler_type_t {
  FSM_EVENT_HANDLER_INVALID,    /**< Here to be able to check
                                       * non-initialized handlers. */
  FSM_EVENT_HANDLER_NO_DATA,    /**< Handler without arguments. */
  FSM_EVENT_HANDLER_WITH_DATA   /**< Handler taking argument. */
};


/// Event handler.
struct fsm_event_handler_t {
  enum fsm_event_handler_type_t type; /**< Handler type. */

  union {
    /// Handler without data.
    struct {
      int (*fn)(int, void *);          /**< Function to be called. */
    } no_data;

    /// Handler with data.
    struct {
      int (*fn)(int, void *, void *);  /**< Function to be called. */
    } with_data;
  } h;
};


/// Not supposed to be used directly.
#define _EVENT_HANDLER(_handler)                            \
  {                                                         \
    .with_data = {                                          \
      _handler                                              \
    }                                                       \
  }                                                         \


/// Not supposed to be used directly.
#define _EVENT_HANDLER_NO_DATA(_handler) \
  { .no_data = { _handler } }


/**
 * Event handler declarator.
 *
 * @param _type        type of event that triggers this handler
 * @param _handler     function to be called when event arrives
 */
#define EVENT(_type, _handler)                                 \
  [_type] = { .type = FSM_EVENT_HANDLER_WITH_DATA,             \
              .h    = _EVENT_HANDLER(_handler) }


/**
 * Event handler declarator (for events without data parameter).
 *
 * @param _type    type of event that triggers this handler
 * @param _handler function to be called when event arrives
 */
#define EVENT_NO_DATA(_type, _handler)                    \
  [_type] = { .type = FSM_EVENT_HANDLER_NO_DATA,          \
              .h    = _EVENT_HANDLER_NO_DATA(_handler) }


/// Handles postponed events.
struct fsm_postponed_events_t {
  spinlock_t          lock;

  atomic_t            cancel;    /**< Flags when no rescheduling should be
                                  * performed. */
  struct delayed_work work;      /**< Work doing the main job. */
  struct list_head    events;    /**< Queue of the postponed events. */
};


/// Single postponed event.
struct fsm_postponed_event_t {
  int           event;          /**< Event type. */
  unsigned long time;           /**< Time when it should be emitted. */

  struct list_head list;        /**< Forms a list of events. */
};


/// Function transforming FSM state to its character presentation.
typedef const char *(*fsm_state_show_fn_t)(int state);


/// Function transforming FSM event to its character presentation.
typedef const char *(*fsm_event_show_fn_t)(int event);


/// FSM type.
struct fsm_t {
  /* read only */
  const char *name;       /**< FSM name */
  int   state_count;      /**< Number of states. */
  int   event_count;      /**< Number of events. */

  fsm_state_show_fn_t show_state; /**< Showing function for states. */
  fsm_event_show_fn_t show_event; /**< Showing function for events. */

  void *data;                   /**< Arbitrary data supplied by user. */

  const struct fsm_event_handler_t *handlers; /**< Event handlers. */

  /* read/write */
  int                  state;           /**< Current state. */
  struct status_attr_t state_attr;      /**< State sysfs attribute. */

  struct fsm_postponed_events_t postponed_events; /**< Postponed events. */
};


/**
 * Initialized FSM.
 *
 * @param fsm         FSM to initialize.
 * @param name        Name of FSM.
 * @param state_count Number of states in FSM.
 * @param event_count Number of events in FSM
 * @param show_state  Showing function for states.
 * @param show_event  Showing function for events.
 * @param handlers    Table of event handlers.
 *
 * @retval  0 FSM initialized successfully
 * @retval <0 error occurred
 */
int
fsm_init(struct fsm_t *fsm,
         const char *name,
         int state_count,
         int event_count,
         fsm_state_show_fn_t show_state,
         fsm_event_show_fn_t show_event,
         void *data,
         const struct fsm_event_handler_t handlers[]);


/**
 * Frees the memory hold by FSM and removes FSM's attributes from sysfs.
 *
 * @param fsm FSM
 */
void
fsm_cleanup(struct fsm_t *fsm);


/**
 * Feeds event to FSM
 *
 * @param fsm   FSM
 * @param event event type
 * @param data  data for event handler
 *
 * @retval  0 success
 * @retval <0 error returned by event handler
 */
int
fsm_emit(struct fsm_t *fsm, int event, void *data);


/**
 * Simplified version of fsm_emit() function. This function does not take a
 * pointer to a data to path to the handler. Instead just event type is
 * taken. As a drawback only events that do not take data parameter can be
 * emitted.
 *
 * @param fsm   FSM
 * @param event event to emit.
 *
 * @retval  0 success
 * @retval <0 error code
 */
int
fsm_emit_simple(struct fsm_t *fsm, int event);


/**
 * Postpones event to the future. Handler for the event must not take
 * arguments.
 *
 * @param fsm    FSM
 * @param event  event type
 * @param delay  delay in jiffies
 *
 * @return execution status
 */
int
fsm_postpone_event(struct fsm_t *fsm,
                   int event_type, unsigned long delay);


/**
 * Cancels postponed events.
 *
 * @param fsm FSM
 */
void
fsm_cancel_postponed_events(struct fsm_t *fsm);


/**
 * Cancels all the events of specific types.
 *
 * @param fsm   FSM
 * @param event event
 */
void
fsm_cancel_postponed_event_by_type(struct fsm_t *fsm, int event);


#endif /* _FSM_H_ */
