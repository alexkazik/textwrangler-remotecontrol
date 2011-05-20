#!/bin/sh

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

HOST=$1
shift

if test -z "$*"
then
	# no more parameters
	export LC_EDIT_PORT=`php -r 'echo mt_rand(20000,50000);'`
	export LC_EDIT_HOST=$HOST
	/usr/bin/ssh -R $LC_EDIT_PORT:127.0.0.1:3333 -o "SendEnv LC_EDIT_HOST LC_EDIT_PORT" "$HOST"
else
	# more than one paramater -> regular mode
	/usr/bin/ssh "$HOST" "$@"
fi
