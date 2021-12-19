#!/bin/bash
if [ $1 ] ; then
  count=$1
else
  count=91
fi
mkdir -p ~/sketchbook/build_test
rm -rf ~/sketchbook/build_test/*
base=`realpath $0`
base=`dirname $base`
base=`dirname $base`
cd ~/sketchbook/build_test
git clone $base test
cd test
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
VER="$ver"
echo $VER

cp lib/build_test.sh lib/build0.sh
i=0
while [ 1 ]
do
 lib/build0.sh test_${i}_ >/tmp/build.log 2>/tmp/build_err.log
 branch=`git branch |grep "^\*" |awk '{print $2}'`
 a=`git rev-parse --short HEAD`
 date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
 ver=$date-${a:0:7}
 log=`git log -1 |grep -v ^$|tail -n 1`
 if [ $? != 0 ] ; then
  err=" 编译错误"
 else
  err=""
  ram=`grep "Global vari" /tmp/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "RAM:"$1}'`
  rom=`grep "Sketch uses" /tmp/info.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "ROM:"$1}'`
 fi
 echo ${i}_$branch-$ver $ram,$rom $log$err |tee -a readme.txt
 echo $(( i++ ))
 if [ $i -gt $count ] ; then
  break
 fi
 git reset --hard HEAD^
done
7z -tzip a $base/lib/${VER}_all_$i.zip readme.txt lib/test_*_wifi_disp.bin >/dev/null
ls -l $base/lib/${VER}_all_$i.zip
