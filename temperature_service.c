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
 *      TemperatureHandleAccessRead
 *
 *  DESCRIPTION
 *      This function handles read operations on Temperature Service attributes
 *      maintained by the application and responds with the GATT_ACCESS_RSP
 *      message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureHandleAccessRead(GATT_ACCESS_IND_T *p_ind)
{
    uint16 length = 0;                  /* Length of attribute data, octets */
    uint8  value[2];                    /* Attribute value */
    uint8 *p_val = NULL;                /* Pointer to attribute value */
    sys_status rc = sys_status_success; /* Function status */

    switch(p_ind->handle)
    {

        case HANDLE_TEMP:
        {
            /* Read the Temperature level */
            length = 1; /* One Octet */

            g_temp_data.temp = readTemperature();

            value[0] = g_temp_data.temp;
        }
        break;

        case HANDLE_TEMP_C_CFG:
        {
            /* Read the client configuration descriptor for the Temperature level
             * characteristic.
             */
            length = 2; /* Two Octets */
            p_val = value;

            BufWriteUint16((uint8 **)&p_val, g_temp_data.temp_client_config);
        }
        break;

        default:
            /* No more IRQ characteristics */
            rc = gatt_status_read_not_permitted;
        break;

    }

    /* Send ACCESS RESPONSE */
    GattAccessRsp(p_ind->cid, p_ind->handle, rc, length, value);

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleAccessWrite
 *
 *  DESCRIPTION
 *      This function handles write operations on Temperature Service attributes
 *      maintained by the application and responds with the GATT_ACCESS_RSP
 *      message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureHandleAccessWrite(GATT_ACCESS_IND_T *p_ind)
{
    uint8 *p_value = p_ind->value;      /* New attribute value */
    uint16 client_config;               /* Client configuration descriptor */
    sys_status rc = sys_status_success; /* Function status */

    switch(p_ind->handle)
    {
        case HANDLE_TEMP_C_CFG:
        {
            /* Write the client configuration descriptor for the Temperature level
             * characteristic.
             */
            client_config = BufReadUint16(&p_value);

            /* Only notifications are allowed for this client configuration 
             * descriptor.
             */
            if((client_config == gatt_client_config_notification) ||
               (client_config == gatt_client_config_none))
            {
                g_temp_data.temp_client_config = client_config;

                /* Write Temperature level client configuration to NVM if the 
                 * device is bonded.
                 */
                if(IsDeviceBonded())
                {
                     Nvm_Write(&client_config,
                              sizeof(client_config),
                              g_temp_data.nvm_offset + 
                              TEMPERATURE_NVM_CLIENT_CONFIG_OFFSET);
                }
            }
            else
            {
                /* INDICATION or RESERVED */

                /* Return error as only notifications are supported */
                rc = gatt_status_app_mask;
            }

        }
        break;


        default:
            rc = gatt_status_write_not_permitted;
        break;

    }

    /* Send ACCESS RESPONSE */
    GattAccessRsp(p_ind->cid, p_ind->handle, rc, 0, NULL);

    /* Send an update as soon as notifications are configured */
    if(g_temp_data.temp_client_config == gatt_client_config_notification)
    {
        /* Reset current Temperature level to an invalid value so that it 
         * triggers notifications on reading the current Temperature level 
         */
        g_temp_data.temp = 0xFF; /* 0 to 100: Valid value range */

        /* Update the Temperature level and send notification. */
        TemperatureUpdateLevel(p_ind->cid);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      TemperatureUpdateLevel
 *
 *  DESCRIPTION
 *      This function is to monitor the Temperature level and trigger notifications
 *      (if configured) to the connected host.
 *
 *  PARAMETERS
 *      ucid [in]               Connection ID of the host
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void TemperatureUpdate(uint16 ucid)
{
    int16 old_vbat;                     /* Previous Temperature */
    int16 cur_bat_level;                /* Current Temperature */

    /* Read the Temperature level */
    cur_temp = readTemperature();

    old_temp = g_temp_data.temp;

    /* If the current and old Temperature level are not same, update the  connected
     * host if notifications are configured.
     */
    if(old_temp != cur_temp)
    {

        if((ucid != GATT_INVALID_UCID) &&
           (g_temp_data.temp_client_config == gatt_client_config_notification))
        {

            GattCharValueNotification(ucid, 
                                      HANDLE_TEMP, 
                                      1, &cur_temp);

            /* Update Temperature Level characteristic in database */
            g_temp_data.temp = cur_temp;

        }
    }
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
 *      TemperatureCheckHandleRange
 *
 *  DESCRIPTION
 *      This function is used to check if the handle belongs to the Temperature 
 *      Service.
 *
 *  PARAMETERS
 *      handle [in]             Handle to check
 *
 *  RETURNS
 *      TRUE if handle belongs to the Temperature Service, FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool TemperatureCheckHandleRange(uint16 handle)
{
    return ((handle >= HANDLE_TEMPERATURE_SERVICE) &&
            (handle <= HANDLE_TEMPERATURE_SERVICE_END))
            ? TRUE : FALSE;
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
