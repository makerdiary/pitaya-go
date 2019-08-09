/**
* Copyright (c) 2019 makerdiary
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* * Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
*   copyright notice, this list of conditions and the following
*   disclaimer in the documentation and/or other materials provided
*   with the distribution.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
/** @file ap_scan_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 * to explain how to scan AP as a station.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"

#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;


/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type.
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    static uint8 u8ScanResultIdx = 0;
    
    switch(u8MsgType)
    {
    case M2M_WIFI_RESP_SCAN_DONE:
        {
            tstrM2mScanDone *pstrInfo = (tstrM2mScanDone*)pvMsg;
            
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nNum of AP found %d\r\n",pstrInfo->u8NumofCh);
            if(pstrInfo->s8ScanState == M2M_SUCCESS)
            {
                u8ScanResultIdx = 0;
                if(pstrInfo->u8NumofCh >= 1)
                {
                    m2m_wifi_req_scan_result(u8ScanResultIdx);
                    u8ScanResultIdx ++;
                }
                else
                {
                    nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "No AP Found. Rescan....\r\n");
                    m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
                }
            }
            else
            {
                nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Scan fail with error <%d>\r\n", pstrInfo->s8ScanState);
                nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
                nrf_cli_process(mp_curr_cli);
            }
        }
        break;
    
    case M2M_WIFI_RESP_SCAN_RESULT:
        {
            tstrM2mWifiscanResult       *pstrScanResult =(tstrM2mWifiscanResult*)pvMsg;
            uint8                       u8NumFoundAPs = m2m_wifi_get_num_ap_found();
            
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, ">>%02d RI %d SEC %s CH %02d BSSID %02X:%02X:%02X:%02X:%02X:%02X SSID %s\r\n",
                pstrScanResult->u8index,pstrScanResult->s8rssi,
                pstrScanResult->u8AuthType,
                pstrScanResult->u8ch,
                pstrScanResult->au8BSSID[0], pstrScanResult->au8BSSID[1], pstrScanResult->au8BSSID[2],
                pstrScanResult->au8BSSID[3], pstrScanResult->au8BSSID[4], pstrScanResult->au8BSSID[5],
                pstrScanResult->au8SSID);
            
            if(u8ScanResultIdx < u8NumFoundAPs)
            {
                // Read the next scan result
                m2m_wifi_req_scan_result(u8ScanResultIdx);
                u8ScanResultIdx ++;
            }
            else
            {
                nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
                nrf_cli_process(mp_curr_cli);                
            }
        }
        break;
    default:
        break;
    }
}


/**@brief Function for Wi-Fi module initialization.
 */
void wifi_setup(void)
{
    tstrWifiInitParam param;
    int8_t ret;
    uint8_t u8IsMacAddrValid;

    /* Initialize the BSP. */
    nm_bsp_init();

    /* Initialize Wi-Fi parameters structure. */
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

    /* Initialize Wi-Fi driver with data and status callbacks. */
    param.pfAppWifiCb = wifi_cb;
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret) 
    {
        while (true) 
        {
            bsp_board_led_invert(1);
            nrf_delay_ms(200);
        }
    }

    /* Get MAC Address from OTP. */
    m2m_wifi_get_otp_mac_address(m_mac_addr, &u8IsMacAddrValid);
    if (!u8IsMacAddrValid) {
        /* Cannot found MAC Address from OTP. Set user define MAC address. */
        m_user_define_mac_address[3] = NRF_FICR->DEVICEID[0] & 0xFF;
        m_user_define_mac_address[4] = (NRF_FICR->DEVICEID[0] >> 8) & 0xFF;
        m_user_define_mac_address[5] = (NRF_FICR->DEVICEID[0] >> 16) & 0xFF;

        m2m_wifi_set_mac_address((uint8_t *)m_user_define_mac_address);
    }
}

/**@brief Function for processing the Wi-Fi events.
 */
void wifi_process(void)
{
    while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS)
    {
        // No implementation needed.
    }
}


/**@brief wifi command implementation.
 */
static void cmd_wifi(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "%s %s: command not found\r\n", argv[0], argv[1]);
}

/**@brief wifi scan command implementation.
 */
static void cmd_wifi_scan(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc > 1)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    /* Request scan. */
    m2m_wifi_request_scan(M2M_WIFI_CH_ALL);

    mp_curr_cli = p_cli;
}


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(scan, NULL, "Request the Wi-Fi module to scan for networks", cmd_wifi_scan),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);


