#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#define MAXLINE 1496
#define SA struct sockaddr
#define maxThreads 100


//struct for the paramenter stucture that is used in the threads
struct arg_struct {
    int sockfd;
    SA *pcliaddr;
    socklen_t clilen;
    char newPort[10];
};



int dg_echo(int sockfd, SA *pcliaddr, socklen_t clilen,int newPortNumber){
    int fd,remain_data,sent_bytes=0,res=0,i=0,toSend,ackNumber;
    char commandCode[10],chunkSize[10],fileName[256];
    socklen_t len;
    struct stat file_stat;
    char msg[MAXLINE+4], ack[8],newPort[10];
    long int offset;
    FILE *fp;

        len=clilen;
        //Receiving offset
        recvfrom(sockfd, commandCode, 10, 0, pcliaddr, &len);
        printf("Offset: %s\n",commandCode);
        //Receving Chunk size
        recvfrom(sockfd, chunkSize, 10, 0, pcliaddr, &len);
        printf("Chunk size: %s\n",chunkSize);
        //Receiving fileName
        recvfrom(sockfd, fileName,256, 0, pcliaddr, &len);
        printf("File name: %s\n",fileName);
        fd = open(fileName, O_RDONLY);
        //If the server can't open the file it sends "Error opening the file"
        if (fd == -1){
          sendto(sockfd,"Error opening the file",MAXLINE,0, pcliaddr, len);
          printf("Error opening file --> %s\n", fileName);
          return 0;
        }
        printf("Opened: %s\n",fileName);
        /* Get file stats */
        if (fstat(fd, &file_stat) < 0){
          fprintf(stderr, "Error fstat --> %s", strerror(errno));
          return 0;
        }
        sprintf(msg, "%d", file_stat.st_size);
        printf("Size: %s\n",msg);
        //Send the file size if the command code was -1
        if (strcmp(commandCode,"-1")==0){
          printf("Sending file size of %s\n\n",fileName);
          sendto(sockfd, msg, MAXLINE, 0, pcliaddr, len);
          //Sending new port numbet
          sprintf(newPort,"%ld",newPortNumber);
          sendto(sockfd, newPort, 10, 0, pcliaddr, len);
          bzero(newPort, 10);
          bzero(fileName, 256);
          bzero(commandCode, 10);
          bzero(chunkSize, 10);
          return 1;
        }
        
        offset = atoi(commandCode);
        remain_data = atoi(chunkSize);
        fp=fopen(fileName,"r");
        //Setting the file descriptor with the offset
        fseek(fp,offset,SEEK_SET);
        toSend=MAXLINE;

        //Sending the chunks of data
        while(remain_data>0){
          if(remain_data<MAXLINE)
                toSend=remain_data;
          fread(msg,toSend,1,fp);
          addChunkNumber(msg,i);
          printf("Chunk number %i \n",i);
          sendto(sockfd,msg,toSend+4,0,pcliaddr,len);
          printf("Sent %i bytes, %i remaining \n",toSend,remain_data-toSend);

          //Waiting for correct Ack message with a timeout of 5 seconds 
          res = recvfromTimeOut(sockfd, 0, 5000000);

          if (res == 0){
                //Ack timeout
                //Move back file pointer
                fseek(fp, -toSend, SEEK_CUR);
                bzero(ack,10);
                bzero(msg,toSend+4);
                printf("ACK timeout\n");
                continue;
          }
          recvfrom(sockfd, ack, 8, 0, pcliaddr,&len);


          ackNumber=getChunkNumber(ack);
          ack[3]='\0';

          if (strcmp(ack,"ACK")!=0){
                //The message recieved is not an ack
                //Move back file pointer
                fseek(fp, -toSend, SEEK_CUR);
                printf("ACK not recieved\n");
                continue;
          }
          if (ackNumber!=i){
                //The message recieved is not an ack
                //Move back file 
                printf("Recieved ack number %i and we are expecting %i\n",ackNumber,i);
                fseek(fp, -toSend, SEEK_CUR);
                printf("ACK not recieved\n");
                continue;
          }
          printf("ACK number %i recieved\n\n",ackNumber);
          i++;
          bzero(ack,10);
          bzero(msg,toSend+4);
          remain_data=remain_data-toSend;
          
        }
        printf("Sent %i bytes to the client with an offset of %i\n",atoi(chunkSize),atoi(commandCode));

        close(sockfd);

}

//Thread handler function
void *connectionHandler(void  *parameters){
    struct arg_struct *argument=parameters;
    dg_echo(argument->sockfd, argument->pcliaddr,argument->clilen,0);
}



int
main(int argc, char * * argv){
    int listenfd,senderfd,i=0,threadListenFD[maxThreads];
    struct sockaddr_in servaddr,cliaddr,threadAddress[maxThreads],cliThreadAddr[maxThreads];
    pthread_t tid[maxThreads];
    struct arg_struct threadArg[maxThreads];
    char newPort[10];
    socklen_t len;
    unsigned short newPortShort;

    //Check if the arguments are okay
    if (argc != 2) {
        printf("usage: <Port>");
        exit(1);
    }

    //Creation of the UDP socket
    listenfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero( & servaddr, sizeof(servaddr));
    //Creating the address struck
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    //Bind the socket to the address
    Bind(listenfd, (SA * ) & servaddr, sizeof(servaddr));
    for(;;){

        //Checking if the servwr has the correct file  
        if(dg_echo(listenfd, (SA *) &cliaddr,sizeof(cliaddr), atoi(argv[1])+i+1)==1){
          
            //Creation of the UDP socket
            threadListenFD[i] = Socket(AF_INET, SOCK_DGRAM, 0);
            bzero( & threadAddress[i], sizeof(threadAddress[i]));
            //Creating the address struck
            threadAddress[i].sin_family = AF_INET;
            threadAddress[i].sin_addr.s_addr = htonl(INADDR_ANY);
            threadAddress[i].sin_port =htons(atoi(argv[1])+i+1);
            printf("Opened port %i for the thread number %i\n",atoi(argv[1])+i+1,i);
            //Bind the socket to the address
            Bind(threadListenFD[i], (SA * ) & threadAddress[i], sizeof(threadAddress[i]));
            threadArg[i].sockfd=threadListenFD[i];
            threadArg[i].pcliaddr=(SA *) &cliThreadAddr[i];
            threadArg[i].clilen=sizeof(cliThreadAddr[i]);
            
            pthread_create(&tid[i], NULL,connectionHandler,(void *)&threadArg[i]);
            i++;
        }
        
    }

    for (int aux=0;aux<i;aux++){
          pthread_join(tid[aux],NULL);
    }
}
