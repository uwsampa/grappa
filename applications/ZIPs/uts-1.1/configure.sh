#!/bin/bash

CONFDIR="config"
CONFFILE="config.in"
MYNAME="configure.sh"

function usage () {

	echo "UTS - Unbalanced Tree Search Configuration.  Selects from available"
	echo "      configurations in the '$CONFDIR' directory."
	echo
	echo " Usage: $MYNAME CONFIGURATION_NAME"
	echo
	echo " Available Configurations:"

	for file in ${CONFDIR}/*
	do
		[ -r $file ] && [ ! -d $file ] && echo "   $(echo $file | cut -d/ -f2)"
	done

}

if [ ! -d $CONFDIR ]
then
	echo "Fatal error: Unable to access the config file directory, '$CONFDIR'!"
	exit 1
fi

if [ $# -lt 1 ] || [ $1 = '-h' ] || [ $1 = '--help' ]
then
	usage
	exit 0
fi

if [ -r $CONFDIR/$1 ]
then
	ln -sf $CONFDIR/$1 $CONFFILE

	echo
	echo "Configuration changed.  Please review '$CONFDIR/$1' to ensure the"
	echo "new settings are correct."
	echo
else
	echo "Could not find configuration file: $1"
	exit 1
fi
