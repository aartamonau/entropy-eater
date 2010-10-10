#include "utils/trace.h"

#include "brain/brain.h"
#include "brain/living_fsm.h"
#include "brain/feeding_fsm.h"


int
brain_init(void)
{
  int ret;

  ret = living_fsm_init();
  if (ret != 0) {
    TRACE_ERR("Failed to initialize living FSM: %d", ret);
    return ret;
  }

  ret = feeding_fsm_init();
  if (ret != 0) {
    TRACE_ERR("Failed to initialize feeding FSM: %d", ret);
    goto error_living_fsm_cleanup;
  }

  return 0;

error_living_fsm_cleanup:
  living_fsm_cleanup();
  return ret;
}


void
brain_cleanup(void)
{
  feeding_fsm_cleanup();
  living_fsm_cleanup();
}
