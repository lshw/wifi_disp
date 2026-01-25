#!/bin/bash
cd `dirname $0`
cd ..

project=wifi_disp

a=`git log --date=short -1 $project |head -n 1 |awk '{print $2}'`
date=`git log --date=short -1 $project |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
git tag $ver
export debug=",dbg=Serial,lvl=SSLTLS_MEMHTTP_CLIENTHTTP_SERVERCOREWIFIHTTP_UPDATEUPDATEROTAOOMMDNSHWDT"
${project}/build.sh
mv ${project}/${project}.bin lib/wifi_disp_dbg.bin
export debug=",dbg=Disabled,lvl=None____"
${project}/build.sh
mv ${project}/${project}.bin lib
git log >changelog.txt
cp doc/readme.pdf doc/readme.txt  doc/files.* .
tar cvfz wifi_disp_$ver.tar.gz lib/wifi_disp.bin lib/wifi_disp_dbg.bin lib/wifi_disp.php lib/proc3_socat.sh changelog.txt readme.* files.* lib/update.sh lib/*.py
rm -f changelog.txt readme.pdf readme.txt files.pdf files.txt
