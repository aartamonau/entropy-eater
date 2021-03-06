/**
 * @file   eater.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Sep 12 10:27:37 2010
 *
 * @brief  Wrapper library for communications with kernel space entropy eater.
 *
 *
 */

#ifndef _EATER_H_
#define _EATER_H_


#include <stdint.h>

#include "eater_interface.h"


enum {
  EATER_OK,                     /**< Status indicating that no errors
                                 * occurred during execution. */
  EATER_ERROR = -1,             /**< Error occurred during execution and @e
                                 * errno set accordingly. */
};


/**
 * Connects to entropy eater. All other calls must be performed after the
 * connection has been established.
 *
 *
 * @return execution status
 * @retval EATER_OK    connected successfully
 * @retval EATER_ERROR error occurred
 */
int
eater_connect(void);


/**
 * Disconnects from entropy eater. No calls should be performed after
 * connection has been closed.
 *
 */
void
eater_disconnect(void);


int
eater_cmd_hello(void);


/**
 * Feeds data to entropy eater.
 *
 * @param data  data to feed
 * @param count size of data
 *
 * @return
 */
int
eater_cmd_feed(uint8_t *data, size_t count);


/**
 * Sweeps eater's room.
 *
 *
 * @return
 */
int
eater_cmd_sweep(void);


/**
 * Disinfects eater's room.
 *
 *
 * @return
 */
int
eater_cmd_disinfect(void);


/**
 * Cures ill entropy eater.
 *
 *
 * @return
 */
int
eater_cmd_cure(void);


/**
 * Plays in rock-paper-scissors game with eater.
 *
 * @param sign sign chosen by user
 *
 * @return
 */
int
eater_cmd_play_rps(enum rps_sign_t sign);


#endif /* _EATER_H_ */
