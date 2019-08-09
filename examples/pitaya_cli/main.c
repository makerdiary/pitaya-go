/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief Wi-Fi exmaple Application main file.
 *
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "boards.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_state.h"

#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "task_manager.h"
#include "nrf_cli.h"
#include "nrf_cli_ble_uart.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_saadc.h"
#include "nrf_stack_guard.h"
#include "nrf_fstorage_sd.h"

#if defined(APP_USBD_ENABLED) && APP_USBD_ENABLED
#define CLI_OVER_USB_CDC_ACM 1
#else
#define CLI_OVER_USB_CDC_ACM 0
#endif

#if defined(NRF_CLI_UART_ENABLED) && NRF_CLI_UART_ENABLED
#define CLI_OVER_UART 1
#else
#define CLI_OVER_UART 0
#endif

#if CLI_OVER_UART
#include "nrf_cli_uart.h"
#endif

#if CLI_OVER_USB_CDC_ACM
#include "nrf_cli_cdc_acm.h"
#include "nrf_drv_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#endif //CLI_OVER_USB_CDC_ACM


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_backend_flash.h"

#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                60                                          /**< Maximum number of events in the scheduler queue. */

#define APP_ADV_INTERVAL                300                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(400, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(650, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

void wifi_setup(void);
void wifi_process(void);

BLE_BAS_DEF(m_bas);                                                                 /**< Battery service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_battery_timer_id);                                                  /**< Battery timer. */

#if CLI_OVER_USB_CDC_ACM
NRF_CLI_CDC_ACM_DEF(m_cli_cdc_acm_transport);
NRF_CLI_DEF(m_cli_cdc_acm,
            "usb_cli:~$ ",
            &m_cli_cdc_acm_transport.transport,
            '\r',
            4);
#endif //CLI_OVER_USB_CDC_ACM

#if CLI_OVER_UART
NRF_CLI_UART_DEF(cli_uart,0 , 256, 16);
NRF_CLI_DEF(m_cli_uart, "uart_cli:~$ ", &cli_uart.transport,'\r', 4);
#endif

NRF_CLI_BLE_UART_DEF(cli_ble_uart, &m_gatt, 1024, 1024);
NRF_CLI_DEF(m_ble_cli, "ble_cli:~$ ", &cli_ble_uart.transport,'\r', 8);


//NRF_LOG_BACKEND_FLASHLOG_DEF(m_flash_log_backend);
//NRF_LOG_BACKEND_CRASHLOG_DEF(m_crash_log_backend);

static uint16_t  m_conn_handle = BLE_CONN_HANDLE_INVALID;                           /**< Handle of the current connection. */

task_id_t m_ble_console_task_id;


static ble_uuid_t m_adv_uuids[] =                                                   /**< Universally unique service identifiers. */
{
    {BLE_UUID_BATTERY_SERVICE,            BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};


#if CLI_OVER_USB_CDC_ACM

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif


static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        default:
            break;
    }
}

#endif //CLI_OVER_USB_CDC_ACM


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t err_code;

        err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_bas_init_t     bas_init;
    ble_dis_init_t     dis_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Connected.");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);

            nrf_cli_ble_uart_config_t config = { .conn_handle = m_conn_handle };

            err_code = nrf_cli_init(&m_ble_cli, &config, true, true, NRF_LOG_SEVERITY_INFO);
            APP_ERROR_CHECK(nrf_cli_task_create(&m_ble_cli));

            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected, reason %d.",
                          p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            (void)nrf_cli_uninit(&m_ble_cli);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/** @brief Function for initializing BLE components.
 */
void ble_init(void)
{
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    peer_manager_init();
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for handling the SAADC events.
*/
static void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        //
    }
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code;
    uint8_t  battery_level;
    nrf_saadc_value_t adc_value = 0;
    uint16_t voltage_mv = 0;

    err_code = nrf_drv_saadc_sample_convert(0, &adc_value);
    APP_ERROR_CHECK(err_code);

    voltage_mv = adc_value * 3600 * BATT_VOLTAGE_DIVIDER_FACTOR / 1024;

    int16_t soc_vector_element = (voltage_mv - BATT_MEAS_LOW_BATT_LIMIT_MV)/
                                               BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV;
    
    // Ensure that only valid vector entries are used.
    if (soc_vector_element < 0)
    {
        soc_vector_element = 0;
    }
    else if (soc_vector_element > (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS - 1) )
    {
        soc_vector_element = (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS - 1);
    }

    battery_level = BATT_MEAS_VOLTAGE_TO_SOC[soc_vector_element];

    //NRF_LOG_INFO("Battery level update: %d", battery_level);
    err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


/**@brief Function for initialization of ticker for periodical battery measurement.
 */
static void battery_measurement_init(void)
{
    ret_code_t err_code;

    nrf_drv_saadc_config_t saadc_config = NRF_DRV_SAADC_DEFAULT_CONFIG;
    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATT_VSENSE_AIN);

    nrf_gpio_cfg_input(BATT_CHRG_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(BATT_PGOOD_PIN, NRF_GPIO_PIN_NOPULL);

   /* Burst enabled to oversample the SAADC. */
    channel_config.burst    = NRF_SAADC_BURST_ENABLED;
    channel_config.acq_time = NRF_SAADC_ACQTIME_40US;

    err_code = nrf_drv_saadc_init(&saadc_config, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    // Create timers.
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting batter measurement timers.
 */
static void battery_measurement_start(void)
{
    ret_code_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_battery_timer_id, APP_TIMER_TICKS(BATT_MEAS_INTERVAL_MS), NULL);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for shutdown events.
 *
 * @param[in]   event       Shutdown type.
 */
static bool shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    switch (event)
    {
        case NRF_PWR_MGMT_EVT_PREPARE_WAKEUP:
            // Prepare wakeup buttons.
            err_code = bsp_btn_ble_sleep_mode_prepare();
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(shutdown_handler, 0);

void idle_task(void * p_context)
{
    bool erase_bonds = (bool)p_context;

    battery_measurement_start();

    advertising_start(erase_bonds);

    // Set the TX Power
    APP_ERROR_CHECK(sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, 0, 8));

    // Enter main loop.
    for (;;)
    {
        app_sched_execute();

        wifi_process();

        if (NRF_LOG_PROCESS() == false)
        {
            nrf_pwr_mgmt_run();
        }
        task_yield();
    }

}

static void usbd_init(void)
{
#if CLI_OVER_USB_CDC_ACM
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_handler = app_usbd_event_execute,
        .ev_state_proc = usbd_user_ev_handler
    };

    app_usbd_serial_num_generate();

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm =
            app_usbd_cdc_acm_class_inst_get(&nrf_cli_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
    
    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    /* Give some time for the host to enumerate and connect to the USB CDC port */
    nrf_delay_ms(1000);
#endif
}

static void core_init(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(app_timer_cnt_get));

    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

#if CLI_OVER_USB_CDC_ACM
    APP_ERROR_CHECK(nrf_cli_init(&m_cli_cdc_acm, NULL, true, true, NRF_LOG_SEVERITY_INFO));
#endif

#if CLI_OVER_UART
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = TX_PIN_NUMBER;
    uart_config.pselrxd = RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    APP_ERROR_CHECK(nrf_cli_init(&m_cli_uart, &uart_config, true, true, NRF_LOG_SEVERITY_INFO));
#endif

    APP_ERROR_CHECK(nrf_drv_clock_init());

    nrf_drv_clock_lfclk_request(NULL);

    APP_ERROR_CHECK(app_timer_init());

    usbd_init();

    APP_ERROR_CHECK(nrf_stack_guard_init());

    APP_ERROR_CHECK(nrf_pwr_mgmt_init());

#if CLI_OVER_USB_CDC_ACM
    APP_ERROR_CHECK(nrf_cli_task_create(&m_cli_cdc_acm));
#endif

#if CLI_OVER_UART
    APP_ERROR_CHECK(nrf_cli_task_create(&m_cli_uart));
#endif

}


/**@brief Function for application main entry.
 */
int main(void)

{
    bool erase_bonds;

    core_init();

    buttons_leds_init(&erase_bonds);

    /* Setup the wifi module */
    wifi_setup();

    ble_init();

    battery_measurement_init();

    APP_ERROR_CHECK(nrf_cli_ble_uart_service_init());

    erase_bonds = true;
    
    task_manager_start(idle_task, (void *)erase_bonds);
}
