/**
 * @file   brain.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Oct 10 10:41:38 2010
 *
 * @brief  Provides initialization/cleanup facilities for all the FSMs.
 *
 *
 */
#ifndef _BRAIN__BRAIN_H_
#define _BRAIN__BRAIN_H_


/**
 * Initializes all the FSMs.
 *
 *
 * @return
 */
int
brain_init(void);


/**
 * Cleanups all the FSMs.
 *
 */
void
brain_cleanup(void);


#endif /* _BRAIN__BRAIN_H_ */
