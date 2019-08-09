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
/** @file provision_http_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 *  to start Provision Mode.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"
#include <string.h>

#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

#define MAIN_M2M_DEVICE_NAME                 "PITAYA_00:00"
#define MAIN_HTTP_PROV_SERVER_DOMAIN_NAME    "iot.makerdiary.com"

/** Default Device Name */
static char m_default_device_name[] = MAIN_M2M_DEVICE_NAME;

static char m_http_prov_domain_name[] = MAIN_HTTP_PROV_SERVER_DOMAIN_NAME;

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
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
 *  - [M2M_WIFI_RESP_PROVISION_INFO](@ref M2M_WIFI_RESP_PROVISION_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type.
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) 
        {
            m2m_wifi_request_dhcp_client();
        } 
        else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nWi-Fi disconnected\r\n");
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
            nrf_cli_process(mp_curr_cli);
            
            bsp_board_led_off(LED_B_IDX);
        }

        break;
    }

    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nWi-Fi connected\r\n");
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Wi-Fi IP is %u.%u.%u.%u\r\n",
                pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
        nrf_cli_process(mp_curr_cli);

        bsp_board_led_on(LED_B_IDX);

        break;
    }

    case M2M_WIFI_RESP_PROVISION_INFO:
    {
        tstrM2MProvisionInfo *pstrProvInfo = (tstrM2MProvisionInfo *)pvMsg;
        if (pstrProvInfo->u8Status == M2M_SUCCESS)
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nProvision successfully. Connecting to %s...\r\n", pstrProvInfo->au8SSID);

            m2m_wifi_connect((char *)pstrProvInfo->au8SSID, strlen((char *)pstrProvInfo->au8SSID), pstrProvInfo->u8SecType,
                    pstrProvInfo->au8Password, M2M_WIFI_CH_ALL);
        } 
        else 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Provision failed\r\n");
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
            nrf_cli_process(mp_curr_cli);
        }
    }
    break;

    default:
    {
        break;
    }
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


#define HEX2ASCII(x) (((x) >= 10) ? (((x) - 10) + 'A') : ((x) + '0'))
static void set_dev_name_to_mac(uint8 *name, uint8 *mac_addr)
{
    /* Name must be in the format PITAYA_00:00 */
    uint16 len;

    len = m2m_strlen(name);
    if (len >= 5) {
        name[len - 1] = HEX2ASCII((mac_addr[5] >> 0) & 0x0f);
        name[len - 2] = HEX2ASCII((mac_addr[5] >> 4) & 0x0f);
        name[len - 4] = HEX2ASCII((mac_addr[4] >> 0) & 0x0f);
        name[len - 5] = HEX2ASCII((mac_addr[4] >> 4) & 0x0f);
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


/**@brief wifi start_provision command implementation.
 */
static void cmd_wifi_start_provision(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    tstrM2MAPConfig strM2MAPConfig;
    int8_t ret;

    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc != 1)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    /* Initialize AP mode parameters structure with SSID, channel and OPEN security type. */
    memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));

    m2m_wifi_get_mac_address(m_mac_addr);
    set_dev_name_to_mac((uint8_t *)m_default_device_name, m_mac_addr);
    m2m_wifi_set_device_name((uint8_t *)m_default_device_name, (uint8_t)m2m_strlen((uint8_t *)m_default_device_name));
    strcpy((char *)&strM2MAPConfig.au8SSID, m_default_device_name);
    strM2MAPConfig.u8ListenChannel = M2M_WIFI_CH_6;
    strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
    strM2MAPConfig.au8DHCPServerIP[0] = 192;
    strM2MAPConfig.au8DHCPServerIP[1] = 168;
    strM2MAPConfig.au8DHCPServerIP[2] = 1;
    strM2MAPConfig.au8DHCPServerIP[3] = 1;

    m2m_wifi_start_provision_mode((tstrM2MAPConfig *)&strM2MAPConfig, (char *)m_http_prov_domain_name, 1);

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Provision Mode started.\r\nConnect to [%s] via AP[%s] and fill up the page.\r\n", m_http_prov_domain_name, strM2MAPConfig.au8SSID);

    mp_curr_cli = p_cli;
}


/**@brief wifi stop_provision command implementation.
 */
static void cmd_wifi_stop_provision(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    if(m2m_wifi_stop_provision_mode() == M2M_SUCCESS)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Provision stopped\r\n");
    }

    mp_curr_cli = p_cli;
}



NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(start_provision, NULL, "Start provision mode", cmd_wifi_start_provision),
    NRF_CLI_CMD(stop_provision, NULL, "Stop provision mode", cmd_wifi_stop_provision),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);


