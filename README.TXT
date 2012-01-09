This is the firmware source for the MakerBot Generation 4 board sets. 
This version includes changes made by Jetty840 and support for a 20x4
LCD display.

If you're getting started, please begin by reading the documentation for
the firmware:
http://wiki.makerbot.com/v2-firmware

This project contains the following subdirectories:
* firmware - this is the firmware, where active development takes place.
       See http://wiki.makerbot.com/v2-firmware for details on how to
       build and install the code.
* bootloader - this contains a slightly modified version of the Arduino
       bootloader for installing on older extruder controller boards.  It
       includes code to quickly shut down all mosfets to avoid possible
       damage during bootloader startup.
* dist - this directory contains tools and scripts for quickly installing
       bootloaders and board images on Piles O' Boards.
