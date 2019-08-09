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
/** @file power_save_mode_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 * to check the PS (Power Save) mode.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"

#include "driver/include/m2m_periph.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

/** PowerSave status */
#define MAIN_PS_WAKE                0
#define MAIN_PS_SLEEP               1
#define MAIN_PS_REQ_SLEEP           3

const char * strSecType[M2M_WIFI_NUM_AUTH_TYPES]= {"Invalid", "Open", "WPA/WPA2 personal(PSK)", "WEP (40 or 104) OPEN OR SHARED", "WPA/WPA2 Enterprise.IEEE802.1x"};

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;

static uint8_t m_sleep_status = MAIN_PS_WAKE;
static uint8_t m_sleep_mode = M2M_PS_MANUAL;            /**< Power save mode. Manual as default. */
static uint16_t m_sleep_interval = 1000;                /**< Sleep interval in ms. 1s as default. */
 

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CONN_INFO](@ref M2M_WIFI_RESP_CONN_INFO)
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
        m_sleep_status = MAIN_PS_WAKE;
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
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

        m_sleep_status = MAIN_PS_REQ_SLEEP;

        break;
    }

    case M2M_WIFI_RESP_CONN_INFO:
    {
        tstrM2MConnInfo *pstrConnInfo = (tstrM2MConnInfo*)pvMsg;
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nCONNECTED AP INFO\r\n");
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "SSID                : %s\r\n", pstrConnInfo->acSSID);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "SEC TYPE            : %s\r\n", strSecType[pstrConnInfo->u8SecType]);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Signal Strength     : %d\r\n", pstrConnInfo->s8RSSI); 
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "RF Channel          : %d\r\n", pstrConnInfo->u8CurrChannel);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Local IP Address    : %d.%d.%d.%d\r\n", 
                    pstrConnInfo->au8IPAddr[0] , pstrConnInfo->au8IPAddr[1], pstrConnInfo->au8IPAddr[2], pstrConnInfo->au8IPAddr[3]);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "MAC Address         : %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
                    pstrConnInfo->au8MACAddress[0] , pstrConnInfo->au8MACAddress[1], pstrConnInfo->au8MACAddress[2], pstrConnInfo->au8MACAddress[3], pstrConnInfo->au8MACAddress[4], pstrConnInfo->au8MACAddress[5]);
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
        nrf_cli_process(mp_curr_cli);

        m_sleep_status = MAIN_PS_REQ_SLEEP;

        break;
    }

    default:
    {
        break;
    }
    }
}


static void disable_pullups(void)
{
    uint32 pinmask;

    pinmask = (
    M2M_PERIPH_PULLUP_DIS_HOST_WAKEUP|
    M2M_PERIPH_PULLUP_DIS_GPIO_3|
    M2M_PERIPH_PULLUP_DIS_GPIO_5|
    M2M_PERIPH_PULLUP_DIS_SD_DAT0_SPI_TXD|
    M2M_PERIPH_PULLUP_DIS_GPIO_6);

    m2m_periph_pullup_ctrl(pinmask, 0);
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

    /* Disable pullups */
    disable_pullups();

    /* Set default sleep mode */
    if (m_sleep_mode == M2M_PS_MANUAL)
    {
        m2m_wifi_set_sleep_mode(m_sleep_mode, 1);
    } 
    else if (m_sleep_mode == M2M_PS_DEEP_AUTOMATIC) 
    {
        tstrM2mLsnInt strM2mLsnInt;
        m2m_wifi_set_sleep_mode(m_sleep_mode, 1);
        strM2mLsnInt.u16LsnInt = M2M_LISTEN_INTERVAL;
        m2m_wifi_set_lsn_int(&strM2mLsnInt);
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

/**@brief wifi sleep command implementation.
 */
static void cmd_wifi_sleep(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
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

    mp_curr_cli = p_cli;

    if(argc == 1)
    {
        /* Request sleep mode */
        if(m_sleep_status == MAIN_PS_REQ_SLEEP)
        {
            if(m_sleep_mode == M2M_PS_MANUAL)
            {
                m2m_wifi_request_sleep(m_sleep_interval);
                m_sleep_status = MAIN_PS_SLEEP;
            }
        }
        return;
    }

    if(!strcmp(argv[1], "manual"))
    {
        m_sleep_mode = M2M_PS_MANUAL;
    }
    else if(!strcmp(argv[1], "auto"))
    {
        m_sleep_mode = M2M_PS_DEEP_AUTOMATIC;
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown power mode. Set 'manual' or 'auto'\r\n");
        return;        
    }

    if(argc == 3)
    {
        m_sleep_interval = atoi(argv[2]);  // in ms
        if(m_sleep_mode == M2M_PS_DEEP_AUTOMATIC)
        {
            m_sleep_interval = m_sleep_interval / 100; // in 100(ms)
        }
    }

    tstrM2mLsnInt strM2mLsnInt;
    m2m_wifi_set_sleep_mode(m_sleep_mode, 1);
    strM2mLsnInt.u16LsnInt = m_sleep_interval;
    m2m_wifi_set_lsn_int(&strM2mLsnInt);
}

/**@brief wifi connect command implementation.
 */
static void cmd_wifi_connect(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t sec_type = M2M_WIFI_SEC_OPEN;
    char * p_auth_info = NULL;

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

    mp_curr_cli = p_cli;

    if(argc == 1)
    {
        /* Attempt to reconnect to the last-associated AP */
        m2m_wifi_default_connect();
        return;
    }

    if(argc == 3)
    {
        sec_type = M2M_WIFI_SEC_WPA_PSK;
        p_auth_info = argv[2];
    }

    /* Connect to the specified AP. */
    m2m_wifi_connect((char *)argv[1], strlen(argv[1]), sec_type, (void *)p_auth_info, M2M_WIFI_CH_ALL);
}


/**@brief wifi disconnect command implementation.
 */
static void cmd_wifi_disconnect(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    m2m_wifi_disconnect();

    mp_curr_cli = p_cli;
}

/**@brief wifi status command implementation.
 */
static void cmd_wifi_status(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    // Get the current AP information.
    if(m2m_wifi_get_connection_info() != M2M_SUCCESS)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Fail to get the current AP information\r\n");
    }

    mp_curr_cli = p_cli;
}


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(sleep, NULL, "Enter sleep mode. Usage: wifi sleep {manual,auto} {interval}", cmd_wifi_sleep),
    NRF_CLI_CMD(connect, NULL, "Connect to an AP. Usage: wifi connect {SSID} {PSK}", cmd_wifi_connect),
    NRF_CLI_CMD(disconnect, NULL, "Disconnect from the AP", cmd_wifi_disconnect),
    NRF_CLI_CMD(status, NULL, "Display Wi-Fi connection status", cmd_wifi_status),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);

