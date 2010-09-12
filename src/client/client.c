#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "eater.h"


#define error(msg, ...) \
  fprintf(stderr, msg " Error: %m.\n", ##__VA_ARGS__, errno)


int
main(int argc, char *argv[])
{
  int ret;

  ret = eater_connect();
  if (ret != EATER_OK) {
    error("Cannot connect to entropy eater.");
    return EXIT_FAILURE;
  }

  ret = eater_cmd_hello();
  if (ret != EATER_OK) {
    error("Cannot send hello to entropy eater.");
    ret = EXIT_FAILURE;
    goto out;
  }

  ret = EXIT_SUCCESS;

out:
  eater_disconnect();
  return ret;
}
