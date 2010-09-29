#include <linux/init.h>
#include <linux/module.h>

#include "trace.h"
#include "eater_server.h"
#include "eater_status.h"
#include "eater_fsm.h"


ssize_t show(const char *name, char *buffer)
{
  return snprintf(buffer, PAGE_SIZE, "%s: test\n", name);
}


EATER_STATUS_ATTR(attr, show);


int __init eater_init(void)
{
  int ret;

  ret = eater_server_register();
  if (ret != 0) {
    TRACE_ERR("Cannot register entropy eater server");
    return ret;
  }

  ret = eater_status_create();
  if (ret != 0) {
    goto error_server_unregister;
  }

  eater_status_create_file(&eater_attr_attr);
  eater_fsm_init();

  ret = eater_fsm_emit_simple(EATER_FSM_EVENT_TYPE_INIT);
  if (ret != 0) {
    TRACE_ERR("Cannot initialize entropy eater");
    goto error_server_unregister;
  }

  return 0;

error_server_unregister:
  ret = eater_server_unregister();
  if (ret != 0) {
    TRACE_CRIT("Entropy eater left in inconsistent state because of "
               "unrecoverable errors");
  }

  return ret;
}


void __exit eater_exit(void)
{
  int ret;

  ret = eater_server_unregister();
  if (ret != 0) {
    TRACE_ERR("Cannot unregister entropy eater server");
  }

  ret = eater_fsm_emit_simple(EATER_FSM_EVENT_TYPE_DIE_NOBLY);
  if (ret != 0) {
    TRACE_ERR("Sorrowfully, entropy eater has been unable to say his last "
              "noble word to you");
  }

  eater_status_remove_file(&eater_attr_attr);
  eater_status_remove();
}


module_init(eater_init);
module_exit(eater_exit);

MODULE_AUTHOR("Aliaksiej Artamonau <aliaksiej.artamonau@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:0.0");
