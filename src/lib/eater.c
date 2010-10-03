#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/genl/genl.h>

#include "eater.h"
#include "eater_interface.h"


/// Internally visible connection representation.
struct connection_t {
  int family;                   /**< Generic netlink family that corresponds
                                 * to #EATER_PROTO_NAME. */

  struct nl_handle *sock;       /**< Netlink socket used for communications. */
};


/// Global connection to entropy eater.
static struct connection_t connection = { 0, NULL };


/**
 * Utility macro to catch programmers errors of calling functions which
 * require connection to be established while actually it's not.
 *
 *
 */
#define ASSERT_CONNECTED() assert( connection.sock != NULL )


/**
 * Allocates a message. Then performs entropy eater specific initialization.
 *
 * @param cmd command to be sent in the message
 *
 * @return mesage
 * @retval NULL error occurred
 */
static struct nl_msg *
eater_prepare_message(enum eater_cmd_t cmd);


int
eater_connect(void)
{
  int ret;

  connection.sock = nl_handle_alloc();
  if (connection.sock == NULL) {
    errno = nl_get_errno();
    return EATER_ERROR;
  }

  ret = genl_connect(connection.sock);
  if (ret != 0) {
    errno = -ret;
    goto error;
  }

  ret = genl_ctrl_resolve(connection.sock, EATER_PROTO_NAME);
  connection.family = ret;
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  return EATER_OK;

error:
  nl_handle_destroy(connection.sock);

  return EATER_ERROR;
}


void
eater_disconnect(void)
{
  ASSERT_CONNECTED();

  nl_handle_destroy(connection.sock);
  connection.sock = NULL;
}


static struct nl_msg *
eater_prepare_message(enum eater_cmd_t cmd)
{
  struct nl_msg *msg;
  void          *header;

  ASSERT_CONNECTED();

  msg = nlmsg_alloc();
  if (msg == NULL) {
    errno = nl_get_errno();
    return NULL;
  }

  header = genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, connection.family, 0,
                       NLM_F_ECHO, cmd, EATER_PROTO_VERSION);
  if (header == NULL) {
    errno = nl_get_errno();
    goto error;
  }

  return msg;

error:
  nlmsg_free(msg);

  return NULL;
}


static int
eater_cmd_hello_cb(struct nl_msg *msg, void *arg)
{
  puts("hello cb\n");

  return NL_OK;
}


int
eater_cmd_hello(void)
{
  int ret;
  struct nl_msg *msg;

  msg = eater_prepare_message(EATER_CMD_HELLO);
  if (msg == NULL) {
    return EATER_ERROR;
  }

  ret = nl_send_auto_complete(connection.sock, msg);
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  ret = nl_socket_modify_cb(connection.sock, NL_CB_VALID,
                            NL_CB_CUSTOM, eater_cmd_hello_cb, NULL);

  /* the only error that can reported here is ERANGE */
  assert( ret == 0 );

  ret = nl_recvmsgs_default(connection.sock);
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  ret = EATER_OK;
  goto out;

error:
  ret = EATER_ERROR;
out:
  nlmsg_free(msg);
  return ret;
}


int
eater_cmd_feed(uint8_t *data, size_t count)
{
  int ret;
  struct nl_msg *msg;

  msg = eater_prepare_message(EATER_CMD_FEED);
  if (msg == NULL) {
    return EATER_ERROR;
  }

  ret = nla_put(msg, EATER_ATTR_FOOD, count, data);
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  ret = nl_send_auto_complete(connection.sock, msg);
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  ret = nl_recvmsgs_default(connection.sock);
  if (ret < 0) {
    errno = -ret;
    goto error;
  }

  ret = EATER_OK;
  goto out;

error:
  ret = EATER_ERROR;
out:
  nlmsg_free(msg);
  return ret;
}
