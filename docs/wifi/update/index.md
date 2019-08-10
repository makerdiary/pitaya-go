# Prepare for Setup

## Description

The Wi-Fi module(ATWINC1500B) features an on-chip microcontroller and integrated SPI Flash memory for the system firmware. The serial Flash memory also stores the root certificate required for the TLS/SSL connection and the power gain values used by the transceiver.

This section details the download procedure of firmware, TLS/SSL root certificates, and TX power gain values into WINC serial Flash.

!!! Note
	Before updating the firmware, use [Chip Info Example](../basic/chip-info-example.md) to check your current version. The latest Firmware Version is **19.6.1**. If it's up to date, there is no need to download the firmware.

## How it works

The following illustration shows how contents are downloaded into WINC serial Flash:

![](assets/images/download-procedure.png)

## Prerequisites

* Pitaya Go board
* A Windows 10 machine
* [nRF Connect for Desktop](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-desktop)
* USB-C Cable

## Clone the repository

Clone the `pitaya-go` repository from GitHub to your Windows 10 machine:

``` sh
git clone https://github.com/makerdiary/pitaya-go
```

This repository contains all the necessary tools to update the firmware. Note that all Wi-Fi tools only work on Windows platform.


## Next Steps

* [Firmware download procedure](download-firmware.md)
* [Root certificates download procedure](download-root-certs.md)
* [TLS certificates download procedure](download-tls-certs.md)
* [Customized provisioning webpage download](customize-provisioning-page.md)
* [Gain values download procedure](modify-gain-values.md)


## Reference

* [ATWINC15x0 Wi-Fi® Network Controller Software Design Guide](http://ww1.microchip.com/downloads/en/DeviceDoc/ATWINC15x0-Wi-Fi-Network-Controller-Software-Design-Guide-User-Guide-DS00002389B.pdf)

* [ATWINC15x0 Wi-Fi® Network Controller Software Programming Guide](http://ww1.microchip.com/downloads/en/DeviceDoc/ATWINC15x0-ATWINC3400-Wi-Fi-Network-Controller-Software-Programming-Guide-DS70005305B.pdf)

* [ATWINC15x0 Integrated Serial Flash and Memory Download Procedure](http://ww1.microchip.com/downloads/en/DeviceDoc/00002378B.pdf)

* [ATWINC15x0 Transport Layer Security (TLS) User's Guide](http://ww1.microchip.com/downloads/en/DeviceDoc/50002599A.pdf)

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>

