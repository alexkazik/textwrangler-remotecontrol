#!/bin/sh

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

ETB=$'\x17'
EOT=$'\x04'

if test "$1" = "-w"
then
	COMMAND=editw
	# slurp the -w
	shift
else
	COMMAND=edit
fi

if test -z "$*"
then
	# no params -> stdin mode
	if test -t 0
	then
		echo interactive stdin not supported
	else
		{
			echo -n "stdin${ETB}"
			cat
		} | netcat -q 1 127.0.0.1 $LC_EDIT_PORT
	fi
else
	# params -> file mode
	DIR=`pwd`
	DATA=${COMMAND}
	while test -n "$*"
	do
		ARG=$1
		shift
		if test -n "$ARG"
		then
			cd -- "$DIR"
			if test ! -e "$ARG"
			then
				touch -- "$ARG"
			fi
			cd -- "`dirname $ARG`"
			DATA=${DATA}${ETB}sftp://${LC_EDIT_HOST}/`pwd`/`basename $ARG`
		fi
	done
	echo -n ${DATA}${EOT} | netcat 127.0.0.1 $LC_EDIT_PORT
fi
