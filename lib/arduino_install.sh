#!/bin/bash
arduino=/opt/arduino-1.8.9a

if ! [ -x $arduino ] ; then
sudo mkdir -p $arduino
cd /opt
sudo wget -c https://downloads.arduino.cc/arduino-1.8.9-linux64.tar.xz
sudo tar Jxvf arduino-1.8.9-linux64.tar.xz
fi
sudo mkdir -p $arduino/hardware/esp8266com
cd $arduino/hardware/esp8266com
sudo git clone https://github.com/esp8266/Arduino.git esp8266

cd esp8266
sudo git pull

