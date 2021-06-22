#!/bin/bash 

DIR=`dirname "$0"`

OCD_DIR=$DIR/fr.ac6.mcu.externaltools.openocd.linux64_1.23.0.201904120827/tools/openocd
SCRIPTS_DIR=$DIR/fr.ac6.mcu.debug_2.5.0.201904120827/resources/openocd

export LD_LIBRARY_PATH=$OCD_DIR/lib:$LD_LIBRARY_PATH

if [ $# -lt 1 ]; then
	$OCD_DIR/bin/openocd --help
	exit 1
fi

$OCD_DIR/bin/openocd --search $SCRIPTS_DIR/scripts \
	--search $SCRIPTS_DIR/st_scripts \
	"$@"
