#!/bin/sh
_moPath=$1
_moFile=$2
_poFile=$3
mkdir -p $_moPath
msgfmt -o $_moFile $_poFile
