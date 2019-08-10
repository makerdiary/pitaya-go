# How to Create Custom TLS Certificates

## Introduction

This guide explains the required procedures to create and sign custom certificates using OpenSSL. To use this guide you must install OpenSSL on your machine.

OpenSSL is an open-source implementation of the SSL and TLS protocols. The core library, written in the C programming language, implements basic cryptographic functions and provides various utility functions.

OpenSSL can be downloaded from the following URL: [https://www.openssl.org/source/](https://www.openssl.org/source/).

## Generate Certificates

After installing OpenSSL, perform the following steps:

1. Generate a key for the CA (certification authority). To generate a 2048-bit long RSA (creates a new file `PitayaRootCARSA.key` to store the random key), using the following command:

	```sh
	openssl genrsa -out PitayaRootCARSA.key 2048
	```

2. Create your self-signed root CA certificate `PitayaRoot.crt`. You need to provide some data for your Root certificate, using the following command:

	```sh
	openssl req -new -x509 -days 1826 -key PitayaRootCARSA.key -out PitayaRoot.crt
	```

3. Create the custom certificate, which is signed by the CA root certificate created earlier. First, generate the `pitaya_rsa.key`, using the following command:

	```sh
	openssl genrsa -out pitaya_rsa.key 2048
	```

4. To generate a certificate request file (CSR) using this generated key, use the following command:

	```sh
	openssl req -new -key pitaya_rsa.key -out pitaya_rsa.csr
	```

5. Process the request for the certificate and get it signed by the root CA, using the following command:

	```sh
	 openssl x509 -req -days 730 -in pitaya_rsa.csr -CA PitayaRoot.crt -CAkey PitayaRootCARSA.key -CAcreateserial -out pitaya_rsa.crt
	```

## Convert X.509 Certificates

The most known encodings for the X.509 digital certificates are PEM and DER formats.
The PEM format is base64 encoding of the DER enclosed with messages `-----BEGIN CERTIFICATE-----` and `-----END CERTIFICATE-----`.

The current implementation of the WINC `root_certificate_downloader` supports only DER format. If the certificate is not in DER format, it must be converted first.

The OpenSSL is used for certificate conversion by the following command:

```sh
openssl x509 -outform der -in PitayaRoot.crt -out PitayaRoot.cer
```

```sh
openssl x509 -outform der -in pitaya_rsa.crt -out pitaya_rsa.cer
```

## Download Certificates

Follow the sections below to download the certificates:

* [Download Root Certificates](update/download-root-certs.md)
* [Download TLS Certificates](update/download-tls-certs.md)


## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>



