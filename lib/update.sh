#!/bin/bash
#update.sh 192.168.1.2
arduino=/opt/arduino-1.8.9
if [ "a$1" != "a" ] ; then
lib/espota.py -f lib/wifi_disp.bin -i $1
else
killall minicom 2>/dev/null
sleep 3
$arduino/hardware/esp8266com/esp8266/tools/esptool/esptool -vv -cd nodemcu -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf lib/wifi_disp.bin
fi
