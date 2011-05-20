TextWrangler remote control
===========================

This project makes it possible to be logged into an remote server and launch from there the local [TextWrangler][] with the remote file.

[TextWrangler]: http://www.barebones.com/products/textwrangler/


Usage
-----

	local:~/$ ssh my-server
	my-server:/path$ edit file

That will result in opening `sftp://my-server/path/file` on the local computer.

	my-server:/path$ ls -la | edit
	
That will result in opening the list in TextWragngler (useful on `diff` and other utilities).

	my-server:/path$ crontab -e

That will result in opening the temporary file from the remote server, and the program waits until the text is closed, so it works really great.

-----

You have two "commands" available: `edit` and `editw`.
`edit` just opens the file in TextWrangler and then returns to promt, just like `edit` on your mac (TextWrangler command line utility). With the option `-w` or the command `editw` the file is also openend in TextWrangler, but the command exits on closing the text, this is useful for interactive editors, like `crontab`, `svn ci` and so on.

It's also possible to open `stdin` and not a file, in this case the `-w` flag has no meaning. Interactive stdin mode is not possible.


Requirements
------------

#### Local machine

* `gcc`
* `php` (almost any version will do, see `edit-ssh.sh`)

#### Remote machine

* `netcat`


Installation
------------

#### Compilation

Compile the server with:

	gcc -Os -std=c99 -o edit-server edit-server.c

#### Local setup

Install the command line tools from TextWrangler.

Run the server with:

	./edit-server

  See for options below.

Copy `edit-ssh.sh` to a safe place (e.g. `/Users/USER/bin/edit-ssh.sh`).

Edit your `~/.profile`, `~/.bashrc` or similar file, append:

	alias ssh=/Users/USER/bin/edit-ssh.sh

  (or where ever you've placed your file)

#### Remote setup

Copy `edit-remove.sh` to the remove server (e.g. `/home/USER/edit-remote.sh`).

Edit your `~/.profile`, `~/.bashrc` or similar file, append:

	if test \( -n "$LC_EDIT_HOST" \) -a \( -n "$LC_EDIT_PORT" \)
	then
			alias edit='/home/USER/edit-remote.sh'
			alias editw='/home/USER/edit-remote.sh -w'
			export EDITOR='/home/USER/edit-remote.sh -w'
	else
			alias edit=joe
			alias editw=joe
			export EDITOR=joe
	fi

  Use the correct path, in the second part enter a editor of your choice for alternate use.


Server options
--------------

* `-f`: run in foreground, don't daemonize, log to stderr
* `-p port`: run on another port than 3333 - this port is hardcoded into `edit-remote.sh` so you have to change it there to
* `-l logfile`: log also to `logfile`


How it works
------------

* The ssh connection opens a reverse portforward, which allows it our program on the remote server to connect to a randomly choosen port on the server, which is forwarded to the localhost's port 3333 (the default server port).
* The two variables `LC_EDIT_HOST` and `LC_EDIT_PORT` are pushed to the remote server, I had to choose `LC_*` variables, because the ssh-server default is to only accept those variables.
* The variable `LC_EDIT_PORT` contains the randomly choosen port number on the remote server.
* The variable `LC_EDIT_HOST` contains the host of the remote server, as seen from the local machine, the file to be openend looks like `sftp://${LC_EDIT_HOST}/path/file`.


License
-------

[Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License](http://creativecommons.org/licenses/by-nc-sa/3.0/)
