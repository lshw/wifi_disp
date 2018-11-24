#!/bin/bash
arduino=/opt/arduino-1.8.5
if [ "a$1" != "a" ] ; then
$arduino/hardware/esp8266com/esp8266/tools/espota.py -i $1 -f lib/wifi_disp.bin 
else
killall minicom 2>/dev/null
sleep 3
$arduino/hardware/esp8266com/esp8266/tools/esptool/esptool -vv -cd nodemcu -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf lib/wifi_disp.bin
fi
