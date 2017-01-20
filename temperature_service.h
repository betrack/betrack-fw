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
/* Read the Temperature level */
static int16 readTemperature(void);

/* Initialise the Temperature Service data structure.*/
extern void TemperatureDataInit(void);

/* Initialise the Temperature Service data structure at chip reset */
extern void TemperatureInitChipReset(void);

/* Read the Temperature Service specific data stored in NVM */
extern void TemperatureReadDataFromNVM(uint16 *p_offset);

/* Write the Temperature Service specific data to NVM */
extern void TemperatureWriteDataToNVM(uint16 *p_offset);

/* Monitor the temperature and trigger notifications (if configured) to the
 * connected host
 */
extern void TemperatureUpdate(uint16 ucid);

/* Notify bonding status to the Temperature Service */
extern void TemperatureBondingNotify(void);

#endif /* __TEMPERATURE_SERVICE_H__ */
