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

/** @file console_log.c
 * @brief Logging console over USB CDC ACM.
 *
 * This implements the logging console over USB CDC ACM.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sdk_common.h"
#include "nrf_queue.h"
#include "app_error.h"
#include "nrf_assert.h"
#include "nrf_delay.h"

#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_cdc_acm.h"


#define CDC_ACM_COMM_INTERFACE        0
#define CDC_ACM_COMM_EPIN             NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE        1
#define CDC_ACM_DATA_EPIN             NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT            NRF_DRV_USBD_EPOUT1

volatile static bool m_transfer_done = false;

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
                            APP_USBD_CDC_COMM_PROTOCOL_NONE
);


NRF_QUEUE_DEF(uint8_t,
              m_rx_queue,
              2*NRF_DRV_USBD_EPSIZE,
              NRF_QUEUE_MODE_OVERFLOW);

static char m_rx_buffer[NRF_DRV_USBD_EPSIZE];


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

/**
 * @brief Set new buffer and process any data if already present
 *
 * This is internal function.
 * The result of its execution is the library waiting for the event of the new data.
 * If there is already any data that was returned from the CDC internal buffer
 * it would be processed here.
 */
static void cdc_acm_process_and_prepare_buffer(app_usbd_cdc_acm_t const * p_cdc_acm)
{
    for (;;)
    {
        if (!nrf_queue_is_empty(&m_rx_queue))
        {
            //
        }
        ret_code_t ret = app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                                                   m_rx_buffer,
                                                   sizeof(m_rx_buffer));
        if (ret == NRF_SUCCESS)
        {
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            size_t qsize = nrf_queue_in(&m_rx_queue, m_rx_buffer, size);
            ASSERT(size == qsize);
            UNUSED_VARIABLE(qsize);
        }
        else if (ret == NRF_ERROR_IO_PENDING)
        {
            break;
        }
        else
        {
            APP_ERROR_CHECK(ret);
            break;
        }
    }
}



/**
 * @brief User event handler.
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);


    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            /*Setup first transfer*/
            cdc_acm_process_and_prepare_buffer(p_cdc_acm);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            m_transfer_done = true;
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            /*Get amount of data transfered*/
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            size_t qsize = nrf_queue_in(&m_rx_queue, m_rx_buffer, size);
            ASSERT(size == qsize);
            UNUSED_VARIABLE(qsize);

            /*Setup next transfer*/
            cdc_acm_process_and_prepare_buffer(p_cdc_acm);
            break;
        }
        default:
            break;
    }
}

void configure_console(void)
{
    static bool usb_initialized = false;

    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_handler = app_usbd_event_execute,
        .ev_state_proc = usbd_user_ev_handler
    };

    if(usb_initialized)
    {
        return;
    }

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

    usb_initialized = true;
}

#if defined(__GNUC__)

int _write(int file, const char * p_char, int len)
{
    UNUSED_PARAMETER(file);

    ret_code_t ret;

    if(p_char == NULL || len == 0)
    {
        return 0; // Nothing to write
    }

    m_transfer_done = false;
    ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, p_char, len);
    if(ret == NRF_ERROR_INVALID_STATE)
    {
        return 0; /*Port is not opened*/
    }
    while(!m_transfer_done);

    return len;
}

int _read(int file, char * p_char, int len)
{
    UNUSED_PARAMETER(file);
    
    while(nrf_queue_is_empty(&m_rx_queue))
    {
        // No implementation needed.
    }

    size_t qsize = nrf_queue_out(&m_rx_queue, p_char, len);

    return qsize;
}

#endif


