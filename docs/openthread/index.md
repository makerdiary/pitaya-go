# OpenThread <br><small>An open-source implementation of the Thread networking protocol</small>

## Introduction

OpenThread is a portable and flexible open-source implementation of the [Thread®](http://threadgroup.org/) networking protocol, created by Nest in active collaboration with Nordic to accelerate the development of products for the connected home.

## Features
OpenThread implements all Thread networking layers (IPv6, 6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, Mesh Routing) and device roles, as well as Border Router support.

* Application Services
	* IPv6 configuration and raw data interface
	* UDP sockets
	* CoAP client and server
	* DHCPv6 client and server
	* DNSv6 client
* Enhanced Features
	* Child Supervision
	* Inform Previous Parent on Reattach
	* Jam Detection
	* Periodic Parent Search
* NCP Support
	* [Spinel](https://github.com/openthread/openthread/tree/master/src/ncp), a general purpose NCP protocol
	* [wpantund](https://github.com/openthread/wpantund), a user-space NCP network interface driver/daemon
	* Sniffer support via NCP Spinel nodes
* Border Router
	* Web UI for configuration and management
	* Thread Border Agent to support an External Commissioner
	* NAT64 for connecting to IPv4 networks
	* Thread interface driver using wpantund

## OpenThread Architecture
OpenThread's portable nature makes no assumptions about platform features. OpenThread provides the hooks to utilize enhanced radio and crypto features, reducing system requirements, such as memory, code, and compute cycles. This can be done per platform, while retaining the ability to default to a standard configuration. See [OpenThread platform designs](https://infocenter.nordicsemi.com/topic/sdk_tz_v3.1.0/thread_architectures.html) for more information.

![](assets/images/ot-arch_2x.png)

## Resources

* [OpenThread GitHub repository](https://github.com/openthread/openthread)
* [openthread.io](https://openthread.io/)
* [nRF5 SDK for Thread Documentation](https://infocenter.nordicsemi.com/topic/sdk_tz_v3.1.0/thread_main.html)

## License

Portions of this page are reproduced from work created and [shared by Google](https://developers.google.com/readme/policies/) and used according to terms described in the [Creative Commons 3.0 Attribution License](https://creativecommons.org/licenses/by/3.0/).

Thread is a registered trademark of the Thread Group, Inc.

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>

