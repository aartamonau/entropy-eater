#include "eater_server.h"

#include "utils/trace.h"
#include "utils/rps.h"
#include "brain/sanitation_fsm.h"
#include "brain/feeding_fsm.h"
#include "brain/living_fsm.h"
#include "brain/social_fsm.h"


/// Attributes' policies.
static struct nla_policy eater_attr_policy[] = {
  [EATER_ATTR_NONE]     = { .type = NLA_UNSPEC, .len = 0 },
  [EATER_ATTR_FOOD]     = { .type = NLA_BINARY },
  [EATER_ATTR_RPS_SIGN] = { .type = NLA_U8 }
};


/// Entropy eater family description.
static struct genl_family eater_genl_family = {
  .id      = GENL_ID_GENERATE,
  .hdrsize = 0,
  .name    = EATER_PROTO_NAME,
  .version = EATER_PROTO_VERSION,
  .maxattr = EATER_ATTR_MAX,
};


/**
 * Implementation for eater_cmd_t::EATER_CMD_HELLO.
 *
 */
static int
eater_hello(struct sk_buff *skb, struct genl_info *info);



/**
 * Implementation for eater_cmd_t::EATER_CMD_FEED.
 *
 */
static int
eater_feed(struct sk_buff *skb, struct genl_info *info);


/**
 * Implementation for eater_cmd_t::EATER_CMD_SWEEP.
 *
 */
static int
eater_sweep(struct sk_buff *skb, struct genl_info *info);


/**
 * Implementation for eater_cmd_t::EATER_CMD_DISINFECT.
 *
 */
static int
eater_disinfect(struct sk_buff *skb, struct genl_info *info);


/**
 * Implementation for eater_cmd_t::EATER_CMD_CURE.
 *
 */
static int
eater_cure(struct sk_buff *skb, struct genl_info *info);


/**
 * Implementation for eater_cmd_t::EATER_CMD_PLAY_RPS
 *
 */
static int
eater_play_rps(struct sk_buff *skb, struct genl_info *info);


/// Entropy eater commands.
static struct genl_ops eater_cmds[] = {
  {
    .cmd    = EATER_CMD_HELLO,
    .policy = eater_attr_policy,
    .doit   = eater_hello,
  },
  {
    .cmd    = EATER_CMD_FEED,
    .policy = eater_attr_policy,
    .doit   = eater_feed,
  },
  {
    .cmd    = EATER_CMD_SWEEP,
    .policy = eater_attr_policy,
    .doit   = eater_sweep,
  },
  {
    .cmd    = EATER_CMD_DISINFECT,
    .policy = eater_attr_policy,
    .doit   = eater_disinfect,
  },
  {
    .cmd    = EATER_CMD_CURE,
    .policy = eater_attr_policy,
    .doit   = eater_cure,
  },
  {
    .cmd    = EATER_CMD_PLAY_RPS,
    .policy = eater_attr_policy,
    .doit   = eater_play_rps,
  },
};


int
eater_server_register(void)
{
  int ret;

  ret = genl_register_family_with_ops(&eater_genl_family,
                                      eater_cmds, ARRAY_SIZE(eater_cmds));
  if (ret != 0) {
    TRACE_ERR("genl_register_family_with_ops failed: %d", ret);
    return ret;
  }

  return 0;
}


int
eater_server_unregister(void)
{
  int ret;

  ret = genl_unregister_family(&eater_genl_family);
  if (0 != ret) {
    TRACE_ERR("genl_unregister_family failed: %d", ret);
    return ret;
  }

  return 0;
}


static int
eater_hello(struct sk_buff *skb, struct genl_info *info)
{
  printk(KERN_INFO "hello from entropy eater server\n");

  return 0;
}


static int
eater_feed(struct sk_buff *skb, struct genl_info *info)
{
  u8    *data;
  size_t data_length;

  if (!info->attrs[EATER_ATTR_FOOD]) {
    TRACE_ERR("EATER_ATTR_FOOD attribute not found");
    return -EINVAL;
  }

  data        = nla_data(info->attrs[EATER_ATTR_FOOD]);
  data_length = nla_len(info->attrs[EATER_ATTR_FOOD]);

  feeding_fsm_feed(data, data_length);

  return 0;
}


static int
eater_sweep(struct sk_buff *skb, struct genl_info *info)
{
  return sanitation_fsm_sweep();
}


static int
eater_disinfect(struct sk_buff *skb, struct genl_info *info)
{
  return sanitation_fsm_disinfect();
}


static int
eater_cure(struct sk_buff *skb, struct genl_info *info)
{
  return living_fsm_cure_illness();
}

static int
eater_play_rps(struct sk_buff *skb, struct genl_info *info)
{
  u8 sign;

  if (!info->attrs[EATER_ATTR_RPS_SIGN]) {
    TRACE_ERR("EATER_ATTR_RPS_SIGN attribute not found");
    return -EINVAL;
  }

  sign = nla_get_u8(info->attrs[EATER_ATTR_RPS_SIGN]);
  if (sign >= RPS_SIGNS_COUNT) {
    TRACE_ERR("Too big value %u for the RPS sign", sign);
    return -EINVAL;
  }

  social_fsm_play_rps(sign);

  return 0;
}
