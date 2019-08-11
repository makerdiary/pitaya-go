# How to Set Power Save Mode

## Description

This example demonstrates the use of the Wi-Fi module with the Pitaya Go board to check the PS (Power Save) mode.

This example also implements the Command Line Interface over BLE NUS and USB CDC ACM. Both consoles can be opened and used independently.

!!! Tip
	Before you start building, remember to set up the nRF5 SDK development environment first. See [Setup the nRF5 SDK](../../nrf5-sdk/setup-the-nrf5-sdk.md) for details.

## Hardware Prerequisites

* 1x Pitaya Go board
* 1x Digital Multimeter
* 1x DC Power Supply
* 1x 2-pin JST cable

## Building the example

You can find the source code and the project file of the example in the following folder: [pitaya-go/examples/wifi/power_save_mode_example](https://github.com/makerdiary/pitaya-go/tree/master/examples/wifi/power_save_mode_example).

Open terminal and navigate to the directory with the example Makefile:

``` sh
cd ./pitaya-go/examples/wifi/power_save_mode_example/armgcc
```

Run `make` to build the example:

``` sh
make
```

## Programming the firmware

If compiled successfully, the firmware is located in `power_save_mode_example/armgcc/_build` with the name `nrf52840_xxaa.hex`.

While pushing the **USER** button, press the **RESET** button to enter the DFU mode. Then program the firmware using the [nRF Connect for Desktop](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-desktop) tool.

If the SoftDevice is not programmed before, remember to add the SoftDevice. The SoftDevice is located in `<nRF5 SDK>/components/softdevice/s140/hex/s140_nrf52_6.1.0_softdevice.hex`.

!!! Tip
	See **[Programming](../../programming.md)** section for details about how to program your Pitaya Go.

## Testing

The application is now programmed and running. Perform the following steps to check the PS (Power Save) mode:

1. Follow the figure below to setup the hardware for current measurements:

	![](assets/images/power-profiling-setup.png)

2. Run a terminal application like [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) or [screen](https://www.gnu.org/software/screen/manual/screen.html):

	``` sh
	screen /dev/cu.usbmodemD2E39D222D781 115200
	```

3. Press <kbd>Enter</kbd> in the terminal. A console prompt is displayed.

4. Use command `wifi` to print the command help information.

5. Use command `wifi connect {SSID} {PSK}` to connect to your AP. The parameters stand for the following:

	* `{SSID}`: The name of AP. If not specified, the device will attempt to reconnect to the last-associated AP.
	* `{PSK}`: The passphrase of AP. MUST be 8 characters or more. If it's an Open network, it can be left blank.

6. Observe that BLUE LED is lit, that is, the Wi-Fi is connected.

7. Use command `wifi sleep` to place the Wi-Fi module into sleep mode for a specified period of time.

	![](assets/images/power-save-mode-example-terminal.png)

8. The current measurements can be done over Digital Multimeter once the current remains constant.

9. You can also use the [Web Device CLI](https://wiki.makerdiary.com/web-device-cli/) application to test this example.

	<a href="https://wiki.makerdiary.com/web-device-cli/" target="_blank"><button data-md-color-primary="marsala">Web Device CLI</button></a>

[![](assets/images/power-save-mode-example-web.png)](assets/images/power-save-mode-example-web.png)

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>
