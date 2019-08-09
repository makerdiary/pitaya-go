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
 * @defgroup battery_status_example_main main.c
 * @{
 * @ingroup battery_status_example
 *
 * @brief Battery Status Example Application main file.
 *
 * This file contains the source code for a sample application to get the battery status.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrf_drv_clock.h"
#include "nrf_drv_saadc.h"
#include "nrf_delay.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"
#include "bsp.h"
#include "sdk_config.h"

#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                60                                          /**< Maximum number of events in the scheduler queue. */

APP_TIMER_DEF(m_battery_timer_id);                                                  /**< Battery timer. */


void configure_console(void);


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
    bool is_charging, is_power_good;

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

    is_charging = !(bool)nrf_gpio_pin_read(BATT_CHRG_PIN);
    is_power_good = !(bool)nrf_gpio_pin_read(BATT_PGOOD_PIN);


    if(is_power_good)
    {
        printf("Now drawing from 'USB Power'\r\n");
        printf("Status: %s\r\n", is_charging? "charging" : "charged");
    }
    else
    {
       printf("Now drawing from 'Battery Power'\r\n");
    }
    printf("Battery level update: %d%%\r\n\r\n", battery_level);
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
 * @brief Function for application main entry.
 */
int main(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    APP_ERROR_CHECK(nrf_drv_clock_init());
    
    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    APP_ERROR_CHECK(app_timer_init());

    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);

    /* Configure Console */
    configure_console();

    battery_measurement_init();

    battery_measurement_start();

    bsp_board_led_on(LED_G_IDX);

    while (true)
    {
        app_sched_execute();

        // Wait for an event.
        __WFE();
        // Clear the internal event register.
        __SEV();
        __WFE();

        // no implementation needed
    }
}

/** @} */
