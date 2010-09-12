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


#endif /* _EATER_H_ */
