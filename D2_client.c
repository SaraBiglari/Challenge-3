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
	ssize_t numRead;
	char buf[[BUF_SIZE];
	
	//create a new client socket with domain:AF_UNIX, type:SOCK_STREAM, protocol:0	
	int sfd= socket(AF_UNIX, SOCK_STREAM, 0);
	printf("client socket fd=%d"\n, sfd);
	
	if(sfd == -1) {errExit("socket");}
	
	//construct server address, and make the connection.
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path)-1);
	
	//connect the active socketrefferd to be sfd to the listening socket whose address is specified by addr
	
	if(connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un))== -1)
	{errExit("connect");}
	
	
	//copy stdin to socket
	
	//read at most BUF_SIZE bytes from STDIN into buf.
	while((numRead = read(STDIN_FILENO, buf, BUF_SIZE))>0)
	{
	//then writes those bytes from buf into the socket
		if(write(sfd, buf, numRead) != numRead)
		{
			fatal("partial/failed write");
		}
	}
	if (numRead == -1){errExit("read");}
	
	//closes our socket.server sees EOF
	exit(EXIT_SUCCESS);	
	
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
