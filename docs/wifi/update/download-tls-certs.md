# How to Download TLS Certificates

## Description

For proper operation of both the TLS server and TLS client authentication, the ATWINC1500B device must have a certificate/private key pair assigned to it.

An 8KB flash area is reserved for storing the TLS certificates starting from offset 20KB in the ATWINC1500B stacked flash.

The TLS certificates can be downloaded into the ATWINC1500B stacked flash by using the dedicated tool `tls_cert_flash_tool.exe`.

!!! Tip
	Before you start download, see [Prepare for Setup](index.md) for prerequisites.

## Steps

Perform the following steps to download the custom TLS certificates:

1. Program Pitaya Go with the serial bridge firmware: **[serial_bridge_usb_pitaya_go_vx.x.x.hex](https://github.com/makerdiary/pitaya-go/tree/master/firmware/wifi/)**.

2. Connect Pitaya Go to a Windows computer. Check the comport number of the device detected.

3. Generate the custom certificate/private key pair. See [How to Create Custom TLS Certificates](../../create-tls-certificates) for more details.

4. Open CMD and navigate to the directory with *tls_cert_flash_tool.exe*:

	``` sh
	cd .\pitaya-go\utils\wifi\Tools\tls_cert_flash_tool\Debug_UART\
	```

5. Run the following scripts to download the certificates:
	
	``` sh
	tls_cert_flash_tool.exe write -port {comport} -key {private_key_path} -cert {rsa_cert_path} -cadir {ca_directory} -erase
	```
	If an ECDSA certificate is available, go on to run:
	``` sh
	tls_cert_flash_tool.exe write -port {comport} -nokey -cert {ecsda_cert_path} -cadir {ca_directory}
	```
	The parameters stand for the following:

	* `{comport}`: Number of the serialport (e.g. `36` represents `COM36`).
	* `{private_key_path}`: Path of the TLS Private Key.
	* `{rsa_cert_path}`: Path of RSA based Certificate.
	* `{ecdsa_cert_path}`: Path of ECC based Certificate.
	* `{ca_directory}`: A directory containing intermediate CA certificates and/or the Root CA certificate of the ATWINC1500B certificate chain(s).

6. After a few seconds, the following message appears to indicate that the root certificates are successfully downloaded.

	![](assets/images/download-tls-certificates.jpg)

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>

