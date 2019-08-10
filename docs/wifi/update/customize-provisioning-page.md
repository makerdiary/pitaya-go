# How to Customize Provisioning Page

## Description

The WINC device features a Provisioning Webpage mode that can be used to enter user credentials to connect the WINC device to the desired Access Point. The HTTP server and the actual HTML provisioning webpage is embedded in the WINC firmware, therefore, it cannot be modified from the host MCU. 

The Wi-Fi utils include the HTML code used by the WINC for HTTP provisioning and also the necessary scripts to generate a new WINC firmware image with the modifed provisioning web page.

!!! Tip
	Before you start download, see [Prepare for Setup](index.md) for prerequisites.

## Steps

Perform the following steps to download customized provisioning webpage:

1. Modify the HTML code in `utils/wifi/provisioning_webpage` folder as required.

2. Open CMD and navigate to the directory with the Wi-Fi utils:

	``` sh
	cd .\pitaya-go\utils\wifi
	```

3. Run `modify_provisioning_webpage.bat` script to generate a new WINC firmware image that
includes the customized HTML code.

	``` sh
	modify_provisioning_webpage.bat
	```

4. The generated new firmware image overwrites the default firmware image located in the `utils/wifi/firmware` folder. The following message appears to indicate that the new firmware is successfully generated.

	![](assets/images/modify-provisioning-webpage.jpg)

5. Follow the steps mentioned in [How to Download Wi-Fi Firmware](download-firmware.md) to update the new firmware.


## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>

