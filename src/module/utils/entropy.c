#include <linux/bitops.h>

#include "utils/assert.h"
#include "utils/entropy.h"
#include "utils/log2.h"


u8
entropy_estimate(u8 *data, size_t n)
{
  int    i;
  u8    *byte;
  size_t counters[1 << BITS_PER_BYTE] = { 0 };
  int    entropy = 0;

  ASSERT( n != 0 );

  for (byte = data; byte < data + n; ++byte) {
    ++counters[*byte];
  }

  for (i = 0; i < (1 << BITS_PER_BYTE); ++i) {
    unsigned int arg = counters[i] * LOG2_ARG_MULTIPLIER / n;

    if (arg != 0) {
      entropy += counters[i] * log2(arg);
    }
  }

  entropy *= -1;
  entropy /= LOG2_RESULT_MULTIPLIER * n;

  return entropy;
}
