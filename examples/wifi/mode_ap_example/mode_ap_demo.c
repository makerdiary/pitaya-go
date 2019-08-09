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
/** @file mode_ap_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 * to behave as an AP.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"
#include <string.h>

#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

#define MAIN_M2M_DEVICE_NAME                 "PITAYA_00:00"

/** Default AP Name */
static char m_default_device_name[] = MAIN_M2M_DEVICE_NAME;

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;

volatile static bool m_ap_enabled = false;

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
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
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nStation disconnected\r\n");
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
            nrf_cli_process(mp_curr_cli);

            bsp_board_led_off(LED_B_IDX);
        }

        break;
    }

    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nStation connected\r\n");
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Station IP is %u.%u.%u.%u\r\n",
                pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
        nrf_cli_process(mp_curr_cli);

        bsp_board_led_on(LED_B_IDX);

        break;
    }

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


/**@brief wifi enable_ap command implementation.
 */
static void cmd_wifi_enable_ap(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    if(argc > 3)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(m_ap_enabled)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "AP mode is already enabled\r\n");
        return;        
    }

    /* Initialize AP mode parameters structure with SSID, channel and OPEN security type. */
    memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));

    if(argc == 1)
    {
        /* No SSID specified. Use 'PITAYA_XX:XX' as default */
        m2m_wifi_get_mac_address(m_mac_addr);
        set_dev_name_to_mac((uint8_t *)m_default_device_name, m_mac_addr);
        strcpy((char *)&strM2MAPConfig.au8SSID, m_default_device_name);
    }
    else
    {
        strcpy((char *)&strM2MAPConfig.au8SSID, (char *)argv[1]);
    }

    strM2MAPConfig.u8ListenChannel = M2M_WIFI_CH_6;
    strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
    strM2MAPConfig.au8DHCPServerIP[0] = 192;
    strM2MAPConfig.au8DHCPServerIP[1] = 168;
    strM2MAPConfig.au8DHCPServerIP[2] = 1;
    strM2MAPConfig.au8DHCPServerIP[3] = 1;

    if(argc == 3)
    {
        strM2MAPConfig.u8SecType = M2M_WIFI_SEC_WPA_PSK;
        strcpy((char *)&strM2MAPConfig.au8Key, (char *)argv[2]);
        strM2MAPConfig.u8KeySz = strlen((char *)argv[2]);
    }

    /* Bring up AP mode with parameters structure. */
    ret = m2m_wifi_enable_ap(&strM2MAPConfig);
    if (M2M_SUCCESS != ret)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Fail to enable AP mode: %d\r\n", ret);
    }
    else
    {
        m_ap_enabled = true;
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "AP mode started. You can connect to %s.\r\n", (char *)&strM2MAPConfig.au8SSID);
    }

    mp_curr_cli = p_cli;
}


/**@brief wifi disable_ap command implementation.
 */
static void cmd_wifi_disable_ap(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    if(m2m_wifi_disable_ap() == M2M_SUCCESS)
    {
        m_ap_enabled = false;
    }

    mp_curr_cli = p_cli;
}



NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(enable_ap, NULL, "Enable AP mode. Usage: wifi enable_ap {NAME} {PWD}", cmd_wifi_enable_ap),
    NRF_CLI_CMD(disable_ap, NULL, "Disable AP mode", cmd_wifi_disable_ap),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);


