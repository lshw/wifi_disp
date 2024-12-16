#!/bin/bash
which arduino-cli
if [ $? != 0 ] ; then
 echo 没有找到 arduino-cli
 echo 请到https://github.com/arduino/arduino-cli/releases 下载， 并放到 /usr/local/bin目录下
 exit
fi
project=$( basename $( dirname $( realpath $0 )))
echo $project

cd $( dirname $0 )



CRC_MAGIC=$( grep CRC_MAGIC config.h | awk '{printf $3}' )
cd ..
rm -f $project/${project}.bin

if ! [ -x lib/uncrc32 ] ; then
gcc -o lib/uncrc32 lib/uncrc32.c
fi
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
git_id=${a:0:7}
ver=$date-${a:0:7}
echo $ver

me=$( whoami )
build=/tmp/${me}_${project}_build
cache=/tmp/${me}_${project}_cache
mkdir -p $build $cache
rm -f $build/${project}.ino.bin


#debug输出设置
if [ "a$debug" == "a" ] ; then
debug=",dbg=Disabled,lvl=None____"
#debug=",dbg=Serial,lvl=WIFI"
#debug=",dbg=Serial,lvl=SSLTLS_MEMHTTP_CLIENTHTTP_SERVERCOREWIFIHTTP_UPDATEUPDATEROTAOOMMDNSHWDT"
fi
fqbn="esp8266:esp8266:generic:xtal=160,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=4816,non32xfer=fast,CrystalFreq=26,FlashFreq=80,FlashMode=qio,eesz=4M2M,led=2,sdk=nonosdk305,ip=hb2f$debug,wipe=none,baud=921600"
#传递宏定义 GIT_COMMIT_ID 到源码中，源码git版本
CXXFLAGS=" -DGIT_COMMIT_ID=\"$git_id\" -DGIT_VER=\"$ver\" -DBUILD_SET=\"$fqbn\" "

#esp8266用 extra_flags esp32c3 用defines
arduino-cli compile \
--fqbn $fqbn \
--verbose \
--build-property compiler.c.extra_flags="$CXXFLAGS" \
--build-property compiler.cpp.extra_flags="$CXXFLAGS" \
--build-path $build \
--build-cache-path $cache \
$project 2>&1 |tee /tmp/${me}_info.log 
if [ -e $build/${project}.ino.bin ] ; then
#esp8266
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used \([0-9]*\) .* (\([0-9]*\)%).*$/RAM:使用\1字节(\2%)/p"
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^. Code in flash (default, ICACHE_FLASH_ATTR), used \([0-9]*\) .* (\(39\)%)$/ROM:使用\1字节(\1%)/p"
#esp32-c3
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^Global variables use \([0-9]*\) bytes (\([0-9]*\)%) of dynamic memory, leaving \([0-9]*\) bytes for local variables. .*$/RAM:使用\1字节(\2%),全局变量:\2字节/p"
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^Sketch uses \([0-9]*\) bytes (\([0-9]*\)%) of program storage space. Maximum is.*$/ROM:使用\1字节(\2%)/p"

  cp -a $build/${project}.ino.bin ${project}/${project}.bin
  if [ -x $build/${project}.ino.boorloader.bin ] ; then
    cp -a $build/${project}.ino.bootloader.bin \
    $build/${project}.partitions.bin $project
  fi
  #把bin文件的crc32值修改为0
  lib/uncrc32 ${project}/${project}.bin $CRC_MAGIC
fi
echo $ver
exit

esp8266
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 45811 / 65536 bytes (69%)
. Code in flash (default, ICACHE_FLASH_ATTR), used 410180 / 1048576 bytes (39%)
esp32-c3
Sketch uses 1198874 bytes (91%) of program storage space. Maximum is 1310720 bytes.
Global variables use 39844 bytes (12%) of dynamic memory, leaving 287836 bytes for local variables. Maximum is 327680 bytes.

