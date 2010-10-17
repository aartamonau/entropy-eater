#include "utils/trace.h"

#include "brain/brain.h"
#include "brain/living_fsm.h"
#include "brain/sanitation_fsm.h"
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

  ret = sanitation_fsm_init();
  if (ret != 0) {
    TRACE_ERR("Failed to initialize sanitation FSM: %d", ret);
    goto error_living_fsm_cleanup;
  }

  ret = feeding_fsm_init();
  if (ret != 0) {
    TRACE_ERR("Failed to initialize feeding FSM: %d", ret);
    goto error_sanitation_fsm_cleanup;
  }

  return 0;

error_sanitation_fsm_cleanup:
  sanitation_fsm_cleanup();
error_living_fsm_cleanup:
  living_fsm_cleanup();
  return ret;
}


void
brain_cleanup(void)
{
  feeding_fsm_cleanup();
  sanitation_fsm_cleanup();
  living_fsm_cleanup();
}
