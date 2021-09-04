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

arduino=/opt/arduino-1.8.13
arduinoset=$home/.arduino15
mkdir -p /tmp/${me}_build /tmp/${me}_cach

#传递宏定义 GIT_COMMIT_ID 到源码中，源码git版本
CXXFLAGS="-DGIT_COMMIT_ID=\"$branch-$ver\" "

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
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=160,vt=flash,exception=disabled,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=460800 \
-ide-version=10813 \
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
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=160,vt=flash,exception=disabled,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=460800 \
-ide-version=10813 \
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
lib/uncrc32 lib/wifi_disp.bin 0
 if [ "a$1" != "a"  ] ;then
  $arduino/hardware/esp8266com/esp8266/tools/espota.py -p 8266 -i $1 -f lib/wifi_disp.bin
 fi
fi
