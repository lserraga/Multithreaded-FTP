#include <stdarg.h>
#include <sys/socket.h>

char * Fgets(char *ptr, int n, FILE *stream);

void addChunkNumber(char *msg,int i);

int recvfromTimeOut(int socket, long sec, long usec);

int getChunkNumber(char message[]);

void err_ret(const char *fmt, ...);

void err_sys(const char *fmt, ...);

void err_dump(const char *fmt, ...);

void err_msg(const char *fmt, ...);

void err_quit(const char *fmt, ...);

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

void Bind(int fd, const struct sockaddr *sa, socklen_t salen);

void Connect(int fd, const struct sockaddr *sa, socklen_t salen);

void Listen(int fd, int backlog);

void Send(int fd, const void *ptr, size_t nbytes, int flags);

const char * Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);

void Inet_pton(int family, const char *strptr, void *addrptr);

ssize_t Recv(int fd, void *ptr, size_t nbytes, int flags);

int Socket(int family, int type, int protocol);

void addChunkNumber(char *msg,int i);

int countLines(FILE *fp);

void createACK(int i,char *ACKmessage);