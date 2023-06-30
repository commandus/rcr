#!/bin/sh
R=~/src/rcr/
BIN=$R/cmake-build-debug
SRC=$R/data
echo "V"
for f in $SRC/Мксх*.xlsx ; do
echo $f
$BIN/rcr-cli xlsx-add-v \"$f\"
done
echo "C"
for f in $SRC/*Конденсаторы*.xlsx ; do
echo $f
$BIN/rcr-cli xlsx-add-c \"$f\"      
done