TFTP Network bootloader for ARM Cortex and Wiznet W5500
=======================================================

This bootloader allows a Cortex processor to load and flash a firmware image via TFTP.

Steps:
1. Init W5500
2. Get address via DHCP
3. Fetch firmware from a TFTP server (it only flashes modified rows)
4. Boot into the new firmware

Tested with a Adafruit [Feather M0 Basic Proto](https://www.adafruit.com/product/2772) and [Ethernet FeatherWing](https://www.adafruit.com/product/3201).

This work is based on the [Industruino bootloader](https://github.com/Industruino/IndustruinoSAMD/tree/master/bootloaders/d21g)

How to build:
1. Make sure you have Docker installed & happy
2. `./make.sh`
  * The first time will be slow since it needs to build the docker container
3. `./make.sh install` will use openocd to install via jlink

