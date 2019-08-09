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
/** @file simple_tcp_server_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 * to test TCP server.
 *
 */

#include "ctype.h"
#include "string.h"

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"

#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
#include "socket/include/socket.h"

/** Using IP address. */
#define IPV4_BYTE(val, index)           ((val >> (index * 8)) & 0xFF)

#define MAIN_WIFI_M2M_BUFFER_SIZE    1460

const char * strSecType[M2M_WIFI_NUM_AUTH_TYPES]= {"Invalid", "Open", "WPA/WPA2 personal(PSK)", "WEP (40 or 104) OPEN OR SHARED", "WPA/WPA2 Enterprise.IEEE802.1x"};

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;

/** Wi-Fi connection state */
static uint8_t m_wifi_connected = M2M_WIFI_DISCONNECTED;

/** Socket for client */
static SOCKET m_tcp_client_socket = -1;

/** Socket for server */
static SOCKET m_tcp_server_socket = -1;

/** Socket address structure */
static struct sockaddr_in m_addr;

/** Test buffer */
static uint8_t m_rx_socket_buffer[MAIN_WIFI_M2M_BUFFER_SIZE] = {0};



/**
 * \brief Callback to get the Data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
    switch (u8Msg) {
    /* Socket bind */
    case SOCKET_MSG_BIND:
    {
        tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
        if (pstrBind && pstrBind->status == 0) 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nBind success\r\n");
            listen(m_tcp_server_socket, 0);
        } 
        else 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "\r\nBind error\r\n");
            close(m_tcp_server_socket);
            m_tcp_server_socket = -1;
        }
    }
    break;

    /* Socket listen */
    case SOCKET_MSG_LISTEN:
    {
        tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg *)pvMsg;
        if (pstrListen && pstrListen->status == 0) 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Listening...\r\n");
            accept(m_tcp_server_socket, NULL, NULL);
        } 
        else 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Listen error\r\n");
            close(m_tcp_server_socket);
            m_tcp_server_socket = -1;
        }
    }
    break;

    /* Connect accept */
    case SOCKET_MSG_ACCEPT:
    {
        tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
        if (pstrAccept) 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nAccept success\r\n");
            accept(m_tcp_server_socket, NULL, NULL);
            m_tcp_client_socket = pstrAccept->sock;
            recv(m_tcp_client_socket, m_rx_socket_buffer, sizeof(m_rx_socket_buffer), 0);
        } 
        else
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Accept error\r\n");
            close(m_tcp_server_socket);
            m_tcp_server_socket = -1;
        }
    }
    break;

    /* Message send */
    case SOCKET_MSG_SEND:
    {
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "\r\nSend success\r\n");
    }
    break;

    /* Message receive */
    case SOCKET_MSG_RECV:
    {
        tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
        if (pstrRecv && pstrRecv->s16BufferSize > 0) 
        {
            pstrRecv->pu8Buffer[pstrRecv->s16BufferSize] = '\0';
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Received: %s\r\n", pstrRecv->pu8Buffer);
        } 
        else 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Receive error\r\n");
            close(m_tcp_server_socket);
            m_tcp_server_socket = -1;
        }
    }

    break;

    default:
        break;
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

            m_wifi_connected = M2M_WIFI_DISCONNECTED;

            close(m_tcp_server_socket);
            m_tcp_server_socket = -1;

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

        m_addr.sin_addr.s_addr = *((uint32_t *)pu8IPAddress);

        m_wifi_connected = M2M_WIFI_CONNECTED;

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

    /* Initialize socket module */
    socketInit();
    registerSocketCallback(socket_cb, NULL);
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

/**@brief tcp command implementation.
 */
static void cmd_tcp(nrf_cli_t const * p_cli, size_t argc, char **argv)
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


/**@brief tcp bind command implementation.
 */
static void cmd_tcp_bind(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc != 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(m_wifi_connected == M2M_WIFI_DISCONNECTED)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Connect to the Wi-Fi first\r\n");
        return;        
    }

    if(m_tcp_server_socket < 0)
    {
        if((m_tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Failed to create TCP server socket\r\n");
            return;
        }
    }

    /* Initialize socket address structure. */
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = _htons(atoi(argv[1]));

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Binding to %d.%d.%d.%d:%s\r\n", 
        (int)IPV4_BYTE(m_addr.sin_addr.s_addr, 0), (int)IPV4_BYTE(m_addr.sin_addr.s_addr, 1),
        (int)IPV4_BYTE(m_addr.sin_addr.s_addr, 2), (int)IPV4_BYTE(m_addr.sin_addr.s_addr, 3),
        argv[1]);

    /* Bind the client */
    bind(m_tcp_server_socket, (struct sockaddr *)&m_addr, sizeof(struct sockaddr_in));

    mp_curr_cli = p_cli;
}

/**@brief tcp send command implementation.
 */
static void cmd_tcp_send(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc != 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(m_tcp_client_socket < 0)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "No client connected\r\n");
        return;
    }

    send(m_tcp_client_socket, argv[1], strlen(argv[1])+1, 0);

    mp_curr_cli = p_cli;
}

/**@brief tcp disconnect command implementation.
 */
static void cmd_tcp_disconnect(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    if(m_tcp_server_socket < 0)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Disconnected\r\n");
        return;
    }

    close(m_tcp_server_socket);
    m_tcp_server_socket = -1;
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Done\r\n");

    mp_curr_cli = p_cli;
}


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_wifi)
{
    NRF_CLI_CMD(connect, NULL, "Connect to an AP. Usage: wifi connect {SSID} {PSK}", cmd_wifi_connect),
    NRF_CLI_CMD(disconnect, NULL, "Disconnect from the AP", cmd_wifi_disconnect),
    NRF_CLI_CMD(status, NULL, "Display Wi-Fi connection status", cmd_wifi_status),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(wifi, &m_sub_wifi, "Commands for Wi-Fi access", cmd_wifi);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_tcp)
{
    NRF_CLI_CMD(bind, NULL, "Bind the TCP Socket. Usage: tcp bind [PORT]", cmd_tcp_bind),
    NRF_CLI_CMD(send, NULL, "Send a message. Usage: tcp send [MSG]", cmd_tcp_send),
    NRF_CLI_CMD(disconnect, NULL, "Disconnect the TCP Socket", cmd_tcp_disconnect),
    NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(tcp, &m_sub_tcp, "Commands for TCP operations", cmd_tcp);


