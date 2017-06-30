#include <stdio.h>
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>		/* ANSI C header file */
#include <syslog.h>		/* for syslog() */
#include <sys/errno.h>
#include <arpa/inet.h>
#define MAXLINE	4096

int		daemon_proc;		/* set nonzero by daemon_init() */
/* Nonfatal error related to system call
 * Print message and return */


void addChunkNumber(char *msg,int i){
    char chunkNumber[4];
    if(i<10)
        sprintf(chunkNumber, "000%i", i);
    else if (i<100)
        sprintf(chunkNumber, "00%i", i);
    else if (i<1000)
        sprintf(chunkNumber, "0%i", i);
    else 
        sprintf(chunkNumber, "%i", i);
    strcat(msg, chunkNumber);

    return;
}

//function that counts lines in a txt file
int countLines(FILE *fp) {

    int lines = 0;
    char ch;

    while(!feof(fp)) {
      ch = fgetc(fp);
      if(ch == '\n') {
        lines++;
      }
    }
    rewind(fp);
    return lines;
}

void createACK(int i,char *ACKmessage){
    char chunkNumber[4];
    bzero(ACKmessage,8);
    if(i<10)
        sprintf(chunkNumber, "000%i", i);
    else if (i<100)
        sprintf(chunkNumber, "00%i", i);
    else if (i<1000)
        sprintf(chunkNumber, "0%i", i);
    else 
        sprintf(chunkNumber, "%i", i);
    strcat(ACKmessage,"ACK");  
    strcat(ACKmessage, chunkNumber);
    ACKmessage[7]='\0';

    return ;
}

int recvfromTimeOut(int socket, long sec, long usec){
  // Setup timeval variable
  struct timeval timeout;
  timeout.tv_sec = sec;
  timeout.tv_usec = usec;
  // Setup fd_set structure
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(socket, &fds);
  // >0: data ready to be read
  return select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
}

int getChunkNumber(char message[]){
    char chunkNumber[3];
    chunkNumber[0]=message[strlen(message)-4];
    chunkNumber[1]=message[strlen(message)-3];
    chunkNumber[2]=message[strlen(message)-2];
    chunkNumber[3]=message[strlen(message)-1];
    return atoi(chunkNumber);
}

 char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}

 const char *
Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
	const char	*ptr;

	if (strptr == NULL)		/* check for old code */
		err_quit("NULL 3rd argument to inet_ntop");
	if ( (ptr = inet_ntop(family, addrptr, strptr, len)) == NULL)
		err_sys("inet_ntop error");		/* sets errno */
	return(ptr);
}

void
Inet_pton(int family, const char *strptr, void *addrptr)
{
	int		n;

	if ( (n = inet_pton(family, strptr, addrptr)) < 0)
		err_sys("inet_pton error for %s", strptr);	/* errno set */
	else if (n == 0)
		err_quit("inet_pton error for %s", strptr);	/* errno not set */

	/* nothing to return */
}


int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr){
 	int		n;

 again:
 	if ( (n = accept(fd, sa, salenptr)) < 0) {
 #ifdef	EPROTO
 		if (errno == EPROTO || errno == ECONNABORTED)
 #else
 		if (errno == ECONNABORTED)
 #endif
 			goto again;
 		else
 			err_sys("accept error");
 	}
 	return(n);
 }

 ssize_t Recv(int fd, void *ptr, size_t nbytes, int flags)
{
	ssize_t		n;

	if ( (n = recv(fd, ptr, nbytes, flags)) < 0)
		err_sys("recv error");
	return(n);
}

 void
 Bind(int fd, const struct sockaddr *sa, socklen_t salen)
 {
 	if (bind(fd, sa, salen) < 0)
 		err_sys("bind error");
 }

 void
 Connect(int fd, const struct sockaddr *sa, socklen_t salen)
 {
 	if (connect(fd, sa, salen) < 0){
 		err_sys("connect error");
	}
 }

void
 Listen(int fd, int backlog)
 {
 	char	*ptr;
 	if ( (ptr = getenv("LISTENQ")) != NULL)
 		backlog = atoi(ptr);

 	if (listen(fd, backlog) < 0)
 		err_sys("listen error");
 }

void
 Send(int fd, const void *ptr, size_t nbytes, int flags)
 {
 	if (send(fd, ptr, nbytes, flags) != (ssize_t)nbytes)
 		err_sys("send error");
 }

 int
 Socket(int family, int type, int protocol)
 {
 	int		n;

 	if ( (n = socket(family, type, protocol)) < 0)
 		err_sys("socket error");
 	return(n);
 }

void
err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error related to system call
 * Print message and terminate */

void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void
err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void
err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	buf[MAXLINE + 1];

	errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
#else
	vsprintf(buf, fmt, ap);					/* not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, buf);
	} else {
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}
