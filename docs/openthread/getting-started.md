# Getting Started with OpenThread

## Overview

This section describes how to quickly run an OpenThread example without going through the complete toolchain installation. 

There is no need to build any firmware for running it, as it uses the pre-built firmware of [OpenThread CLI](https://github.com/makerdiary/pitaya-go/tree/master/firmware/openthread/cli). This application demonstrates a minimal OpenThread application that exposes the OpenThread configuration and management interfaces via a basic command-line interface.

## Hardware Requirements

A minimal OpenThread Mesh network requires at least two nodes. Before starting to work, prepare the following parts:

* Two or more Pitaya Go boards
* A macOS, Linux or Windows computer

## Flashing the OpenThread CLI

Before running the OpenThread CLI application, you need to flash the boards. The pre-built firmware is located in 
[pitaya-go/firmware/openthread/cli](https://github.com/makerdiary/pitaya-go/tree/master/firmware/openthread/cli) folder.

1. Connect one Pitaya Go board to your PC using the USB-C cable. 

2. While pushing the **USER** button, press the **RESET** button to enter the DFU (Device Firmware Update) mode. Then program the firmware using the [nRF Connect for Desktop](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-desktop) tool.

3. Label the board **Node#1** so that later you don't confuse the boards.

4. Program the other board by repeating steps as described above. Label them **Node#2**

!!! Tip
	See **[Programming](../programming.md)** section for details about how to program your Pitaya Go.

## Running the OpenThread CLI

You can access the OpenThread CLI by using a serial terminal like [screen](https://www.gnu.org/software/screen/manual/screen.html) or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html). 

### Start Node#1

1. Open a terminal window and run:

	``` sh
	screen /dev/cu.usbmodemD2E39D222D781 115200
	```
	where `/dev/cu.usbmodemD2E39D222D781` is the port name of the board. Replace it with your owns.

2. Press <kbd>Enter</kbd> on the keyboard to bring up the OpenThread CLI `>` prompt.

3. Generate, view, and commit a new Active Operational Dataset:

	``` sh
	> dataset init new
	Done
	> dataset
	Active Timestamp: 1
	Channel: 24
	Channel Mask: 07fff800
	Ext PAN ID: 57a7486ff74c8538
	Mesh Local Prefix: fda9:4820:7a9c:66ba/64
	Master Key: aa8da3931ffcbcb26c8b4b7c9b4114c1
	Network Name: OpenThread-4d92
	PAN ID: 0x4d92
	PSKc: 22537a67d4040e34848d6d82ca2c313c
	Security Policy: 0, onrcb
	Done
	> dataset commit active
	Done
	```

4. Bring up the IPv6 interface:

	``` sh
	> ifconfig up
	Done
	```

5. Start Thread protocol operation. Observe that the GREEN LED is lit.

	``` sh
	> thread start
	Done
	```

6. Wait a few seconds and verify that the device has become a Thread Leader:

	``` sh
	> state
	leader
	Done
	```

7. View IPv6 addresses assigned to **Node#1**'s Thread interface:

	``` sh
	> ipaddr
	fda9:4820:7a9c:66ba:0:ff:fe00:fc00
	fda9:4820:7a9c:66ba:0:ff:fe00:8800
	fda9:4820:7a9c:66ba:4d7b:9ef4:cbdf:8cd3
	fe80:0:0:0:3c1d:5115:ccc5:6ba7
	Done
	```


### Start Node#2

1. Start another terminal window and enter the CLI.

2. Configure *Thread Master Key* and *PAN ID* from **Node#1**'s Active Operational Dataset:

	``` sh
	> dataset masterkey aa8da3931ffcbcb26c8b4b7c9b4114c1
	Done
	> dataset panid 0x4d92
	Done
	> dataset commit active
	Done
	```

3. Bring up the IPv6 interface:

	``` sh
	> ifconfig up
	Done
	```

4. Start Thread protocol operation. Observe that the GREEN LED is lit.

	``` sh
	> thread start
	Done
	```

5. Wait a few seconds and verify that **Node#2** has become a Thread Child or Router:

	``` sh
	> state
	child
	Done
	```

### Ping Node#1 from Node#2

In the CLI of **Node#2**, ping **Node#1**:

``` sh
> ping fda9:4820:7a9c:66ba:4d7b:9ef4:cbdf:8cd3
16 bytes from fda9:4820:7a9c:66ba:4d7b:9ef4:cbdf:8cd3: icmp_seq=1 hlim=64 time=87ms
```

Observe that the LEDs on **Node#1** will blink when receiving the ping request.


## Next Steps

Congratulations! You have built a simple OpenThread Mesh network with two Pitaya Go boards. 

You may use the CLI to change network parameters, other configurations, and perform other operations. See the [OpenThread CLI Reference README.md](https://github.com/makerdiary/openthread/blob/master/src/cli/README.md) to explore more.

Follow the [Setup the Thread SDK](setup-the-thread-sdk.md) section to set up the Thread development environment on your system. 

Check out the [Build and Run a Thread example](build-n-run-thread-example.md) guide to build a Thread example.

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>
