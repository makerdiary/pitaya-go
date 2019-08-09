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
/** @file mqtt_client_demo.c
 * @brief This example demonstrates the use of the Pitaya Go board
 * to implement a simple MQTT client.
 *
 */

#include "nrf_cli.h"
#include "nrf_delay.h"
#include "boards.h"

#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
#include "MQTTClient/Wrapper/mqtt.h"
#include "socket/include/socket.h"

/* Max size of MQTT buffer. */
#define MQTT_MAX_BUFFER_SIZE    512

#define MQTT_MAX_USER_ID_LEN    33
#define MQTT_MAX_PASSWORD_LEN   65
#define MQTT_MAX_CLIENT_ID_LEN  13

const char * strSecType[M2M_WIFI_NUM_AUTH_TYPES]= {"Invalid", "Open", "WPA/WPA2 personal(PSK)", "WEP (40 or 104) OPEN OR SHARED", "WPA/WPA2 Enterprise.IEEE802.1x"};

/** Mac address information. */
static uint8_t m_mac_addr[M2M_MAC_ADDRES_LEN];

/** User define MAC Address. */
static char m_user_define_mac_address[] = {0xf8, 0xf0, 0x05, 0x00, 0x00, 0x00};

static nrf_cli_t const * mp_curr_cli = NULL;

/** Wi-Fi status variable. */
static bool m_wifi_connected = false;

/* Instance of MQTT service. */
static struct mqtt_module m_mqtt_inst;

/* Receive buffer of the MQTT service. */
static unsigned char m_mqtt_read_buffer[MQTT_MAX_BUFFER_SIZE];
static unsigned char m_mqtt_send_buffer[MQTT_MAX_BUFFER_SIZE];

static char m_mqtt_user_id[MQTT_MAX_USER_ID_LEN];
static char m_mqtt_password[MQTT_MAX_PASSWORD_LEN];
static uint8_t m_mqtt_client_id[MQTT_MAX_CLIENT_ID_LEN];


/**@brief Function for generating the MQTT Client ID.
 *
 * @param[in]  p_client_id  The client id string. Must be terminated with \0.
 */

static uint8_t mqtt_client_id_generate(uint8_t * p_client_id) 
{

    const uint16_t serial_num_high_bytes = (uint16_t)NRF_FICR->DEVICEADDR[1] | 0xC000; // The masking makes the address match the Random Static BLE address.
    const uint32_t serial_num_low_bytes  = NRF_FICR->DEVICEADDR[0];

    (void)snprintf((char *)p_client_id,
                   MQTT_MAX_CLIENT_ID_LEN,
                   "%04"PRIx16"%08"PRIx32,
                   serial_num_high_bytes,
                   serial_num_low_bytes);  

    return MQTT_MAX_CLIENT_ID_LEN - 1;
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

    default:
    {
        break;
    }
    }
}

/**
 * \brief Callback to get the Socket event.
 *
 * \param[in] Socket descriptor.
 * \param[in] msg_type type of Socket notification. Possible types are:
 *  - [SOCKET_MSG_CONNECT](@ref SOCKET_MSG_CONNECT)
 *  - [SOCKET_MSG_BIND](@ref SOCKET_MSG_BIND)
 *  - [SOCKET_MSG_LISTEN](@ref SOCKET_MSG_LISTEN)
 *  - [SOCKET_MSG_ACCEPT](@ref SOCKET_MSG_ACCEPT)
 *  - [SOCKET_MSG_RECV](@ref SOCKET_MSG_RECV)
 *  - [SOCKET_MSG_SEND](@ref SOCKET_MSG_SEND)
 *  - [SOCKET_MSG_SENDTO](@ref SOCKET_MSG_SENDTO)
 *  - [SOCKET_MSG_RECVFROM](@ref SOCKET_MSG_RECVFROM)
 * \param[in] msg_data A structure contains notification informations.
 */
static void socket_cb(SOCKET sock, uint8_t msg_type, void *msg_data)
{
    mqtt_socket_event_handler(sock, msg_type, msg_data);
}

/**
 * \brief Callback of gethostbyname function.
 *
 * \param[in] doamin_name Domain name.
 * \param[in] server_ip IP of server.
 */
static void resolve_cb(uint8_t *doamin_name, uint32_t server_ip)
{
    mqtt_socket_resolve_handler(doamin_name, server_ip);
}

/**
 * \brief Callback to receive the subscribed Message.
 *
 * \param[in] msgData Data to be received.
 */

static void subscribe_cb(MessageData *msgData)
{
    /* You received publish message which you had subscribed. */
    /* Print Topic and message */
    char str[MQTT_MAX_BUFFER_SIZE*2];
    sprintf(str, "%.*s >> %.*s", msgData->topicName->lenstring.len,msgData->topicName->lenstring.data,
        msgData->message->payloadlen,(char *)msgData->message->payload);
    nrf_cli_fprintf(mp_curr_cli, NRF_CLI_OPTION, "\r\n%s", str);
    nrf_cli_process(mp_curr_cli);
}

/**
 * \brief Callback to get the MQTT status update.
 *
 * \param[in] conn_id instance id of connection which is being used.
 * \param[in] type type of MQTT notification. Possible types are:
 *  - [MQTT_CALLBACK_SOCK_CONNECTED](@ref MQTT_CALLBACK_SOCK_CONNECTED)
 *  - [MQTT_CALLBACK_CONNECTED](@ref MQTT_CALLBACK_CONNECTED)
 *  - [MQTT_CALLBACK_PUBLISHED](@ref MQTT_CALLBACK_PUBLISHED)
 *  - [MQTT_CALLBACK_SUBSCRIBED](@ref MQTT_CALLBACK_SUBSCRIBED)
 *  - [MQTT_CALLBACK_UNSUBSCRIBED](@ref MQTT_CALLBACK_UNSUBSCRIBED)
 *  - [MQTT_CALLBACK_DISCONNECTED](@ref MQTT_CALLBACK_DISCONNECTED)
 *  - [MQTT_CALLBACK_RECV_PUBLISH](@ref MQTT_CALLBACK_RECV_PUBLISH)
 * \param[in] data A structure contains notification informations. @ref mqtt_data
 */
static void mqtt_callback(struct mqtt_module *module_inst, int type, union mqtt_data *data)
{
    switch (type) 
    {
    case MQTT_CALLBACK_SOCK_CONNECTED:
    {
        /*
         * If connecting to broker server is complete successfully, 
         * Start sending CONNECT message of MQTT.
         *
         */
        if (data->sock_connected.result >= 0) 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Connecting to Broker...\r\n");
            mqtt_connect_broker(module_inst, 0, 
                strlen(m_mqtt_user_id)? m_mqtt_user_id : NULL,
                strlen(m_mqtt_password)? m_mqtt_password : NULL,
                (char *)m_mqtt_client_id, 
                NULL, NULL, 0, 0, 0);
        } 
        else 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "Connect fail to server\r\n");
        }
    }
    break;

    case MQTT_CALLBACK_CONNECTED:
    {
        if (data->connected.result == MQTT_CONN_RESULT_ACCEPT) 
        {
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Connect to Broker successfully\r\n");
        } 
        else 
        {
            /* Cannot connect for some reason. */
            nrf_cli_fprintf(mp_curr_cli, NRF_CLI_ERROR, "MQTT broker decline your access! error code %d\r\n", data->connected.result);
        }
    }
    break;

    case MQTT_CALLBACK_DISCONNECTED:
    {
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "MQTT disconnected\r\n");
    }
    break;

    case MQTT_CALLBACK_UNSUBSCRIBED:
    {
        nrf_cli_fprintf(mp_curr_cli, NRF_CLI_NORMAL, "Unsubscribed\r\n");
    }
    break;

    default:
    {
        break;
    }    
    }

    nrf_cli_process(mp_curr_cli);
}

/**
 * \brief Configure MQTT service.
 */
static void configure_mqtt(void)
{
    struct mqtt_config mqtt_conf;
    int result;

    mqtt_get_config_defaults(&mqtt_conf);
    /* To use the MQTT service, it is necessary to always set the buffer and the timer. */
    mqtt_conf.read_buffer = m_mqtt_read_buffer;
    mqtt_conf.read_buffer_size = sizeof(m_mqtt_read_buffer);
    mqtt_conf.send_buffer = m_mqtt_send_buffer;
    mqtt_conf.send_buffer_size = sizeof(m_mqtt_send_buffer);

#if defined(MQTT_TLS_ENABLED)
    mqtt_conf.tls = 1;
#if defined(MQTT_TLS_PORT)
    mqtt_conf.port = MQTT_TLS_PORT;
#else
    mqtt_conf.port = 8883;
#endif
#endif
    
    result = mqtt_init(&m_mqtt_inst, &mqtt_conf);
    if (result < 0) 
    {
        while (true) 
        {
            bsp_board_led_invert(1);
            nrf_delay_ms(200);
        }
    }

    result = mqtt_register_callback(&m_mqtt_inst, mqtt_callback);
    if (result < 0) 
    {
        while (true) 
        {
            bsp_board_led_invert(1);
            nrf_delay_ms(200);
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

    /* Enable SysTick interrupt for non busy wait delay. */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (true) 
        {
            bsp_board_led_invert(1);
            nrf_delay_ms(200);
        }
    }

    /* Initialize the MQTT service. */
    configure_mqtt();

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

    /* Initialize socket interface. */
    socketInit();
    registerSocketCallback(socket_cb, resolve_cb);
}

/**@brief Function for processing the Wi-Fi events.
 */
void wifi_process(void)
{
    while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS)
    {
        // No implementation needed.
    }

    if(m_mqtt_inst.isConnected)
    {
        mqtt_yield(&m_mqtt_inst, 0);
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

    if(m_mqtt_inst.client->isconnected)
    {
        /* Disconnect from MQTT broker. */
        /* Force close the MQTT connection, because cannot send a disconnect message to the broker when network is broken. */
        mqtt_disconnect(&m_mqtt_inst, 1);
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

/**@brief mqtt command implementation.
 */
static void cmd_mqtt(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

/**@brief mqtt connect command implementation.
 */
static void cmd_mqtt_connect(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc > 4)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(!m_wifi_connected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Connect the Wi-Fi first\r\n");
        return;        
    }

    if(m_mqtt_inst.client->isconnected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "MQTT client is already connected\r\n");
        return;         
    }

    memset(m_mqtt_user_id, 0, sizeof(m_mqtt_user_id));
    memset(m_mqtt_password, 0, sizeof(m_mqtt_password));

    if(argc == 4) //User ID and Password used
    {
        if(strlen(argv[2]) >= MQTT_MAX_USER_ID_LEN)
        {
            strncpy(m_mqtt_user_id, argv[2], MQTT_MAX_USER_ID_LEN-1);
        }
        else
        {
            strcpy(m_mqtt_user_id, argv[2]);
        }

        if(strlen(argv[3]) >= MQTT_MAX_PASSWORD_LEN)
        {
            strncpy(m_mqtt_password, argv[3], MQTT_MAX_PASSWORD_LEN-1);
        }
        else
        {
            strcpy(m_mqtt_password, argv[3]);
        }
    }

    if(argc == 3) // Only ID used and Password is NULL
    {
        if(strlen(argv[2]) >= MQTT_MAX_USER_ID_LEN)
        {
            strncpy(m_mqtt_user_id, argv[2], MQTT_MAX_USER_ID_LEN-1);
        }
        else
        {
            strcpy(m_mqtt_user_id, argv[2]);
        }        
    }

    mqtt_client_id_generate(m_mqtt_client_id);

    /* Try to connect to MQTT broker when Wi-Fi was connected. */
    mqtt_connect(&m_mqtt_inst, argv[1]);

    mp_curr_cli = p_cli;

}

/**@brief mqtt disconnect command implementation.
 */
static void cmd_mqtt_disconnect(nrf_cli_t const * p_cli, size_t argc, char **argv)
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

    /* Disconnect from MQTT broker. */
    mqtt_disconnect(&m_mqtt_inst, 0);

    mp_curr_cli = p_cli;

}

/**@brief mqtt pub command implementation.
 */
static void cmd_mqtt_pub(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t qos = 0;
    uint8_t retain = 0;

    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc < 3 || argc > 5)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(!m_mqtt_inst.client->isconnected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Broker is disconnected\r\n");
        return;
    }

    if(argc == 4) // QoS is set
    {
        qos = atoi(argv[3]);
        if(qos > 2)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "QoS must be 0, 1 or 2\r\n");
            return;
        }        
    }

    if(argc == 5) //QoS and retain are set
    {
        qos = atoi(argv[3]);
        if(qos > 2)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "QoS must be 0, 1 or 2\r\n");
            return;
        }

        if(!strcmp(argv[4], "0"))
        {
            retain = 0;
        }
        else
        {
            retain = 1;
        }
    }

    mqtt_publish(&m_mqtt_inst, argv[1], argv[2], strlen(argv[2]), qos, retain);

    mp_curr_cli = p_cli;
}

/**@brief mqtt sub command implementation.
 */
static void cmd_mqtt_sub(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t qos = 0;
    ASSERT(p_cli);
    ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    if ((argc == 1) || nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    if(argc > 3)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameters\r\n");
        return;
    }

    if(argc == 3) // QoS is set
    {
        qos = atoi(argv[2]);
        if(qos > 2)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "QoS must be 0, 1 or 2\r\n");
            return;
        }
    }

    if(!m_mqtt_inst.client->isconnected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Broker is disconnected\r\n");
        return;
    }

    /* Subscribe the topic. */
    mqtt_subscribe(&m_mqtt_inst, argv[1], qos, subscribe_cb);

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Subscribing topic(%s), QoS(%d)\r\n", argv[1], qos);

    mp_curr_cli = p_cli;
}

/**@brief mqtt unsub command implementation.
 */
static void cmd_mqtt_unsub(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t qos = 0;
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

    if(!m_mqtt_inst.client->isconnected)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Broker is disconnected\r\n");
        return;
    }

    /* Unsubscribe the topic. */
    mqtt_unsubscribe(&m_mqtt_inst, argv[1]);

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


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_mqtt)
{
    NRF_CLI_CMD(connect, NULL, "Connect to a MQTT Broker. Usage: mqtt connect [HOST] {USER} {PWD}", cmd_mqtt_connect),
    NRF_CLI_CMD(disconnect, NULL, "Disconnect from MQTT broker", cmd_mqtt_disconnect),
    NRF_CLI_CMD(pub, NULL, "Publish message to MQTT broker. Usage: mqtt pub [topic] [message] {qos} {retain}", cmd_mqtt_pub),
    NRF_CLI_CMD(sub, NULL, "Subscribe message from MQTT broker. Usage: mqtt sub [topic] {qos}", cmd_mqtt_sub),
    NRF_CLI_CMD(unsub, NULL, "Unsubscribe message from MQTT broker. Usage: mqtt unsub [topic]", cmd_mqtt_unsub),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(mqtt, &m_sub_mqtt, "Commands for MQTT access", cmd_mqtt);
