#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <errno.h>
#include <getopt.h>

#include "eater.h"


static const char *program;

void
usage(void)
{
  fprintf(stderr,
          "Usage:\n"
          "\t%s <global options>\n\n"
          "\t%s <command> <arguments>\n"
          "\n"
          "Global options:\n"
          "\t--help\n"
          "\t\tshow this help;\n"
          "Commands:\n"
          "\thello\n"
          "\t\tsend hello message to entropy eater;\n"
          "\tfeed --food <data>\n"
          "\t\tfeed entropy eater with data;\n"
          "\tsweep\n"
          "\t\tsweep entropy eater's room;\n"
          "\tdisinfect\n"
          "\t\tdisinfect entropy eater's room;\n"
          "\tcure\n"
          "\t\tcure ill entropy eater;\n",
          program, program);
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


#define error(msg, ...) \
  fprintf(stderr, "%s: " msg "\n", program, ##__VA_ARGS__)


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


/// Data for RPS command.
struct command_rps_data_t {
  enum rps_sign_t sign;
};


/// Data for fake global command.
struct command_global_data_t {
  bool help;
};


/// Command data.
union command_data_t {
  struct command_feed_data_t   feed_data;
  struct command_rps_data_t    rps_data;
  struct command_global_data_t global_data;
};


/// Command description.
struct command_t {
  char                    *name;                 /**< Command name. */
  bool                     requires_connection;  /**< Indicates whether
                                                  * command requires
                                                  * connection to the entropy
                                                  * eater. */

  command_handler_t        handler;               /**< Action to be executed. */
  command_opts_handler_t   opts_handler;          /**< Handles CLI options. */
  command_opts_validator_t opts_validator;        /**< Validates CLI options
                                                   * correctness */

  union command_data_t     data;                  /**< Command's data. */

  /// Options for 'getopt_long'.
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

  error("'%s' does not expect any options", command->name);
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
    error("cannot send 'HELLO' to entropy eater: %m", errno);
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
    error("cannot send 'FEED' command to eater: %m", errno);
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


static int
cmd_sweep_handler(struct command_t *command)
{
  int ret;

  ret = eater_cmd_sweep();
  if (ret != EATER_OK) {
    error("cannot send 'SWEEP' command to eater: %m", errno);
    return -1;
  }
  return 0;
}


static int
cmd_disinfect_handler(struct command_t *command)
{
  int ret;

  ret = eater_cmd_disinfect();
  if (ret != EATER_OK) {
    error("cannot send 'DISINFECT' command to eater: %m", errno);
    return -1;
  }
  return 0;
}


static int
cmd_cure_handler(struct command_t *command)
{
  int ret;

  ret = eater_cmd_cure();
  if (ret != EATER_OK) {
    error("cannot send 'CURE' command to eater: %m", errno);
    return -1;
  }
  return 0;
}


static int
cmd_rps_handler(struct command_t *command)
{
  int ret = eater_cmd_play_rps(command->data.rps_data.sign);
  if (ret != EATER_OK) {
    error("cannot send 'PLAY_RPS' command to eater: %m", errno);
    return -1;
  }
  return 0;
}


static int
cmd_rps_opts_handler(struct command_t *command,
                     const char *optname, char *optvalue)
{
  if (strcmp(optname, "sign") == 0) {
    if (strcmp(optvalue, "rock") == 0) {
      command->data.rps_data.sign = RPS_SIGN_ROCK;
    } else if (strcmp(optvalue, "paper") == 0) {
      command->data.rps_data.sign = RPS_SIGN_PAPER;
    } else if (strcmp(optvalue, "scissors") == 0) {
      command->data.rps_data.sign = RPS_SIGN_SCISSORS;
    } else {
      error("invalid value '%s' for the '%s' parameter", optvalue, optname);
    }
  } else {
    /* this is impossible */
    assert( false );
  }

  return 0;
}


static bool
cmd_rps_opts_validator(const struct command_t *command)
{
  if (command->data.rps_data.sign == (enum rps_sign_t) -1) {
    error("'sign' parameter is required for '%s' command", command->name);
    return false;
  }

  return true;
}


struct command_t commands[] = {
  {
    .name                = "hello",
    .requires_connection = true,
    .handler             = cmd_hello_handler,
    .opts_handler        = NULL,
    .opts_validator      = NULL,

    .options = {
      { 0 },
    },
  },
  {
    .name                = "feed",
    .requires_connection = true,
    .handler             = cmd_feed_handler,
    .opts_handler        = cmd_feed_opts_handler,
    .opts_validator      = cmd_feed_opts_validator,

    .data = {
      .feed_data = {
        .food = NULL,
      },
    },

    .options = {
      { "food", required_argument, NULL, 'f' },
      { 0 },
    }
  },
  {
    .name                = "sweep",
    .requires_connection = true,
    .handler             = cmd_sweep_handler,
    .opts_handler        = NULL,
    .opts_validator      = NULL,

    .options = {
      { 0 },
    },
  },
  {
    .name                = "disinfect",
    .requires_connection = true,
    .handler             = cmd_disinfect_handler,
    .opts_handler        = NULL,
    .opts_validator      = NULL,

    .options = {
      { 0 },
    },
  },
  {
    .name                = "cure",
    .requires_connection = true,
    .handler             = cmd_cure_handler,
    .opts_handler        = NULL,
    .opts_validator      = NULL,

    .options = {
      { 0 },
    },
  },
  {
    .name                = "rps",
    .requires_connection = true,
    .handler             = cmd_rps_handler,
    .opts_handler        = cmd_rps_opts_handler,
    .opts_validator      = cmd_rps_opts_validator,

    .data = {
      .rps_data = {
        .sign = (enum rps_sign_t) -1,
      },
    },

    .options = {
      { "sign", required_argument, NULL, 's' },
      { 0 },
    }
  },
};


/// Name of fake global command.
#define GLOBAL_COMMAND "global"


static int
cmd_global_opts_handler(struct command_t *command,
                        const char *optname, char *optvalue)
{
  if (strcmp(optname, "help") == 0) {
    command->data.global_data.help = true;
  } else {
    assert( false );
  }

  return 0;
}


static int
cmd_global_handler(struct command_t *command)
{
  /* for now, print usage here not depending on the value of 'help' because
   * no other global options are handled at all */
  usage();
}


/// Fake command to handle global options.
struct command_t global_command = {
  .name                = GLOBAL_COMMAND,
  .requires_connection = false,
  .handler             = cmd_global_handler,
  .opts_handler        = cmd_global_opts_handler,
  .opts_validator      = NULL,

  .data = {
    .global_data = {
      .help = false
    },
  },

  .options = {
    { "help", no_argument, NULL, 'h' },
    { 0 }
  }
};




/**
 * Distinguishes command names from commands.
 *
 * @param name command/option to check
 *
 * @retval true  name is option
 * @retval false name is command
 */
static bool
looks_like_option(const char *name)
{
  return name[0] == '-';
}


/**
 * Finds command description by its name. If 'name' looks like option
 * fallbacks to #global_command.
 *
 * @param name command name
 *
 * @return command
 * @retval NULL command not found
 */
static struct command_t *
find_command(char *name)
{
  if (looks_like_option(name)) {
    return &global_command;
  }

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
 * @param command command return by find_command()
 * @param argc
 * @param argv
 */
static void
remove_command(const struct command_t *command, int *argc, char *argv[])
{
  /* nothing to do for global command */
  if (command == &global_command) {
    return;
  }

  *argc -= 1;

  for (int i = 1; i < *argc; ++i) {
    argv[i] = argv[i + 1];
  }
}


int
main(int argc, char *argv[])
{
  int   ret;

  program = argv[0];

  struct command_t *command;

  if (argc < 2) {
    usage();
    return EXIT_FAILURE;
  }

  command = find_command(argv[1]);

  if (command == NULL) {
      error("unknown command '%s'", argv[1]);
      return EXIT_FAILURE;
  }

  remove_command(command, &argc, argv);

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

  if (command->requires_connection) {
    ret = eater_connect();
    if (ret != EATER_OK) {
      error("cannot connect to entropy eater");
      return EXIT_FAILURE;
    }
  }

  ret = command_handler(command);
  if (0 != ret) {
    /* error messages are supposed to be printed by the handler */
    ret = EXIT_FAILURE;
  } else {
    ret = EXIT_SUCCESS;
  }

  if (command->requires_connection) {
    eater_disconnect();
  }

  return ret;
}
