/******************************************************************************
 *    Copyright (c) 2015 Cambridge Silicon Radio Limited 
 *    All rights reserved.
 * 
 *    Redistribution and use in source and binary forms, with or without modification, 
 *    are permitted (subject to the limitations in the disclaimer below) provided that the
 *    following conditions are met:
 *
 *    Redistributions of source code must retain the above copyright notice, this list of 
 *    conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
 *    and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS" AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * FILE
 *     beaconing.c
 *
 * DESCRIPTION
 *     This file defines beaconing routines.
 *

 ****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *===========================================================================*/

#include <gatt.h>           /* GATT application interface */
#include <buf_utils.h>      /* Buffer functions */
#include <mem.h>            /* Memory routines */
#include <timer.h>
#include <gatt_prim.h>
#include <gatt_uuid.h>
#include <ls_app_if.h>
#include <gap_app_if.h>

/*============================================================================*
 *  Local Header Files
 *===========================================================================*/

#include "esurl_beacon.h" /* Interface to this file */
#include "esurl_beacon_service.h" /* Interface to this file */
#include "beaconing.h"      /* Beaconing routines */

/*=============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Advertisement payload size:
 *      31
 *      - 3 octets for mandatory Flags AD (added automatically by the firmware)
 *      - 1 octet for manufacturer specific AD length field (added by the firmware)
 */
#define ADVERT_SIZE                     (28)

/*============================================================================*
 *  Private data
 *============================================================================*/

/* Timer for the beaconing instance */
static timer_id     beacon_tid;

/*============================================================================*
 *  Private Function Prototypes
 *===========================================================================*/

/* Control beacon at timer expiry */
static void appBeaconTimerHandler(timer_id tid);
/* Beacon update data to LS adv storage */
static uint32 BeaconUpdateData(void);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      appBuzzerTimerHandler
 *
 *  DESCRIPTION
 *      This function is used to stop the Buzzer at the expiry of timer.
 *
 *  PARAMETERS
 *      tid [in]                ID of expired timer (unused)
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appBeaconTimerHandler(timer_id tid)
{
    if(tid == beacon_tid)
    {
        uint32 beacon_interval = BeaconUpdateData();
    
        /* Loop the beacon timer */
        beacon_tid = TimerCreate(beacon_interval, TRUE, appBeaconTimerHandler);
    }   
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BeaconUpdateData
 *
 *  DESCRIPTION
 *      This function is used to stop update the data to LS adv storage
 *
 *  PARAMETERS
 *      Nothing
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static uint32 BeaconUpdateData(void)
{
    uint8 advData[ADVERT_SIZE];
    uint16 offset = 0;
    uint8* beacon_name;
    uint8 beacon_name_size;
    uint8* beacon_data;
    uint8 beacon_data_size;
    uint8 i;
    uint8 len_i = 0;
    uint8 adv_parameter_len = 0;
    
    uint32 beacon_interval = EsurlBeaconGetPeriodMillis();
    
    /* clear the existing advertisement and scan response data */
    LsStoreAdvScanData(0, NULL, ad_src_advertise);
    LsStoreAdvScanData(0, NULL, ad_src_scan_rsp);

    /* set the advertisement interval */

    GapSetAdvInterval(beacon_interval, beacon_interval);
    
    /* get the beaconing name USING SERVICE */
    EsurlBeaconGetName(&beacon_name, &beacon_name_size);

    if(beacon_name_size > 0)
    {
        adv_parameter_len = beacon_name[0];
        len_i = adv_parameter_len - 1;
        
        /* and store in the packet */
        for(i = 1; (i < beacon_name_size) && (offset < ADVERT_SIZE); i++,offset++, len_i--)
        {
            advData[offset] = beacon_name[i];
            
            if(len_i == 0)
            {
                /* store the advertisement parameter and get length for the next parameter */
                LsStoreAdvScanData(adv_parameter_len, &advData[offset - adv_parameter_len + 1], ad_src_advertise);

                adv_parameter_len = beacon_name[i+1];
                len_i = adv_parameter_len;
                i++;
            }
        }
    }
    else
    {
        /* store the advertisement data */
        LsStoreAdvScanData(offset, advData, ad_src_advertise);
    }
    
    /* update the beaconing data */
    EsurlBeaconUpdateData();
    
    /* get the beaconing data USING SERVICE */
    EsurlBeaconGetData(&beacon_data, &beacon_data_size);
    
    if(beacon_data_size > 0)
    {
        adv_parameter_len = beacon_data[0];
        len_i = adv_parameter_len - 1;
        
        /* and store in the packet */
        for(i = 1; (i < beacon_data_size) && (offset < ADVERT_SIZE); i++,offset++, len_i--)
        {
            advData[offset] = beacon_data[i];
            
            if(len_i == 0)
            {
                /* store the advertisement parameter and get length for the next parameter */
                if(LsStoreAdvScanData(adv_parameter_len, &advData[offset - adv_parameter_len + 1], ad_src_advertise) != ls_err_none)
                {
                    ReportPanic(app_panic_set_advert_data);
                }    
                adv_parameter_len = beacon_data[i+1];
                len_i = adv_parameter_len;
                i++;
            }
        }
    }
    else
    {
        /* store the advertisement data */
        LsStoreAdvScanData(offset, advData, ad_src_advertise);
    }
    
    return beacon_interval;
}

/*============================================================================*
 *  Public Function Implementations
 *===========================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      BeaconInitData
 *
 *  DESCRIPTION
 *      This function initialises the beacon data to a known state.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BeaconDataInit(void)
{
    /* Initialise beacon timer */
    beacon_tid = TIMER_INVALID;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BeaconStart
 *
 *  DESCRIPTION
 *      This function is used to start or stop beaconing
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BeaconStart(bool start)
{    
    /* Stop broadcasting */
    LsStartStopAdvertise(FALSE, whitelist_disabled, ls_addr_type_random);
    
    /* Delete buzzer timer if running */
    if (beacon_tid != TIMER_INVALID)
    {
        TimerDelete(beacon_tid);
        beacon_tid = TIMER_INVALID;
    }
    
    /* beacon_interval of zero overrides and stops beaconning */
    if (start) 
    {
        /* prepare the advertisement packet */
   
        /* set the GAP Broadcaster role */
        GapSetMode(gap_role_broadcaster,
                   gap_mode_discover_no,
                   gap_mode_connect_no,
                   gap_mode_bond_no,
                   gap_mode_security_none);
        
        uint32 beacon_interval = BeaconUpdateData();
        
        /* Start broadcasting */
        LsStartStopAdvertise(TRUE, whitelist_disabled, ls_addr_type_random);
        
         /* Start the beacon timer */
        beacon_tid = TimerCreate(beacon_interval/2, TRUE, appBeaconTimerHandler);
    }
}
