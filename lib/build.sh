#!/bin/bash
cd `dirname $0`
cd ..
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver
mkdir -p /tmp/build
chmod 777 /tmp/build
arduino=/opt/arduino-1.8.5
home=/home/liushiwei
sketchbook=./wifi_disp
mkdir -p /tmp/build /tmp/cache
chmod 777 /tmp/build /tmp/cache
chown liushiwei /tmp/build /tmp/cache
$arduino/arduino-builder -dump-prefs -logger=machine -hardware "$arduino/hardware" -hardware "$home/.arduino15/packages" -tools "$arduino/tools-builder" -tools "$arduino/hardware/tools/avr" -tools "$home/.arduino15/packages" -built-in-libraries "$arduino/libraries" -libraries "$home/sketchbook/libraries" \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,CpuFrequency=80,FlashSize=4M3M,LwIPVariant=v2mss536,Debug=Disabled,DebugLevel=None____,FlashErase=none,UploadSpeed=115200 \
-ide-version=10805 -build-path /tmp/build -warnings=none -build-cache /tmp/cache -prefs=build.warn_data_percentage=75 -verbose "$sketchbook/wifi_disp.ino"

$arduino/arduino-builder -compile -logger=machine -hardware "$arduino/hardware" -hardware "$home/.arduino15/packages" -tools "$arduino/tools-builder" -tools "$arduino/hardware/tools/avr" -tools "$home/.arduino15/packages" -built-in-libraries "$arduino/libraries" -libraries "$home/sketchbook/libraries" \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,CpuFrequency=80,FlashSize=4M3M,LwIPVariant=v2mss536,Debug=Disabled,DebugLevel=None____,FlashErase=none,UploadSpeed=115200 \
-ide-version=10609 -build-path "/tmp/build" -build-cache /tmp/cache -warnings=none -prefs=build.warn_data_percentage=75 -verbose "$sketchbook/wifi_disp.ino"|tee /tmp/build/info.log
if [ $? == 0 ] ; then
 chown -R liushiwei /tmp/build /tmp/cache
 chmod -R og+w /tmp/build /tmp/cache
 sync
 grep "Global vari" /tmp/build/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "内存：使用"$1"字节,"$3"%,剩余:"$4"字节"}'
 grep "Sketch uses" /tmp/build/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "ROM：使用"$1"字节,"$3"%"}'

 cp -a /tmp/build/wifi_disp.ino.bin lib/wifi_disp.bin
 if [ "a%1" != "a"  ] ;then
  $arduino/hardware/esp8266com/esp8266/tools/espota.py -i $1 -f lib/wifi_disp.bin
 else
  $arduino/hardware/esp8266com/esp8266/tools/esptool/esptool -vv -cd nodemcu -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf lib/wifi_disp.bin
 fi
fi
