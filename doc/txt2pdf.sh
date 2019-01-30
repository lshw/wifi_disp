#!/bin/bash
if ! [ "`which paps`" ];then
apt-get update
apt-get install paps
fi

paps readme.txt - |ps2pdf - >readme.pdf
