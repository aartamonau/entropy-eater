#include <linux/random.h>

#include "utils/random.h"


u8
get_random_u8(void)
{
  u8 buffer[1];
  get_random_bytes(buffer, 1);

  return buffer[0];
}


u16
get_random_u16(void)
{
  u8 buffer[2];
  get_random_bytes(buffer, 2);

  return (buffer[0] << 8) | buffer[1];
}


u32
get_random_u32(void)
{
  u8 buffer[4];
  get_random_bytes(buffer, 4);

  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}


unsigned int
get_random_int(void)
{
  int i;
  unsigned int ret = 0;

  for (i = 0; i < sizeof(int); ++i) {
    ret <<= 8;

    ret |= get_random_u8();
  }

  return ret;
}
