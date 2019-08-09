## Install the nRF5 SDK

Download the SDK file [nRF5_SDK_v15.2.0_9412b96](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download#infotabs) from [www.nordicsemi.com](https://www.nordicsemi.com). Note that the current version is `15.2.0`.

Extract the zip file to this directory. This should give you the following folder structure:

``` sh
./pitaya-go/
├── LICENSE
├── README.md
├── certs
├── config
├── docs
├── examples
├── external
├── firmware
├── material
├── mkdocs.yml
├── nrf_sdks
│   ├── README.md
│   └── nRF5_SDK_15.2.0_9412b96
└── utils
```

To use the nRF5 SDK you first need to set the toolchain path in `makefile.windows` or `makefile.posix` depending on platform you are using. That is, the `.posix` should be edited if your are working on either Linux or macOS. These files are located in:

``` sh
<nRF5 SDK>/components/toolchain/gcc
```

Open the file in a text editor ([Sublime](https://www.sublimetext.com/) is recommended), and make sure that the `GNU_INSTALL_ROOT` variable is pointing to your GNU Arm Embedded Toolchain install directory.

``` sh
GNU_INSTALL_ROOT ?= $(HOME)/gcc-arm-none-eabi/gcc-arm-none-eabi-6-2017-q2-update/bin/
GNU_VERSION ?= 6.3.1
GNU_PREFIX ?= arm-none-eabi
```

## Install the nRF5 SDK for Thread and Zigbee

If you want to compile the Thread/Zigbee examples, the nRF5 SDK for Thread and Zigbee is needed. The following steps show you how to install and set up the SDK.

Download the SDK file [nRF5 SDK for Thread and Zigbee V3.1.0](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK-for-Thread-and-Zigbee/Download#infotabs) from [www.nordicsemi.com](https://www.nordicsemi.com).

Extract the zip file to this directory. This should give you the following folder structure:

``` sh
./pitaya-go/
├── LICENSE
├── README.md
├── certs
├── config
├── docs
├── examples
├── external
├── firmware
├── material
├── mkdocs.yml
├── nrf_sdks
│   ├── README.md
│   ├── nRF5SDKforThreadandZigbeev310c7c4730
│   └── nRF5_SDK_15.2.0_9412b96
└── utils
```

To use this SDK you first need to set the toolchain path in `makefile.windows` or `makefile.posix` depending on platform you are using. That is, the `.posix` should be edited if your are working on either Linux or macOS. These files are located in:

``` sh
<nRF5 SDK for Thread and Zigbee>/components/toolchain/gcc
```

Open the file in a text editor, and make sure that the `GNU_INSTALL_ROOT` variable is pointing to your GNU Arm Embedded Toolchain install directory.

``` sh
GNU_INSTALL_ROOT ?= $(HOME)/gcc-arm-none-eabi/gcc-arm-none-eabi-7-2018-q2-update/bin/
GNU_VERSION ?= 7.3.1
GNU_PREFIX ?= arm-none-eabi
```
