# How to Modify TX Power Gain

## Description

Gain setting values are used by the RF with different rates to configure the transmission power.

The gain values are downloaded as part of the complete download process explained in [How to Download Wi-Fi Firmware](download-firmware.md) section. It is impossible to download the gain values alone.

!!! Tip
	Before you start download, see [Prepare for Setup](index.md) for prerequisites.

## Steps

Perform the following steps to modify the TX power gain values:

1. The `.csv` file in the `utils\wifi\Tools\gain_builder\gain_sheets` folder holds the gain values. It must be sorted based on the gain rates (Row) and for all the channels (Column).

2. Modify the values in the default `.csv` file.

3. Follow the steps mentioned in [How to Download Wi-Fi Firmware](download-firmware.md) to download the new gain values.

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>

