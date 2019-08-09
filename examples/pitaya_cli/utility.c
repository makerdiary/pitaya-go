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
/** @file utility.c
 * @brief This file contains the source code for the Out of Box Demo.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"
#include "low_power_pwm.h"
#include "nfc_t2t_lib.h"
#include "nfc_uri_msg.h"

#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
#include "socket/include/socket.h"

/** Using IP address. */
#define IPV4_BYTE(val, index)           ((val >> (index * 8)) & 0xFF)

const char * strSecType[M2M_WIFI_NUM_AUTH_TYPES]= {"Invalid", "Open", "WPA/WPA2 personal(PSK)", "WEP (40 or 104) OPEN OR SHARED", "WPA/WPA2 Enterprise.IEEE802.1x"};

const char * strPingErrCode[3] = {"Success", "Destination Unreachable", "Request Timeout"};

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;

/** Wi-Fi status variable. */
static bool m_wifi_connected = false;

static low_power_pwm_t low_power_pwm_r;
static low_power_pwm_t low_power_pwm_g;
static low_power_pwm_t low_power_pwm_b;

static bool m_nfc_activated = false;
/** @snippet [NFC URI usage_0] */
static const uint8_t m_nfc_url[] =
    {'m', 'a', 'k', 'e', 'r', 'd', 'i', 'a', 'r', 'y', '.', 'c', 'o', 'm'}; //URL "makerdiary.com"

uint8_t m_ndef_msg_buf[256];


/** @snippet [NFC URI usage_0] */
/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void * p_context, nfc_t2t_event_t event, const uint8_t * p_data, size_t data_length)
{
    (void)p_context;

    switch (event)
    {
        case NFC_T2T_EVENT_FIELD_ON:
            bsp_board_led_on(BSP_BOARD_LED_2);
            break;

        case NFC_T2T_EVENT_FIELD_OFF:
            bsp_board_led_off(BSP_BOARD_LED_2);
            break;

        default:
            break;
    }
}



/**
 * @brief Function to be called in timer interrupt.
 *
 * @param[in] p_context     General purpose pointer (unused).
 */
static void pwm_handler(void * p_context)
{
    return;
}

/**
 * @brief Function to initalize low_power_pwm instances.
 *
 */

static void pwm_init(void)
{
    uint32_t err_code;
    low_power_pwm_config_t low_power_pwm_config;

    APP_TIMER_DEF(lpp_timer_0);
    low_power_pwm_config.active_high    = false;
    low_power_pwm_config.period         = 255;
    low_power_pwm_config.bit_mask       = BSP_LED_0_MASK;
    low_power_pwm_config.p_timer_id     = &lpp_timer_0;
    low_power_pwm_config.p_port         = BSP_LED_0_PORT;

    err_code = low_power_pwm_init((&low_power_pwm_g), &low_power_pwm_config, pwm_handler);
    APP_ERROR_CHECK(err_code);
    nrf_gpio_port_dir_output_set(low_power_pwm_config.p_port, low_power_pwm_g.bit_mask);
    err_code = low_power_pwm_duty_set(&low_power_pwm_g, 0);
    APP_ERROR_CHECK(err_code);

    APP_TIMER_DEF(lpp_timer_1);
    low_power_pwm_config.active_high    = false;
    low_power_pwm_config.period         = 255;
    low_power_pwm_config.bit_mask       = BSP_LED_1_MASK;
    low_power_pwm_config.p_timer_id     = &lpp_timer_1;
    low_power_pwm_config.p_port         = BSP_LED_1_PORT;

    err_code = low_power_pwm_init((&low_power_pwm_r), &low_power_pwm_config, pwm_handler);
    APP_ERROR_CHECK(err_code);
    nrf_gpio_port_dir_output_set(low_power_pwm_config.p_port, low_power_pwm_r.bit_mask);
    err_code = low_power_pwm_duty_set(&low_power_pwm_r, 0);
    APP_ERROR_CHECK(err_code);

    APP_TIMER_DEF(lpp_timer_2);
    low_power_pwm_config.active_high    = false;
    low_power_pwm_config.period         = 255;
    low_power_pwm_config.bit_mask       = BSP_LED_2_MASK;
    low_power_pwm_config.p_timer_id     = &lpp_timer_2;
    low_power_pwm_config.p_port         = BSP_LED_2_PORT;

    err_code = low_power_pwm_init((&low_power_pwm_b), &low_power_pwm_config, pwm_handler);
    APP_ERROR_CHECK(err_code);
    nrf_gpio_port_dir_output_set(low_power_pwm_config.p_port, low_power_pwm_b.bit_mask);
    err_code = low_power_pwm_duty_set(&low_power_pwm_b, 0);
    APP_ERROR_CHECK(err_code);
}



/**
 * \brief  PING Callback to deliver the ping statistics for the sent ping triggered by calling 
 * m2m_ping_req.
 *
 * \param[in] u32IPAddr Destination IP.
 * \param[in] u32RTT    Round Trip Time.
 * \param[in] u8ErrorCode Ping error code. It may be one of:
 *  - PING_ERR_SUCCESS
 *  - PING_ERR_DEST_UNREACH
 *  - PING_ERR_TIMEOUT
*/
static void ping_cb(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode)
{
    nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nFrom %d.%d.%d.%d, RTT=%d, %s\r\n", 
        u32IPAddr&0xFF, (u32IPAddr>>8)&0xFF,  (u32IPAddr>>16)&0xFF, (u32IPAddr>>24)&0xFF, 
        u32RTT, strPingErrCode[u8ErrorCode]);
    nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
    nrf_cli_process(mp_curr_cli);
}

/**
 * \brief Callback function of IP address.
 *
 * \param[in] hostName Domain name.
 * \param[in] hostIp Server IP.
 *
 * \return None.
 */
static void resolve_cb(uint8_t *hostName, uint32_t hostIp)
{
    nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nPING %s [%d.%d.%d.%d]...", hostName, 
        (int)IPV4_BYTE(hostIp, 0), (int)IPV4_BYTE(hostIp, 1), (int)IPV4_BYTE(hostIp, 2), (int)IPV4_BYTE(hostIp, 3));

    // Send ping request to the given IP Address.
    if(m2m_ping_req(hostIp, 0, ping_cb) != M2M_SUCCESS)
    {
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "\r\nFail to ping %s\r\n", hostName);
    }

    nrf_cli_process(mp_curr_cli);
}



/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
 *  - [M2M_WIFI_RESP_CONN_INFO](@ref M2M_WIFI_RESP_CONN_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type.
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    static uint8 u8ScanResultIdx = 0;

    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nWi-Fi disconnected\r\n");
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Press <Enter> to continue...");
            nrf_cli_process(mp_curr_cli);

            m_wifi_connected = false;

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

        m_wifi_connected = true;

        bsp_board_led_on(LED_B_IDX);

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
        break;
    }

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

    /*Initialize low power PWM for all 3  channels of RGB LED*/
    pwm_init();

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

    /* Initialize Socket module */
    socketInit();
    registerSocketCallback(NULL, resolve_cb);
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


/**@brief ping command implementation.
 */
static void cmd_ping(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint32_t dst_host;

    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc > 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(!m_wifi_connected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Connect to the Wi-Fi first\r\n");
        return; 
    }

    mp_curr_cli = p_cli;

    dst_host = nmi_inet_addr(argv[1]);

    if(dst_host == 0) // May be a domain name
    {
        /* Obtain the IP Address by host name */
        gethostbyname((uint8_t *)argv[1]);
        return;
    }

    // Send ping request to the given IP Address.
    if(m2m_ping_req(dst_host, 0, ping_cb) != M2M_SUCCESS)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Fail to send ping request\r\n");
        return;
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "PING %s ...\r\n", argv[1]);
}

/**@brief led command implementation.
 */
static void cmd_led(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t duty_r, duty_g, duty_b;
    uint32_t err_code;

    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc != 4)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(low_power_pwm_r.pwm_state == NRFX_DRV_STATE_INITIALIZED)
    {
        err_code = low_power_pwm_start((&low_power_pwm_r), low_power_pwm_r.bit_mask);
        APP_ERROR_CHECK(err_code);
    }

    if(low_power_pwm_g.pwm_state == NRFX_DRV_STATE_INITIALIZED)
    {
        err_code = low_power_pwm_start((&low_power_pwm_g), low_power_pwm_g.bit_mask);
        APP_ERROR_CHECK(err_code);
    }

    if(low_power_pwm_b.pwm_state == NRFX_DRV_STATE_INITIALIZED)
    {
        err_code = low_power_pwm_start((&low_power_pwm_b), low_power_pwm_b.bit_mask);
        APP_ERROR_CHECK(err_code);
    }

    duty_r = atoi(argv[1]);
    duty_g = atoi(argv[2]);
    duty_b = atoi(argv[3]);

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "R:%d, G:%d, B:%d\r\n", duty_r, duty_g, duty_b);

    if(duty_r == 0 && duty_g == 0 && duty_b == 0)
    {
        err_code = low_power_pwm_stop(&low_power_pwm_r);
        APP_ERROR_CHECK(err_code); 
        err_code = low_power_pwm_stop(&low_power_pwm_g);
        APP_ERROR_CHECK(err_code);
        err_code = low_power_pwm_stop(&low_power_pwm_b);
        APP_ERROR_CHECK(err_code);

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Closed\r\n");
        return;
    }

    err_code = low_power_pwm_duty_set(&low_power_pwm_r, duty_r);
    APP_ERROR_CHECK(err_code);

    err_code = low_power_pwm_duty_set(&low_power_pwm_g, duty_g);
    APP_ERROR_CHECK(err_code);

    err_code = low_power_pwm_duty_set(&low_power_pwm_b, duty_b);
    APP_ERROR_CHECK(err_code);

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Done\r\n");
}

/**@brief temp command implementation.
 */
static void cmd_temp(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint32_t err_code;
    int32_t temp;

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

    if(sd_temp_get(&temp) == NRF_SUCCESS)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, ">> Chip temperature: ");
        /* The value from sd_temp_get() is in units of 0.25 degrees Celsius */
        nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "%d degreeC\r\n", temp >> 2); 
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Fail to get temperature\r\n");
    }
}

static void cmd_nfc(nrf_cli_t const * p_cli, size_t argc, char **argv)
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


static void cmd_nfc_start(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ret_code_t err_code;

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

    if(m_nfc_activated)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "NFC: Activated\r\n");
        return;
    }

    /* Set up NFC */
    err_code = nfc_t2t_setup(nfc_callback, NULL);
    APP_ERROR_CHECK(err_code);

    /** @snippet [NFC URI usage_1] */
    /* Provide information about available buffer size to encoding function */
    uint32_t len = sizeof(m_ndef_msg_buf);

    /* Encode URI message into buffer */
    err_code = nfc_uri_msg_encode( NFC_URI_HTTPS,
                                   m_nfc_url,
                                   sizeof(m_nfc_url),
                                   m_ndef_msg_buf,
                                   &len);

    APP_ERROR_CHECK(err_code);
    /** @snippet [NFC URI usage_1] */

    /* Set created message as the NFC payload */
    err_code = nfc_t2t_payload_set(m_ndef_msg_buf, len);
    APP_ERROR_CHECK(err_code);

    /* Start sensing NFC field */
    err_code = nfc_t2t_emulation_start();
    APP_ERROR_CHECK(err_code);

    m_nfc_activated = true;

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Done\r\n");
}

static void cmd_nfc_stop(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ret_code_t err_code;

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

    if(!m_nfc_activated)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "NFC: Deactivated\r\n");
        return;
    }

    /* Start sensing NFC field */
    err_code = nfc_t2t_emulation_stop();
    APP_ERROR_CHECK(err_code);

    err_code = nfc_t2t_done();
    APP_ERROR_CHECK(err_code);

    m_nfc_activated = false;

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Done\r\n");
}

static void cmd_nfc_status(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
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

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "NFC: %s\r\n", m_nfc_activated? "Activated" : "Deactivated");
}


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(connect, NULL, "Connect to an AP. Usage: wifi connect {SSID} {PSK}", cmd_wifi_connect),
    NRF_CLI_CMD(disconnect, NULL, "Disconnect from the AP", cmd_wifi_disconnect),
    NRF_CLI_CMD(status, NULL, "Display Wi-Fi connection status", cmd_wifi_status),
    NRF_CLI_CMD(scan, NULL, "Scan the nearby Wi-Fi APs", cmd_wifi_scan),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);

NRF_CLI_CMD_REGISTER(ping, NULL, "Ping the given IP or domain", cmd_ping); 

NRF_CLI_CMD_REGISTER(led, NULL, "Set RGB LED color(0~255). Usage: led {R} {G} {B}", cmd_led);

NRF_CLI_CMD_REGISTER(temp, NULL, "Get the Temperature.", cmd_temp);


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_nfc)
{
    NRF_CLI_CMD(start, NULL, "Activate the NFC frontend", cmd_nfc_start),
    NRF_CLI_CMD(stop, NULL, "Deactivate the NFC frontend", cmd_nfc_stop),
    NRF_CLI_CMD(status, NULL, "Show the NFC frontend status", cmd_nfc_status),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(nfc, &m_sub_nfc, "Commands for NFC access", cmd_nfc);

