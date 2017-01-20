/******************************************************************************
 *  FILE
 *      temperature_service.h
 *
 *  DESCRIPTION
 *      Header definitions for the Temperature Service
 *
 *
 *****************************************************************************/

#ifndef __TEMPERATURE_SERVICE_H__
#define __TEMPERATURE_SERVICE_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>          /* Commonly used type definitions */
#include <gatt.h>           /* GATT application interface */

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* Initialise the Temperature Service data structure.*/
extern void TemperatureDataInit(void);

/* Initialise the Temperature Service data structure at chip reset */
extern void TemperatureInitChipReset(void);

/* Handle read operations on Temperature Service attributes maintained by the
 * application
 */
extern void TemperatureHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* Handle write operations on Temperature Service attributes maintained by the
 * application
 */
extern void TemperatureHandleAccessWrite(GATT_ACCESS_IND_T *p_ind);

/* Monitor the Temperature level and trigger notifications (if configured) to the
 * connected host
 */
extern void TemperatureUpdateLevel(uint16 ucid);

/* Read the Temperature Service specific data stored in NVM */
extern void TemperatureReadDataFromNVM(uint16 *p_offset);

/* Write the Temperature Service specific data to NVM */
extern void TemperatureWriteDataToNVM(uint16 *p_offset);

/* Check if the handle belongs to the Temperature Service */
extern bool TemperatureCheckHandleRange(uint16 handle);

/* Notify bonding status to the Temperature Service */
extern void TemperatureBondingNotify(void);

#endif /* __TEMPERATURE_SERVICE_H__ */
