#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <errno.h>
#include <getopt.h>

#include "eater.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


#define error(msg, ...) \
  fprintf(stderr, msg "\n", ##__VA_ARGS__)


struct command_t;


/// Handler of command-specific options.
typedef int (*command_opts_handler_t)(struct command_t *command,
                                      const char *optname, char *optvalue);


/// Validator of command-specific options.
typedef bool (*command_opts_validator_t)(const struct command_t *command);


/// Command handler.
typedef int (*command_handler_t)(struct command_t *command);


/// Maximum number of options single command can handle.
#define MAX_COMMAND_OPTIONS 10


/// Data for FEED command.
struct command_feed_data_t {
  uint8_t *food;
  size_t   count;
};


/// Command data.
union command_data_t {
  struct command_feed_data_t feed_data;
};


/// Command description.
struct command_t {
  char                    *name;
  command_handler_t        handler;
  command_opts_handler_t   opts_handler;
  command_opts_validator_t opts_validator;

  union command_data_t     data;

  struct option            options[MAX_COMMAND_OPTIONS + 1];
};


/**
 * Calls handler method of a command.
 */
static inline int
command_handler(struct command_t *command)
{
  assert( command->handler != NULL );

  return command->handler(command);
}


/**
 * Calls options handler method of a command if it was specified. Otherwise
 * reports an error.
 */
static inline int
command_opts_handler(struct command_t *command,
                     const char *optname, char *optvalue)
{
  if (command->opts_handler != NULL) {
    return command->opts_handler(command, optname, optvalue);
  }

  error("Command '%s' does not expect any options.", command->name);
  return -1;
}


/**
 * Calls validator method of a command if it was specified. Otherwise always
 * returns 'true'.
 */
static inline bool
command_opts_validator(const struct command_t *command)
{
  if (command->opts_validator != NULL) {
    return command->opts_validator(command);
  }

  return true;
}


/**
 * Sends 'hello' to the entropy eater.
 */
static int
cmd_hello_handler(struct command_t *command)
{
  int ret = eater_cmd_hello();
  if (ret != EATER_OK) {
    error("Cannot send 'hello' to entropy eater: %m.", errno);
    return -1;
  }

  return 0;
}


static int
cmd_feed_handler(struct command_t *command)
{
  int ret = eater_cmd_feed(command->data.feed_data.food,
                           command->data.feed_data.count);
  if (ret != EATER_OK) {
    error("Cannot send 'FEED' command to eater: %m.", errno);
    return -1;
  }
  return 0;
}


static int
cmd_feed_opts_handler(struct command_t *command,
                      const char *optname, char *optvalue)
{
  if (strcmp(optname, "food") == 0) {
    command->data.feed_data.food  = optvalue;
    command->data.feed_data.count = strlen(optvalue);
  } else {
    /* this is impossible */
    assert( false );
  }

  return 0;
}


static bool
cmd_feed_opts_validator(const struct command_t *command)
{
  if (command->data.feed_data.food == NULL) {
    error("'food' parameter is required for '%s' command", command->name);
    return false;
  }

  return true;
}


struct command_t commands[] = {
  {
    .name           = "hello",
    .handler        = cmd_hello_handler,
    .opts_handler   = NULL,
    .opts_validator = NULL,
    .options        = {
      {0, 0, 0, 0},
    },
  },
  {
    .name           = "feed",
    .handler        = cmd_feed_handler,
    .opts_handler   = cmd_feed_opts_handler,
    .opts_validator = cmd_feed_opts_validator,

    .data           = {
      .feed_data = {
        .food = NULL,
      },
    },

    .options        = {
      { "food", required_argument, NULL, 'f' },
      {0, 0, 0, 0},
    }
  }
};


void
usage(char *program)
{
  fprintf(stderr,
          "Usage:\n"
          "\t%s <command> <arguments>\n"
          "\n"
          "Commands:\n"
          "\thello\n"
          "\tfeed --food <data>\n", program);
}


/**
 * Finds command description by its name.
 *
 * @param name command name
 *
 * @return command
 * @retval NULL command not found
 */
static struct command_t *
find_command(char *name)
{
  for (int i = 0; i < ARRAY_SIZE(commands); ++i) {
    if (strcmp(name, commands[i].name) == 0) {
      return &commands[i];
    }
  }

  return NULL;
}


/**
 * Removes command name from 'argv' and modifies 'argc' accordingly. So that
 * they can be used in the call to getopt().
 *
 * @param argc
 * @param argv
 */
static void
remove_command(int *argc, char *argv[])
{
  *argc -= 1;

  for (int i = 1; i < *argc; ++i) {
    argv[i] = argv[i + 1];
  }
}


int
main(int argc, char *argv[])
{
  int   ret;

  struct command_t *command;

  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  command = find_command(argv[1]);
  if (command == NULL) {
    error("Unknown command: %s\n", argv[1]);
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  remove_command(&argc, argv);

  while (true) {
    int  optind = 0;
    char opt;
    int  ret;

    opt = getopt_long(argc, argv, "", command->options, &optind);

    if (opt == -1) {
      /* no more arguments */
      break;
    } else if (opt == '?') {
      /* unknown option */
      /* error message is issued by getopt itself */
      return EXIT_FAILURE;
    }

    ret = command_opts_handler(command, command->options[optind].name, optarg);
    if (ret != 0) {
      /* error message is supposed to be printed by the handler */
      return EXIT_FAILURE;
    }
  }

  if (!command_opts_validator(command)) {
    /* error messages are supposed to be printed by the handler */
    return EXIT_FAILURE;
  }

  ret = eater_connect();
  if (ret != EATER_OK) {
    error("Cannot connect to entropy eater.");
    return EXIT_FAILURE;
  }

  ret = command_handler(command);
  if (0 != ret) {
    /* error messages are supposed to be printed by the handler */
    ret = EXIT_FAILURE;
  } else {
    ret = EXIT_SUCCESS;
  }

  eater_disconnect();

  return ret;
}
