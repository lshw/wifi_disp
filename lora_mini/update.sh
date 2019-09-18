#!/bin/bash
avrdude -C/etc/avrdude.conf -v -patmega168 -carduino -P/dev/ttyUSB0 -b19200 -D -Uflash:w:/tmp/build/lora_mini.ino.hex:i 
