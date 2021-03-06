#!/bin/bash
cd `dirname $0`
if ! [ -x ./uncrc32 ] ; then
gcc -o uncrc32 uncrc32.c
fi
cd ..
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver
#echo "#define GIT_COMMIT_ID \"$ver\"" > wifi_disp/commit.h

arduino=/opt/arduino-1.8.12
arduinoset=~/.arduino15
sketchbook=~/sketchbook
mkdir -p /tmp/build_wifi_disp /tmp/cache_wifi_disp
chown liushiwei /tmp/build_wifi_disp /tmp/cache_wifi_disp
$arduino/arduino-builder -dump-prefs -logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries $sketchbook/libraries \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=80,vt=flash,exception=legacy,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 \
-ide-version=10812 \
-build-path /tmp/build_wifi_disp \
-warnings=none \
-build-cache /tmp/cache_wifi_disp \
-prefs=build.warn_data_percentage=75 \
-verbose \
./wifi_disp/wifi_disp.ino

$arduino/arduino-builder \
-compile \
-logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries $sketchbook/libraries \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=80,vt=flash,exception=legacy,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 \
-ide-version=10812 \
-build-path /tmp/build_wifi_disp \
-warnings=none \
-build-cache /tmp/cache_wifi_disp \
-prefs=build.warn_data_percentage=75 \
-verbose \
./wifi_disp/wifi_disp.ino \
> /tmp/info_wifi_disp.log
if [ $? = 0 ] ; then
#/opt/arduino-1.8.12/hardware/esp8266com/esp8266/tools/xtensa-lx106-elf/bin/xtensa-lx106-elf-size /tmp/build_wifi_disp/wifi_disp.ino.elf |head -n 5
 grep "Global vari" /tmp/info_wifi_disp.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "内存：使用"$1"字节,"$3"%,剩余:"$4"字节"}'
 grep "Sketch uses" /tmp/info_wifi_disp.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "ROM：使用"$1"字节,"$3"%"}'

 cp -a /tmp/build_wifi_disp/wifi_disp.ino.bin lib/wifi_disp.bin
 lib/uncrc32 lib/wifi_disp.bin 0
 if [ "a$1" != "a"  ] ;then
  $arduino/hardware/esp8266com/esp8266/tools/espota.py -p 8266 -i $1 -f lib/wifi_disp.bin
 fi
fi
