#!/bin/bash
avrdude -C/etc/avrdude.conf -v -patmega328p -carduino -P/dev/ttyUSB0 -b57600 -D -Uflash:w:/tmp/build/lora_mini.ino.hex:i 
