#!/bin/bash
if ! [ "`which paps`" ];then
apt-get update
apt-get install paps
fi
if ! [ "$1" ] ; then
 file=readme.txt
else
 file=$1
fi

paps $file - |ps2pdf - >$file.pdf
