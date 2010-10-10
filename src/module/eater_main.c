#include <linux/init.h>
#include <linux/module.h>

#include "utils/trace.h"
#include "status/status.h"
#include "brain/brain.h"
#include "brain/living_fsm.h"

#include "eater_server.h"


int __init eater_init(void)
{
  int ret;

  ret = eater_server_register();
  if (ret != 0) {
    TRACE_ERR("Cannot register entropy eater server");
    return ret;
  }

  ret = status_create();
  if (ret != 0) {
    goto error_server_unregister;
  }

  ret = brain_init();
  if (ret != 0) {
    TRACE_ERR("Cannot initialize entropy eater's brain. "
              "It's a pain to live without a brain.");
    goto error_status_remove;
  }

  return 0;

error_status_remove:
  status_remove_all_files();
  status_remove();
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

  living_fsm_die_nobly();
  brain_cleanup();

  /* removing all the exported files to make life easier for other modules */
  status_remove_all_files();
  status_remove();
}


module_init(eater_init);
module_exit(eater_exit);

MODULE_AUTHOR("Aliaksiej Artamonau <aliaksiej.artamonau@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:0.0");
