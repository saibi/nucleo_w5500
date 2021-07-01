#!/bin/bash 
if [ $# -eq 1 ]; then
	cat $1 | grep "version_str" | awk ' { print $4 }' | sed s/\"//g | sed s/\;//g | tr -d '\r'
fi
