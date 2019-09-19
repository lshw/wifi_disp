#!/bin/bash
cd `dirname $0`
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver
arduino=/opt/arduino-1.8.9
arduinoset=~/.arduino15
sketchbook=~/sketchbook
mkdir -p /tmp/build /tmp/cache
chmod 777 /tmp/build /tmp/cache
chown liushiwei /tmp/build /tmp/cache
touch /tmp/build/info.log
$arduino/arduino-builder -dump-prefs -logger=machine -hardware $arduino/hardware -hardware $arduinoset/packages -tools $arduino/tools-builder -tools $arduino/hardware/tools/avr -tools $arduinoset/packages -built-in-libraries $arduino/libraries -libraries $sketchbook$/libraries \
-fqbn=arduino:avr:pro:cpu=8MHzatmega168 \
-ide-version=10809 -build-path /tmp/build -warnings=none -build-cache /tmp/cache -prefs=build.warn_data_percentage=75 \
-prefs=runtime.tools.avr-gcc.path=$arduinoset/packages/arduino/tools/avr-gcc/5.4.0-atmel3.6.1-arduino2 \
-prefs=runtime.tools.avr-gcc-5.4.0-atmel3.6.1-arduino2.path=$arduinoset/packages/arduino/tools/avr-gcc/5.4.0-atmel3.6.1-arduino2 \
-prefs=runtime.tools.avrdude.path=$arduinoset/packages/arduino/tools/avrdude/6.3.0-arduino12 \
-prefs=runtime.tools.avrdude-6.3.0-arduino12.path=$arduinoset/packages/arduino/tools/avrdude/6.3.0-arduino12 \
-prefs=runtime.tools.arduinoOTA.path=$arduinoset/packages/arduino/tools/arduinoOTA/1.2.0 \
-prefs=runtime.tools.arduinoOTA-1.2.0.path=$arduinoset/packages/arduino/tools/arduinoOTA/1.2.0 \
-verbose lora_mini.ino

$arduino/arduino-builder -compile -logger=machine -hardware $arduino/hardware -hardware $arduinoset/packages -tools $arduino/tools-builder -tools $arduino/hardware/tools/avr -tools $arduinoset/packages -built-in-libraries $arduino/libraries -libraries $sketchbook/libraries \
-fqbn=arduino:avr:pro:cpu=8MHzatmega168 \
-ide-version=10809 -build-path /tmp/build -warnings=none -build-cache /tmp/cache \
-prefs=build.warn_data_percentage=75 -prefs=runtime.tools.avr-gcc.path=$arduinoset/packages/arduino/tools/avr-gcc/5.4.0-atmel3.6.1-arduino2 \
-prefs=runtime.tools.avr-gcc-5.4.0-atmel3.6.1-arduino2.path=$arduinoset/packages/arduino/tools/avr-gcc/5.4.0-atmel3.6.1-arduino2 \
-prefs=runtime.tools.avrdude.path=$arduinoset/packages/arduino/tools/avrdude/6.3.0-arduino12 \
-prefs=runtime.tools.avrdude-6.3.0-arduino12.path=$arduinoset/packages/arduino/tools/avrdude/6.3.0-arduino12 \
-prefs=runtime.tools.arduinoOTA.path=$arduinoset/packages/arduino/tools/arduinoOTA/1.2.0 \
-prefs=runtime.tools.arduinoOTA-1.2.0.path=$arduinoset/packages/arduino/tools/arduinoOTA/1.2.0 \
-verbose lora_mini.ino |tee -a /tmp/build/info.log

if [ $? == 0 ] ; then
 chown -R liushiwei /tmp/build /tmp/cache
 chmod -R og+w /tmp/build /tmp/cache
 sync
 grep "Global vari" /tmp/build/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "内存：使用"$1"字节,"$3"%,剩余:"$4"字节"}'
 grep "Sketch uses" /tmp/build/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "ROM：使用"$1"字节,"$3"%"}'

 cp -a /tmp/build/lora_mini.ino.hex ../lib/lora_mini.hex
fi
