/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
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
/** @file
 * @brief RSSI Viewer Example Application main file.
 *
 * This file contains the source code for a sample application to behave a RSSI Viewer.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_fifo.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_timer.h"

#include "boards.h"

#define FREQ_ADV_CHANNEL_37           (2)      /**<Radio channel number which corresponds with 37-th BLE channel **/
#define FREQ_ADV_CHANNEL_38           (26)     /**<Radio channel number which corresponds with 38-th BLE channel **/
#define FREQ_ADV_CHANNEL_39           (80)     /**<Radio channel number which corresponds with 39-th BLE channel **/
#define FIRST_CHANNEL                 (0) 
#define LAST_CHANNEL                  (80)
#define RSSI_NO_SIGNAL                (127)    /**< Minimum value of RSSISAMPLE */


// LED definitions for USB state
enum {
    LED_USB_RESUME = 0,
    LED_CDC_ACM_OPEN,
    LED_CDC_ACM_USER,
};

#define M_FIFO_BUFSIZE                (256)

#define CDC_ACM_COMM_INTERFACE        0
#define CDC_ACM_COMM_EPIN             NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE        1
#define CDC_ACM_DATA_EPIN             NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT            NRF_DRV_USBD_EPOUT1


static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

/**
 * @brief CDC_ACM class instance
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

APP_TIMER_DEF(m_rssi_timer);

static uint32_t sweep_delay       = 10;
static uint32_t scan_repeat_times = 1;

static bool scan_ble_adv          = false;
static bool tx_done               = false;

static app_fifo_t m_fifo;
static uint8_t m_fifo_buf[M_FIFO_BUFSIZE];
static char input[32];


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
        case APP_USBD_EVT_DRV_SUSPEND:
            bsp_board_led_off(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_DRV_RESUME:
            bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
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

void send_more()
{
    if (!tx_done) return;
    
    static uint8_t more[M_FIFO_BUFSIZE];
    static uint32_t length;
    length = M_FIFO_BUFSIZE;

    ret_code_t ret = app_fifo_read(&m_fifo, more, &length);
    if (ret == NRF_ERROR_NOT_FOUND) {
        return;
    }
    
    APP_ERROR_CHECK(app_usbd_cdc_acm_write(&m_app_cdc_acm, more, length));
    tx_done = false;
}

void queue_packet(uint8_t channel_number, uint8_t rssi)
{
    uint32_t length = 3;
    uint8_t pkt[3];
    pkt[0] = 0xff;
    pkt[1] = channel_number;
    pkt[2] = rssi;
    
    ret_code_t ret = app_fifo_write(&m_fifo, pkt, &length);
    if (ret != NRF_SUCCESS) {
        return;
    }
    
    send_more();
}

void set_scan_ble_adv(bool enable)
{
    scan_ble_adv = enable;
    for (uint8_t i = FIRST_CHANNEL; i <= LAST_CHANNEL; ++i)
    {
        queue_packet(i, RSSI_NO_SIGNAL);
    }
}

void rssi_measurer_configure_radio(void)
{
    NRF_RNG->TASKS_START = 1;

    // Start 16 MHz crystal oscillator
    NRF_CLOCK->EVENTS_HFCLKSTARTED  = 0;
    NRF_CLOCK->TASKS_HFCLKSTART     = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

#define WAIT_FOR( m ) do { while (!m); m = 0; } while(0)

uint8_t rssi_measurer_scan_channel(uint8_t channel_number)
{
    uint8_t sample;

    NRF_RADIO->FREQUENCY  = channel_number;
    NRF_RADIO->TASKS_RXEN = 1;

    WAIT_FOR(NRF_RADIO->EVENTS_READY);
    NRF_RADIO->TASKS_RSSISTART = 1;
    WAIT_FOR(NRF_RADIO->EVENTS_RSSIEND);

    sample = 127 & NRF_RADIO->RSSISAMPLE;

    NRF_RADIO->TASKS_DISABLE = 1;
    WAIT_FOR(NRF_RADIO->EVENTS_DISABLED);

    return sample;
}

uint8_t rssi_measurer_scan_channel_repeat(uint8_t channel_number)
{
    uint8_t sample;
    uint8_t max = RSSI_NO_SIGNAL;
    for (int i = 0; i <= scan_repeat_times; ++i) {
        sample = rssi_measurer_scan_channel(channel_number);
        // taking minimum since sample = -dBm.
        max = MIN(sample, max);
    }
    return max;
}

void rssi_measurer_start_timer()
{
    APP_ERROR_CHECK(app_timer_start(m_rssi_timer,  APP_TIMER_TICKS(sweep_delay), NULL));
}

void rssi_measurer_timeout_handler(void* p_context)
{   
    uint8_t sample;
    if (scan_ble_adv) {
        sample = rssi_measurer_scan_channel_repeat(FREQ_ADV_CHANNEL_37);
        queue_packet(FREQ_ADV_CHANNEL_37, sample);
        sample = rssi_measurer_scan_channel_repeat(FREQ_ADV_CHANNEL_38);
        queue_packet(FREQ_ADV_CHANNEL_38, sample);
        sample = rssi_measurer_scan_channel_repeat(FREQ_ADV_CHANNEL_39);
        queue_packet(FREQ_ADV_CHANNEL_39, sample);
    } else {
        for (uint8_t i = FIRST_CHANNEL; i <= LAST_CHANNEL; i++)
        {
            sample = rssi_measurer_scan_channel_repeat(i);
            queue_packet(i, sample);
        }
    }

    rssi_measurer_start_timer();
}

void set_sweep_delay(uint32_t sd)
{
    sweep_delay = sd;
}

void stop()
{
    app_fifo_flush(&m_fifo);
    APP_ERROR_CHECK(app_timer_stop(m_rssi_timer));
}

void start()
{
    stop();
    memset(input, 32, 0);
    tx_done = true;
    rssi_measurer_start_timer();
}

void get_line()
{   
    char* q = input;
    if (strncmp(q, "set ", 4) == 0) {
        q += 4;
        if (strncmp(q, "delay ", 6) == 0) {
            q += 6;
            int d = atoi(q);
            set_sweep_delay(MAX(1, MIN(d, 1000)));
            return;
        }
        if (strncmp(q, "repeat ", 7) == 0) {
            q += 7;
            int d = atoi(q);
            scan_repeat_times = MAX(1, MIN(d, 100));
            return;
        }
        return;
    }
    if (strncmp(q, "start", 5) == 0) {
        start();
        return;
    }
    if (strncmp(q, "stop", 4) == 0) {
        stop();
        return;
    }
    if (strncmp(q, "scan adv ", 9) == 0) {
        q += 9;
        if (strncmp(q, "true", 4) == 0) {
            set_scan_ble_adv(true);
            return;
        }
        if (strncmp(q, "false", 5) == 0) {
            set_scan_ble_adv(false);
        }
        return;
    }
    if (strncmp(q, "led", 3) == 0) {
        bsp_board_led_invert(LED_CDC_ACM_USER);
        return;
    }
}

void one_byte_received(char c)
{
    static char* p = input;
    *(p++) = c;
    if (c == '\0' || c == '\r') {
        *p = '\0';
        get_line();
        memset(input, 32, 0);
        p = input;
    }
}


/**
 * @brief User event handler.
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    static char acm_buffer[32];
    ret_code_t ret;

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            bsp_board_led_on(LED_CDC_ACM_OPEN);
            bsp_board_led_off(LED_CDC_ACM_USER);
            app_usbd_cdc_acm_read(&m_app_cdc_acm, acm_buffer, 1);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        {
            bsp_board_led_off(LED_CDC_ACM_OPEN);
            bsp_board_led_off(LED_CDC_ACM_USER);
            stop();
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        {
            tx_done = true;
            send_more();
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            do {
                one_byte_received(acm_buffer[0]);
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, acm_buffer, 1);
            } while (ret == NRF_SUCCESS);
            
            break;
        }
        default:
            break;
    }
}




/**
 * @brief Function for application main entry.
 */
int main(void)
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

    APP_ERROR_CHECK(nrf_drv_clock_init());
    
    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    /* Configure board. */
    bsp_board_init(BSP_INIT_LEDS);

    app_usbd_serial_num_generate();

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm =
            app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        app_usbd_enable();
        app_usbd_start();
    }

    /* Give some time for the host to enumerate and connect to the USB CDC port */
    nrf_delay_ms(1000);

    APP_ERROR_CHECK(app_fifo_init(&m_fifo, m_fifo_buf, M_FIFO_BUFSIZE));
    
    APP_ERROR_CHECK(app_timer_init());
    APP_ERROR_CHECK(app_timer_create(&m_rssi_timer, APP_TIMER_MODE_SINGLE_SHOT, rssi_measurer_timeout_handler));
    
    rssi_measurer_configure_radio();
    
    while (true)
    {
        while (app_usbd_event_queue_process());
    }
}

/** @} */
