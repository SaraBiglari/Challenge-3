#include "lib/error_functions.h"
#include "sockets/unix_socket.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

using namespace std;

#define DAEMON_NAME "vdaemon"
#define BACKLOG 5

void process()
{
	syslog(LOG_NOTICE,"writing to my syslog");
	struct sockaddr_un addr;
	
	// create a new server socket with domain: AF_UNIX, type:SOCK_STREAM, protocol:0
	int sfd= socket(AF_UNIX, SOCK_STREAM, 0);
	printf("server socket fd=%d"\n, sfd);
	
	if(sfd == -1) {errExit("socket");}
	if(strlen(SV_SOCK_PATH)>sizeof(addr.sun_path)-1)
	{
		fatal("server socket path too long: %s",SV_SOCK_PATH);
	}
	
	// delete any file that already exists at the address.
	if(remove(SV_SOCK_PATH)== -1 && errno !=ENOENT)
	{
		errExit("remove-%s",SV_SOCK_PATH);
	}
	
	//zero put the address, and set family and path.
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path)-1);
	
	//bind the socket to address.we're binding the servet socket to a known address so that client know where to connect.
	if(bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un))==-1)
	{
		errExit("bind");
	}
	
	//the listen call marks the socket as "passive" and ready to accept connection from "active" sockets.
	if(listen(sfd, BACKLOG)== -1)
	{
		errExit("listen");
	}
	ssize_t numRead;
	char buf[BUF_SIZE];
	for(;;)  //handle client connections iteratively
	{
		printf(""waiting to accept a connection...\n);
		int cfd= accept(sfd, NULL, NULL);
		printf("Accepted socket fd=%d\n", cfd);
		// transfer data from connected socket to stdout until EOF
		
		//read at most BUF_SIZE bytes from the socket into buf.
		while((numRead = read(cfd, buf, BUF_SIZE))>0)
		{
			//then write those bytes from buf into stdout
			if(write(STDOUT_FILENO, buf, numRead) != numRead)
			{
				fatal("partial/failed write");
			}
		}
		if (numRead == -1){errExit("read");}
		if(close(cfd) == -1){errMsg("close");}
	}
}

int main(int argc, char *argv[])
{

//set logging mask and open the log
	setlogmask(LOG_UP(LOG_NOTICE));
	openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, 		LOG_USER);
	
	syslog(LOG_INFO, "Entering Daemon");
	
	//fork the parent process
	pid_t pid,sid;
	
	pid=fork();
	if (pid<0) {exit(EXIT_FAILURE);}
	if (pid>0) {exit(EXIT_SUCCESS);} //close the parent process
	
	umask(0); //change file mask
	sid=setsid();//create a new signiture ID for child
	if (sid<0) {exit(EXIT_FAILURE);}
	
	if((chdir("/"))<0) {exit(EXIT_FAILURE);}//change directory
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	while(true)
	{
		process();
		sleep(60);
	}
	
	closelog();
}
