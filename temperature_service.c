/******************************************************************************
 * FILE
 *     Temperature_service.c
 *
 * DESCRIPTION
 *     This file defines routines for using the Temperature Service.
 *
 
 ****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *===========================================================================*/

#include <gatt.h>           /* GATT application interface */
#include <thermometer.h>    /* Read the temperature */
#include <buf_utils.h>      /* Buffer functions */

/*============================================================================*
 *  Local Header Files
 *===========================================================================*/

#include "esurl_beacon.h"    /* Definitions used throughout the GATT server */
#include "temperature_service.h"	/* Interface to this file */
#include "nvm_access.h"     /* Non-volatile memory access */
#include "app_gatt_db.h"    /* GATT database definitions */

/*============================================================================*
 *  Private Data Types
 *===========================================================================*/

/* Temperature Service data type */
typedef struct _TEMP_DATA_T
{
    /* Temperature Level in percent */
    uint8   temp;

    /* Client configuration descriptor for Temperature Level characteristic */
    gatt_client_config temp_client_config;

    /* NVM Offset at which Temperature data is stored */
    uint16 nvm_offset;

} TEMP_DATA_T;

/*============================================================================*
 *  Private Data
 *===========================================================================*/

/* Temperature Service data instance */
static TEMP_DATA_T g_temp_data;

/*============================================================================*
 *  Private Definitions
 *===========================================================================*/

/* Number of words of NVM memory used by Temperature Service */
#define TEMPERATURE_SERVICE_NVM_MEMORY_WORDS              (1)

/* The offset of data being stored in NVM for the Temperature Service. This offset
 * is added to the Temperature Service offset in the NVM region (see
 * g_temp_data.nvm_offset) to get the absolute offset at which this data is
 * stored in NVM.
 */
#define TEMPERATURE_NVM_CLIENT_CONFIG_OFFSET        (0)

/*============================================================================*
 *  Private Function Prototypes
 *===========================================================================*/

/* Read the Temperature level */
static int16 readTemperature(void);

/*============================================================================*
 *  Private Function Implementations
 *===========================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      readTemperature
 *
 *  DESCRIPTION
 *      This function reads the temperature.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Temperature
 *----------------------------------------------------------------------------*/
static int16 readTemperature(void)
{
    /* Return the Temperature */
    return ThermometerReadTemperature();
}

/*============================================================================*
 *  Public Function Implementations
 *===========================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureDataInit
 *
 *  DESCRIPTION
 *      This function is used to initialise the Temperature Service data structure.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureDataInit(void)
{
    if(!IsDeviceBonded())
    {
        /* Initialise Temperature level client configuration characterisitic
         * descriptor value only if device is not bonded
         */
        g_temp_data.temp_client_config = gatt_client_config_none;
    }
    readTemperature();
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureInitChipReset
 *
 *  DESCRIPTION
 *      This function is used to initialise the Temperature Service data structure
 *      at chip reset.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureInitChipReset(void)
{
    /* Initialise Temperature level to 0 percent so that the Temperature level 
     * notification (if configured) is sent when the value is read for 
     * the first time after power cycle.
     */
    g_temp_data.temp = 0;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureReadDataFromNVM
 *
 *  DESCRIPTION
 *      This function is used to read Temperature Service specific data stored in 
 *      NVM.
 *
 *  PARAMETERS
 *      p_offset [in]           Offset to Temperature Service data in NVM
 *               [out]          Offset to next entry in NVM
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureReadDataFromNVM(uint16 *p_offset)
{

    g_temp_data.nvm_offset = *p_offset;

    /* Read NVM only if devices are bonded */
    if(IsDeviceBonded())
    {
        /* Read Temperature level client configuration descriptor */
        Nvm_Read((uint16*)&g_temp_data.temp_client_config,
                sizeof(g_temp_data.temp_client_config),
                *p_offset + 
                TEMPERATURE_NVM_CLIENT_CONFIG_OFFSET);
    }

    /* Increment the offset by the number of words of NVM memory required 
     * by the Temperature Service 
     */
    *p_offset += TEMPERATURE_SERVICE_NVM_MEMORY_WORDS;

}
/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureWriteDataToNVM
 *
 *  DESCRIPTION
 *      This function is used to writes Temperature Service specific data to 
 *      NVM.
 *
 *  PARAMETERS
 *      p_offset [in]           Offset to Beacon Service data in NVM
 *               [out]          Offset to next entry in NVM
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureWriteDataToNVM(uint16 *p_offset)
{

    g_temp_data.nvm_offset = *p_offset;
    
    /* Increment the offset by the number of words of NVM memory required 
     * by the Beacon Service 
     */
    *p_offset += TEMPERATURE_SERVICE_NVM_MEMORY_WORDS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureBondingNotify
 *
 *  DESCRIPTION
 *      This function is used by application to notify bonding status to the
 *      Temperature Service.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureBondingNotify(void)
{

    /* Write data to NVM if bond is established */
    if(IsDeviceBonded())
    {
        /* Write to NVM the client configuration value of Temperature level 
         * that was configured prior to bonding 
         */
        Nvm_Write((uint16*)&g_temp_data.temp_client_config, 
                  sizeof(g_temp_data.temp_client_config), 
                  g_temp_data.nvm_offset + 
                  TEMPERATURE_NVM_CLIENT_CONFIG_OFFSET);
    }

}
