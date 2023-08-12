#!/bin/bash
cd `dirname $0`
cd ..

branch=`git status |head -n 1 |awk '{print $2}'`
a=`git log --date=short -1 |head -n 1 |awk '{print $2}'`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:8}
echo $ver
git tag $ver
lib/build.sh
git log >changelog.txt
cp doc/readme.pdf doc/readme.txt .
tar cvfz wifi_disp_$ver.tar.gz lib/wifi_disp.bin lib/wifi_disp.php changelog.txt readme.* lib/update.sh lib/*.py
rm changelog.txt readme.pdf readme.txt
