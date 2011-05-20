/*
** An "edit" Server
** (c) 2011 ALeX Kazik
** License: http://creativecommons.org/licenses/by-nc-sa/3.0/
*/

/*
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
** Compile with: gcc -Os -std=c99 -o edit-server edit-server.c
*/

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <fcntl.h>

// macros
#define min(a,b) (((a) < (b)) ? (a) : (b))

// constants
#define ETB 0x17
#define EOT 0x04

// a few variables
#define BUFFER_LENGTH 4096
char buffer[BUFFER_LENGTH];
#define COMMAND_LENGTH 32
char command[COMMAND_LENGTH];

#define NUM_LOGS 3
FILE *logs[NUM_LOGS];

void ierror(char *msg){
	for(int i=0; i<NUM_LOGS; i++){
		if(logs[i]){
			fprintf(logs[i], "%s: %s\n", msg, strerror(errno));
		}
	}
	exit(1);
}

void cerror(char *format, ...){
	for(int i=0; i<NUM_LOGS; i++){
		if(logs[i]){
			va_list vl;
			va_start(vl, format);
			vfprintf(logs[i], format, vl);
		}
	}
	exit(1);
}

void usage(char *progname, int retval){
	fprintf(stderr, "Usage: %s [-f] [-p port] [-l logfile]\n", progname);
	exit(retval);
}

int read_until(int fd, char *buf, char ch, int maxlen, int step){
	int pos = 0, n;
	while(1){
		n = read(fd,&buf[pos],min(step,maxlen-pos));
		if(n < 0){
			ierror("ERROR reading from socket");
		}else if(n == 0){
			// EOF
			buf[pos] = 0;
			return 0;
		}
		pos += n;
		if(buf[pos-1] == ch){
			buf[pos-1] = 0; // delete the END char
			return 1;
		}
		if(pos >= maxlen){
			// buffer end
			buf[maxlen-1] = 0;
			return 0;
		}
	}
}

// MODIFIES the buffer: remove all non printable ascii chars
char *fix(char *t){
	int i;
	for(i=0; t[i]; i++){
		if(t[i] < 0x20 || t[i] > 0x7e){
			t[i] = '@';
		}
	}
	return t;
}

// checks for validity
int is_valid(char *t){
	int i;
	for(i=0; t[i]; i++){
		if(t[i] < 0x20 || t[i] > 0x7e){
			return 0;
		}
	}
	return 1;
}

void work(int fd){
	// make the network also a log-file
	logs[2] = fdopen(fd, "w");
	// read the command
	if(!read_until(fd, command, ETB, COMMAND_LENGTH, 1)){
		cerror("Unable to get the command (\"%s\")\n", fix(command));
	}
	// check for a command
	int cmd_edit = !strcmp(command, "edit");
	int cmd_editw = !strcmp(command, "editw");
	int cmd_stdin = !strcmp(command, "stdin");
	
	if(cmd_edit || cmd_editw){
		// regular edit mode, read all filenames until EOT
		if(!read_until(fd, buffer, EOT, BUFFER_LENGTH, BUFFER_LENGTH)){
			cerror("Unable to read the files (\"%s\")\n", fix(buffer));
		}
		int i, count=1;
		for(i=0; buffer[i]; i++){
			if(buffer[i] == ETB){
				count++;
			}
		}
		char **args = malloc(sizeof(char *) * (count + 4));
		// args 0 = name, 1 = maybe param "-w", 2 = "--", n-1 = null => 4 more
		int argc = 0;
		args[argc++] = "edit";
		if(cmd_editw){
			args[argc++] = "-w";
		}
		args[argc++] = "--";
		int add=1;
		int first = argc;
		for(i=0; buffer[i]; i++){
			if(buffer[i] == ETB){
				buffer[i] = 0; // end of string
				add = 1;
			}else if(add){
				args[argc++] = &buffer[i];
				add = 0;
			}
		}
		args[argc++] = NULL;
		
		for(i=first; args[i]; i++){
			if(!is_valid(args[i])){
				cerror("Bad file: \"%s\"\n", fix(args[i]));
			}
		}
		
		dup2(fd, 0); // make fd stdin
		dup2(fd, 1); // make fd stdout
		dup2(fd, 2); // make fd stderr
		fclose(logs[2]); // close fd (FILE mode)
		close(fd); // close fd (the dup's are not closed)

		execv("/usr/bin/edit", args);
	}else if(cmd_stdin){
		dup2(fd, 0); // make fd stdin
		dup2(fd, 1); // make fd stdout
		dup2(fd, 2); // make fd stderr
		fclose(logs[2]); // close fd (FILE mode)
		close(fd); // close fd (the dup's are not closed)
		
		execl("/usr/bin/edit", "edit", "--clean", NULL);
	}else{
		cerror("Unknown command: \"%s\"\n", fix(command));

		fclose(logs[2]); // close fd (FILE mode)
		close(fd); // close fd (the dup's are not closed)
	}
}

int main(int argc, char *argv[]){
	// prepare logs (in case of an error)
	logs[0] = stderr;
	for(int i=1; i<NUM_LOGS; i++){
		logs[i] = NULL;
	}

	// our name (save for later, getopt scrambles it)
	char *progname = argv[0];
	
	// our paramters
	int portno = 3333, deamon = 1;
	
	// for the getopt
	int ch;
	
	// read parameter
	while ((ch = getopt(argc, argv, "p:l:f")) != -1) {
		switch (ch) {
			case 'f':
				deamon = 0;
				break;
			case 'p':
				portno = atoi(optarg);
				break;
			case 'l':
				logs[1] = fopen(optarg, "w");
				if(! logs[1]){
					ierror("ERROR opening logfile");
				}
				break;
			case '?':
			default:
				usage(progname, 0);
				break;
		}
	}
	argc -= optind;
	argv += optind;
	
	// check for unused parameters -> fail
	if(argc > 0){
		usage(progname, 1);
	}
	
	// ignore dying childs
	signal(SIGCHLD,SIG_IGN);
	
	// variables
	int sockfd, newsockfd;
	struct sockaddr_in serv_addr;
	
	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){ 
		ierror("ERROR opening socket");
	}
	
	// bind to address/port
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		ierror("ERROR on binding");
	
	// listen
	listen(sockfd,5);
	
	// in daemon mode, fork a child, and disconnect it from the shell
	if(deamon){
		int logfd, pid;
		// fork
		pid = fork();
		if(pid < 0){
			ierror("ERROR on fork");
		}else if(pid > 0){
			// parent
			exit(0);
		}
		// child!
		logs[0] = NULL; // remove stderr "log" file

		// Process Independency
		setsid();
		
		// Inherited Descriptors and Standart I/0 Descriptors
		// close all except the network socket & log file
		if(logs[1]){
			logfd = fileno(logs[1]);
		}else{
			logfd = -1;
		}
		for(int i=getdtablesize(); i>=0; i--){
			if(i != sockfd && i != logfd){
				close(i);
			}
		}
		// open /dev/null as stdin, stdout, stderr
		open("/dev/null", O_RDWR);
		dup(0);
		dup(0);

		// Running Directory
		chdir("/");
		
		// Catching Signals
		signal(SIGCHLD,SIG_IGN);
	}
	
	// loop: wait for a connection and handle it
	while(1){
		struct sockaddr_in cli_addr;
		int n;
		socklen_t clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0){
			ierror("ERROR on accept");
		}
		int pid = fork();
		if(pid < 0){
			ierror("ERROR on fork");
		}else if(pid == 0){
			// child!
			close(sockfd);
			work(newsockfd);
			exit(0);
		}else{
			// parent
			close(newsockfd);
		}
	}
}
