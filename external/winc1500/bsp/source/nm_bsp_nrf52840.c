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

#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "conf_winc.h"
#include "mem_manager.h"


static tpfNmBspIsr gpfIsr;

/*
 *	@fn    gpiote_evt_handler
 *	@brief GPIOTE event handler, executed in interrupt-context.
 */
static void gpiote_evt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if ((pin == CONF_WINC_SPI_INT_PIN) && (action == NRF_GPIOTE_POLARITY_HITOLO))
    {
        if (gpfIsr) {
        	gpfIsr();
        }
    }
}

/*
 *	@fn    gpiote_init
 *	@brief Intitialize GPIOTE to catch pin interrupts.
 */
static uint32_t gpiote_init(uint32_t pin)
{
    uint32_t err_code;

    /* Configure gpiote for the data ready interrupt */
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if(err_code != NRF_SUCCESS) {
        	return err_code;
        }
    }

    nrf_drv_gpiote_in_config_t gpiote_in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    gpiote_in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(pin, &gpiote_in_config, gpiote_evt_handler);
    if(err_code != NRF_SUCCESS) {
    	return err_code;
    }

    nrf_drv_gpiote_in_event_enable(pin, true);

    return NRF_SUCCESS;
}

/*
 *	@fn    gpiote_uninit
 *	@brief Unintitialize GPIOTE pin interrupt.
 */
static void gpiote_uninit(uint32_t pin)
{
    nrf_drv_gpiote_in_event_disable(pin);
    nrf_drv_gpiote_in_uninit(pin);
}


/*
 *	@fn		init_chip_pins
 *	@brief	Initialize reset, chip enable and wake pin
 */
static void init_chip_pins(void)
{
	nrf_gpio_cfg_output(CONF_WINC_PIN_RESET);
	nrf_gpio_cfg_output(CONF_WINC_PIN_CHIP_ENABLE);
	nrf_gpio_cfg_output(CONF_WINC_PIN_WAKE);
	nrf_gpio_pin_clear(CONF_WINC_PIN_CHIP_ENABLE);
	nrf_gpio_pin_clear(CONF_WINC_PIN_RESET);
}

/*
*	@fn		nm_bsp_init
*	@brief	Initialize BSP
*	@return	0 in case of success and -1 in case of failure
*/
sint8 nm_bsp_init(void)
{
	gpfIsr = NULL;

	static bool is_mem_initialized = false;

	if(!is_mem_initialized)
	{
		APP_ERROR_CHECK(nrf_mem_init());
		is_mem_initialized = true;
	}
	
	/* Initialize chip IOs. */
	init_chip_pins();

	return M2M_SUCCESS;
}

/**
*   @fn      nm_bsp_deinit
*   @brief   De-iInitialize BSP
*   @return  0 in case of success and -1 in case of failure
*/
sint8 nm_bsp_deinit(void)
{
	nrf_gpio_pin_clear(CONF_WINC_PIN_CHIP_ENABLE);
	nrf_gpio_pin_clear(CONF_WINC_PIN_RESET);

	gpiote_uninit(CONF_WINC_SPI_INT_PIN);

	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_reset
 *	@brief	Reset WINC1500 SoC by setting CHIP_EN and RESET_N signals low,
 *           CHIP_EN high then RESET_N high
 */
void nm_bsp_reset(void)
{
	nrf_gpio_pin_clear(CONF_WINC_PIN_CHIP_ENABLE);
	nrf_gpio_pin_clear(CONF_WINC_PIN_RESET);
	nm_bsp_sleep(1);
	nrf_gpio_pin_set(CONF_WINC_PIN_CHIP_ENABLE);
	nm_bsp_sleep(10);
	nrf_gpio_pin_set(CONF_WINC_PIN_RESET);
}

/*
*	@fn		nm_bsp_sleep
*	@brief	Sleep in units of mSec
*	@param[IN]	u32TimeMsec
*				Time in milliseconds
*/
void nm_bsp_sleep(uint32 u32TimeMsec)
{
	nrf_delay_ms(u32TimeMsec);
}

/*
*	@fn		nm_bsp_register_isr
*	@brief	Register interrupt service routine
*	@param[IN]	pfIsr
*				Pointer to ISR handler
*/
void nm_bsp_register_isr(tpfNmBspIsr pfIsr)
{
	gpfIsr = pfIsr;

	/* Configure PGIO pin for interrupt from SPI slave, used when slave has data to send. */
	APP_ERROR_CHECK(gpiote_init(CONF_WINC_SPI_INT_PIN));
}

/*
*	@fn		nm_bsp_interrupt_ctrl
*	@brief	Enable/Disable interrupts
*	@param[IN]	u8Enable
*				'0' disable interrupts. '1' enable interrupts
*/
void nm_bsp_interrupt_ctrl(uint8 u8Enable)
{
	if (u8Enable) {
		nrf_drv_gpiote_in_event_enable(CONF_WINC_SPI_INT_PIN, true);
	}
	else {
		nrf_drv_gpiote_in_event_disable(CONF_WINC_SPI_INT_PIN);
	}
}

