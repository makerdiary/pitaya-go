# Setup the nRF5 SDK Development Environment

This section describes how to set up the nRF5 SDK development environment on your system.

## Set up the toolchain

To build the nRF5 SDK applications, GNU ARM Embedded Toolchain is required. Follow one of the following guides for your host operating system:

* [macOS](#macos)
* [Windows](#windows)
* [Linux](#linux)

### macOS

1. GNU make is bundled with Xcode tools. Verify by running:
	``` sh
	make --v
	```

2. Download and install the [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). The `6-2017-q2-update` version is recommended. Then ensure the path is added to your OS PATH environment variable.

    ``` sh
    # in ~/.bash_profile, add the following script
    export PATH="<path to install directory>/gcc-arm-none-eabi-6-2017-q2-update/bin:${PATH}"
    ```
    Type the following in your terminal to verify if the path is set correctly:

    ``` sh
    arm-none-eabi-gcc --version
    ```

3. Download the [nRF5x-Command-Line-Tools-OSX](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools/Download#infotabs), then extract the `.tar` archive anywhere on your filesystem. Ensure the extracted directory is added to your OS PATH environment variable.

    ``` sh
    # in ~/.bash_profile, add the following script
    export PATH="<the path to the extracted directory>:${PATH}"
    ```

    Type the following in your terminal to verify if `mergehex` works:
    ``` sh
    mergehex --version
    ```

4. (Optional) Install the latest stable version of [pyOCD](https://github.com/mbedmicro/pyOCD) via `pip` as follows:

	``` sh
	pip install -U pyocd
	```

	Type the following in your terminal to verify if `pyocd` works:
	``` sh
	pyocd --version
	```

!!! tip
	pyOCD is an open source Python package for programming and debugging Arm Cortex-M microcontrollers using multiple supported types of USB debug probes. If you have a CMSIS-DAP debug probe, it's recommended to install pyOCD.	

### Windows

On Windows the easiest way to install the dependencies is to use the [MSYS2](http://www.msys2.org/). You can do so by performing the following steps:

1. Download and run the installer - "x86_64" for 64-bit, "i686" for 32-bit Windows.
	<a href="http://www.msys2.org/"><button data-md-color-primary="marsala">Download</button></a>

2. Start MSYS2. Update the package database and core system packages with:

	``` sh
	pacman -Syu
	```

3. If needed, close MSYS2, run it again from Start menu. Update the rest with:

	``` sh
	pacman -Su
	```

4. Install dependencies:

	``` sh
	pacman -S git make python2
	```

5. Download and install the [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). The `6-2017-q2-update` version is recommended. Run the installer and follow the given instructions. Upon completion, check the *Add path to environment variable* option. Then verify if the compiler works:

    ``` sh
    arm-none-eabi-gcc --version
    ```

6. Download the [nRF5x-Command-Line-Tools for Win32](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools/Download#infotabs). Run the installer and follow the given instructions. Then verify if `mergehex` works:

    ``` sh
    mergehex --version
    ```

7. (Optional) Install the latest stable version of [pyOCD](https://github.com/mbedmicro/pyOCD) via `pip` as follows:

	``` sh
	pip install -U pyocd
	```
	Type the following in your terminal to verify if `pyocd` works:
	``` sh
	pyocd --version
	```

### Linux

Here we describe how to set up the toolchain on Ubuntu. The steps should be similar for other Linux distributions.

1. Ensure your host system is up to date before proceeding.

    ``` sh
    sudo apt-get update
    ```
    ``` sh
    sudo apt-get upgrade
    ```

2. Install the following packages using your system’s package manager.

    ``` sh
    sudo apt-get install build-essential checkinstall
    ```

3. Download and install the [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). The `6-2017-q2-update` version is recommended. Then ensure the path is added to your OS PATH environment variable.

    ``` sh
    # in ~/.bash_profile, add the following script
    export PATH="<path to install directory>/gcc-arm-none-eabi-6-2017-q2-update/bin:${PATH}"
    ```
    Type the following in your terminal to verify if the path is set correctly:

    ``` sh
    arm-none-eabi-gcc --version
    ```

4. Download the [nRF5x-Command-Line-Tools-Linux-xxx](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools/Download#infotabs), then extract the `.tar` archive anywhere on your filesystem. Ensure the extracted directory is added to your OS PATH environment variable.

    ``` sh
    # in ~/.bash_profile, add the following script
    export PATH="<the path to the extracted directory>:${PATH}"
    ```

    Type the following in your terminal to verify if `mergehex` works:
    ``` sh
    mergehex --version
    ```

5. (Optional) Install the latest stable version of [pyOCD](https://github.com/mbedmicro/pyOCD) via `pip` as follows:

	``` sh
	pip install -U pyocd
	```

	Type the following in your terminal to verify if `pyocd` works:
	``` sh
	pyocd --version
	```

## Clone the repository

Clone the `pitaya-go` repository from GitHub:

``` sh
git clone --recursive https://github.com/makerdiary/pitaya-go
```

Or if you have already cloned the project, you may update the submodule:

``` sh
git submodule update --init
```

## Install the nRF5 SDK

Download the SDK file [nRF5_SDK_v15.2.0_9412b96](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download#infotabs) from [www.nordicsemi.com](https://www.nordicsemi.com). Note that the current version is `15.2.0`.

<a href="https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download#infotabs"><button data-md-color-primary="marsala">Download</button></a>

Extract the zip file to the `pitaya-go/nrf_sdks` directory. This should give you the following folder structure:

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

Open the file in a text editor ([Sublime Text](https://www.sublimetext.com/) is recommended), and make sure that the `GNU_INSTALL_ROOT` variable is pointing to your GNU Arm Embedded Toolchain install directory.

``` sh
GNU_INSTALL_ROOT ?= $(HOME)/gcc-arm-none-eabi/gcc-arm-none-eabi-6-2017-q2-update/bin/
GNU_VERSION ?= 6.3.1
GNU_PREFIX ?= arm-none-eabi
```

## Next Step

If everything works as expected, it's ready to build and run the examples to verify the development environment setup.

The steps that are described in [Build & Run a first example](build-n-run-a-first-example.md) will walk you through verifying your setup with the Hello World example.


## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/pitaya-go/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>
