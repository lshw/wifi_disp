#!/bin/bash
cd `dirname $0`

me=`whoami`
if [ "$me" == "root" ] ; then
  home=/home/liushiwei
else
  home=~
fi

if [ -x $home/sketchbook/libraries ] ; then
 sketchbook=$home/sketchbook
else
 sketchbook=$home/Arduino
fi

arduino=/opt/arduino-1.8.19
astyle  --options=$arduino/lib/formatter.conf ../wifi_disp/*.h ../wifi_disp/*.ino

if ! [ -x ./uncrc32 ] ; then
gcc -o uncrc32 uncrc32.c
fi
cd ..
CRC_MAGIC=$( grep CRC_MAGIC wifi_disp/config.h | awk '{printf $3}' )
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver

arduinoset=$home/.arduino15
mkdir -p /tmp/${me}_build /tmp/${me}_cach

#传递宏定义 GIT_VER 到源码中，源码git版本
CXXFLAGS="-DGIT_VER=\"$ver\" "
fqbn="esp8266:esp8266:espduino:ResetMethod=v1,UploadTool=esptool,xtal=160,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=4816,non32xfer=fast,eesz=4M2M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=460800 " 
$arduino/arduino-builder \
-dump-prefs \
-logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries $sketchbook/libraries \
-fqbn=$fqbn \
-ide-version=10819 \
-build-path /tmp/${me}_build \
-warnings=none \
-prefs build.extra_flags="$CXXFLAGS" \
-build-cache /tmp/${me}_cache \
-prefs=build.warn_data_percentage=75 \
-verbose \
./wifi_disp/wifi_disp.ino
if [ $? != 0 ] ; then
  exit
fi
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
-fqbn=$fqbn \
-ide-version=10819 \
-build-path /tmp/${me}_build \
-warnings=none \
-prefs build.extra_flags="$CXXFLAGS" \
-build-cache /tmp/${me}_cache \
-prefs=build.warn_data_percentage=75 \
-verbose \
./wifi_disp/wifi_disp.ino |tee /tmp/${me}_info.log

if [ $? == 0 ] ; then
  grep "Global vari" /tmp/${me}_info.log |sed -n "s/^.* \[\([0-9]*\) \([0-9]*\) \([0-9]*\) \([0-9]*\)\].*$/RAM:使用\1字节(\3%),剩余\4字节/p"
  grep "Sketch uses" /tmp/${me}_info.log |sed -n "s/^.* \[\([0-9]*\) \([0-9]*\) \([0-9]*\)\].*$/ROM:使用\1字节(\3%)/p"

  cp -a /tmp/${me}_build/wifi_disp.ino.bin lib/wifi_disp.bin
lib/uncrc32 lib/wifi_disp.bin $CRC_MAGIC
 if [ "a$1" != "a"  ] ;then
  $arduino/hardware/esp8266com/esp8266/tools/espota.py -p 8266 -i $1 -f lib/wifi_disp.bin
 fi
fi
