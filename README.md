# Audio Level Meter for rp2040

![LevelMeterScene](doc/level_meter_scene01.jpg)

## Overview
This is Audio Level Meter library for rp2040

This project feattures:
* Analog signal input for multiple channels (max 3 channels)
* Configurable dB scale steps and levels

## Supported Board and Peripheral Devices
* Raspberry Pi Pico 2
* M62429 / FM62429 Electric Volume
* ST7735S 80x160 LCD

## Pin Assignment
### Audio Input

| Pin # | Pin Name | Function | Signal Name |
----|----|----|----
|31 | GP26 | ADC0 | L_IN |
|32 | GP27 | ADC1 | R_IN |

### M62429 / FM62429 Electric Volume

| Pin # | Pin Name | Function | Signal Name |
----|----|----|----
|19 | GP14 | GPIO | PIN_FM62429_DATA |
|20 | GP15 | GPIO | PIN_FM62429_CLOCK |

### ST7735S 80x160 LCD (Waveshare RP2040-LCD-0.96 Board)

| Pin # | Pin Name | Function | Signal Name |
----|----|----|----
|11 | GP8 | GPIO | DC |
|12 | GP9 | SPI1_CSn | CS |
|14 | GP10 | SPI1_SCK | SCL |
|15 | GP11 | SPI1_TX | DIN |
|16 | GP12 | GPIO | RST |
|-- | GP25 | GPIO | BL |

## Schematic
The frontend analog circuit should be needed.

[pico_level_meter.pdf](doc/pico_level_meter.pdf)

## How to build
* See ["Getting started with Raspberry Pi Pico"](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
* Put "pico-sdk", "pico-examples" and "pico-extras" on the same level with this project folder.
* Set environmental variables for PICO_SDK_PATH, PICO_EXTRAS_PATH and PICO_EXAMPLES_PATH
* Confirmed with Pico SDK 2.1.0
```
> git clone -b 2.1.0 https://github.com/raspberrypi/pico-sdk.git
> cd pico-sdk
> git submodule update -i
> cd ..
> git clone -b sdk-2.1.0 https://github.com/raspberrypi/pico-examples.git
>
> git clone -b sdk-2.1.0 https://github.com/raspberrypi/pico-extras.git
> 
> git clone -b main https://github.com/elehobica/pico_level_meter.git
> cd pico_level_meter
> git submodule update -i
> cd ..
```
### Windows
* Build is confirmed with Developer Command Prompt for VS 2022 and Visual Studio Code on Windows environment
* Confirmed with cmake-3.27.2-windows-x86_64 and gcc-arm-none-eabi-10.3-2021.10-win32
* Lanuch "Developer Command Prompt for VS 2022"
```
> cd pico_level_meter
> mkdir build && cd build
> cmake -G "NMake Makefiles" ..
> nmake
```
* Put "pico_level_meter.uf2" on RPI-RP2 drive
### Linux
* Build is confirmed with [pico-sdk-dev-docker:sdk-2.1.0-1.0.0]( https://hub.docker.com/r/elehobica/pico-sdk-dev-docker)
* Confirmed with cmake-3.22.1 and arm-none-eabi-gcc (15:10.3-2021.07-4) 10.3.1
```
$ cd pico_level_meter
$ mkdir build && cd build
$ cmake ..
$ make -j4
```
* Download "pico_level_meter.uf2" on RPI-RP2 drive



* See ["Getting started with Raspberry Pi Pico"](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
* Put "pico-sdk", "pico-examples" and "pico-extras" on the same level with this project folder.
* Set environmental variables for PICO_SDK_PATH, PICO_EXTRAS_PATH and PICO_EXAMPLES_PATH
* Build is confirmed in Developer Command Prompt for VS 2022 and Visual Studio Code on Windows enviroment
* Confirmed with Pico SDK 1.5.1, cmake-3.27.2-windows-x86_64 and gcc-arm-none-eabi-10.3-2021.10-win32
```
> git clone -b master https://github.com/raspberrypi/pico-sdk.git
> cd pico-sdk
> git submodule update -i
> cd ..
> git clone -b master https://github.com/raspberrypi/pico-examples.git
> 
> git clone -b main https://github.com/elehobica/pico_level_meter.git
> cd pico_level_meter
> git submodule update -i
> cd ..
```
* Lanuch "Developer Command Prompt for VS 2022"
```
> cd pico_level_meter
> mkdir build
> cd build
> cmake -G "NMake Makefiles" ..
> nmake
```
* Put "pico_level_meter.uf2" on RPI-RP2 drive

### Serial interface usage
* type ' ' to display current settings
* type 's' to store current settings to Flash
* type '+' or '=' to decrease attenuation (level up)
* type '-' to increase attenuation (level down)
* type 'b' to adjust attenuation for both channels
* type 'l' to adjust attenuation for left channel
* type 'r' to adjust attenuation for right channel
* type 'p' to toggle peak hold mode