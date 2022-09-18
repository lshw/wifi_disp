#!/bin/bash
cd `dirname $0`

branch=`git status |head -n 1 |awk '{print $2}'`
a=`git log --date=short -1 |head -n 1 |awk '{print $2}'`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:8}
echo $ver
git tag $ver
./build.sh
git log >changelog.txt
cp ../doc/readme3.pdf ../doc/readme3.txt .
tar cvfz wifi_disp_$ver.tar.gz wifi_disp.bin wifi_disp.php changelog.txt readme3.* update.sh *.py
rm changelog.txt readme3.pdf readme3.txt
