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
 * @defgroup button_example_main main.c
 * @{
 * @ingroup button_example
 *
 * @brief Button Example Application main file.
 *
 * This file contains the source code for a sample application using the user button.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrf_drv_clock.h"
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

#define BTN_USER                 0

#define BTN_USER_KEY_RELEASE     (bsp_event_t)(BSP_EVENT_KEY_LAST + 1)
#define BTN_USER_KEY_LONG_PUSH   (bsp_event_t)(BSP_EVENT_KEY_LAST + 2)

void configure_console(void);


static void bsp_event_callback(bsp_event_t ev)
{
    switch ((unsigned int)ev)
    {
        case CONCAT_2(BSP_EVENT_KEY_, BTN_USER):
        {
            printf("User button pushed.\r\n");
            bsp_board_led_on(LED_R_IDX);
            break;
        }
        
        case BTN_USER_KEY_RELEASE:
        {
            printf("User button released.\r\n");
            bsp_board_led_off(LED_R_IDX);
            break;
        }

        case BTN_USER_KEY_LONG_PUSH:
        {
            printf("User button long pushed.\r\n");
            bsp_board_led_invert(LED_B_IDX);
            break;
        }

        default:
            return; // no implementation needed
    }
}

static void init_bsp(void)
{
    ret_code_t ret;
    ret = bsp_init(BSP_INIT_BUTTONS, bsp_event_callback);
    APP_ERROR_CHECK(ret);
    
    UNUSED_RETURN_VALUE(bsp_event_to_button_action_assign(BTN_USER,
                                                          BSP_BUTTON_ACTION_RELEASE,
                                                          BTN_USER_KEY_RELEASE));

    UNUSED_RETURN_VALUE(bsp_event_to_button_action_assign(BTN_USER,
                                                          BSP_BUTTON_ACTION_LONG_PUSH,
                                                          BTN_USER_KEY_LONG_PUSH));
    
    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);
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

    /* Configure board. */
    init_bsp();

    /* Configure Console */
    configure_console();

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
