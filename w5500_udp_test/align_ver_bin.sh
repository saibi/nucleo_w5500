#!/bin/bash
# firmware binary 4-byte alignment 
size=`ls -al $1 | awk '{ print $5 }'`

let "addval = 4 - size % 4"

while [ $addval != 0 ] 
do
	echo "" >> "$1"
	let "addval = addval - 1"
done
exit 0
