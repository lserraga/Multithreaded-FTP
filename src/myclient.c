#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "error.h"
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/errno.h>
#define MAXLINE	1496
#define SA struct sockaddr
#define maxThreads 1000
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;

//Definition of boolean value
typedef int bool;
enum { false, true };

//struct for the paramenter stucture that is used in the threads
struct arg_struct {
    char addrress[64];
    char port[64];
		char nameFile[64];
		char offset[10];
		char chunkSize[10];
		int file_size;
};


//thread handler
void *connectionHandler(void  *adresses){
    int         sockfd,remain_data = 0,i=0,toWrite,chunkNumber;
    char        server_response[MAXLINE+2],ACKmessage[8];
    FILE *received_file;
    ssize_t len;
    struct arg_struct *argument=adresses;
    struct sockaddr_in  serveraddr;
    socklen_t len2;

    //Creating the TCP Socket
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    //Setting the server address struct
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(atoi(argument->port));
    Inet_pton(AF_INET, argument->addrress, &serveraddr.sin_addr);

    //Offset
    sendto(sockfd,argument->offset, 10, 0, (SA *) &serveraddr, sizeof(serveraddr));
    //Chunk size
    sendto(sockfd,argument->chunkSize, 10, 0, (SA *) &serveraddr, sizeof(serveraddr));
    //file name
    sendto(sockfd,argument->nameFile, 256, 0, (SA *) &serveraddr, sizeof(serveraddr));

    //Opening the file to copy the chunk
    received_file = fopen(argument->nameFile, "rb+");
    //Offset the file pointer to be able to fwrite in order
    fseek(received_file,atoi(argument->offset),SEEK_SET);
    remain_data = atoi(argument->chunkSize);
    bzero(server_response, strlen(server_response));


    
    //Recive the chunk from the server
    toWrite=MAXLINE;
    while (remain_data > 0){
      //The last packet will have a smaller size so we don't want to write MAXLINE bytes
        if (remain_data<MAXLINE)
            toWrite=remain_data;
          //Receving the Chunk
        len = recvfrom(sockfd, server_response, toWrite+4, 0,(SA *) &serveraddr, &len2);
        //Getting the ChunkNumber
        chunkNumber=getChunkNumber(server_response);
        printf("Chunk number:%i\n",chunkNumber);
        //Checking if the Chunk is in order
        if(chunkNumber!=i){
            printf("Recieved chunk number %i and we are expecting %i\n",chunkNumber,i);
            bzero(server_response, strlen(server_response));
            continue;
        } 
        //Creating ACK message with the ChunkNumber
        createACK(i,ACKmessage);
        
        printf("Sending ACK message %i (%s)\n",chunkNumber,ACKmessage);

        sendto(sockfd,ACKmessage, 8, 0, (SA *) &serveraddr, sizeof(serveraddr));
        //The file being copies is shared between the threads so we need the mutex when we write 
        pthread_mutex_lock(&m);
        fwrite(server_response, sizeof(char), len-4, received_file);
        pthread_mutex_unlock(&m);
        remain_data -= (len-4);
        printf("Step 2 remain data: %i. Written:%i\n", remain_data,len-4);
        printf("New offset of the file:%i\n\n", ftell(received_file));
        bzero(server_response, strlen(server_response));
        i++;
        
    }
    fclose(received_file);
    
    close(sockfd);
    pthread_exit(0);
}


bool udpTest(int sockfd, const SA *pservaddr, socklen_t servlen, char* filename,int* file_size, int* newportNumber,int serverNumber,int printDo){
    int n,res=0;
    char recvline[MAXLINE];
    socklen_t len;
    struct sockaddr *preply_addr;
    preply_addr = malloc(servlen);
    char newport[10];

    //With -1 means the client is asking for the filesize
    sendto(sockfd,"-1", 10, 0, pservaddr, servlen);
    //Chunk size
    sendto(sockfd,"-1", 10, 0, pservaddr, servlen);
    //file name
    sendto(sockfd,filename, 256, 0, pservaddr, servlen);

    len = servlen;

    //Checking if the server is up and has the file
    //If the client doesn't get a message back within 10 seconds it will be considered down
    res = recvfromTimeOut(sockfd, 0, 1000);

    if (res == 0){
        printf("TIMEOUT from server %i\n",serverNumber);
        return false;
    }
    n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr,&len);

    //Need to return IP address and port of who sent back reply
    if (len != servlen || memcmp(pservaddr, preply_addr, len) != 0) {
        printf("reply from (ignore)\n");
        return false;
    }
    if (strcmp(recvline,"Error opening the file")==0){
        printf("Server %i doesn't have the file\n",serverNumber);
        return false;
    }
    recvline[n] = 0; /* NULL terminate */
    *file_size=atoi(recvline);
    if (printDo)
        printf("Server %i is up and has the file\n",serverNumber);
    //Receiving the new port to which we have to write the ACKs and receive the data from 
    recvfrom(sockfd, newport, 10, 0, preply_addr,&len);
    *newportNumber=atoi(newport);
    return true;
}


  int main(int argc, char **argv)
  {
  		int					sockfd,offset[maxThreads],chunkSize[maxThreads],nfiles,serversAvaliable=0,aux, concServ=1,aux3;
  		char				message[256];
  		struct sockaddr_in	servaddr;
      int i,i2=0,i3=0,file_size,nServers;
      char * ipAddress[maxThreads];
      char * port[maxThreads];
      int  newportNumber[maxThreads];
  		pthread_t tid[maxThreads];
  		FILE *received_file;
  		struct arg_struct threadArg[maxThreads];
      bool success[maxThreads];
      char newport[10];

      //Check if the arguments are okey
  		if (argc != 4 || atoi(argv[2])<=0){
    			printf("usage: a.out <server-info.text> <num-connections> <filename>\n");
    			exit(1);
  		}

      //Loading the server ports and addresses
      FILE *serverFile = fopen (argv[1], "r");
  		if(serverFile==NULL){
    			printf("Error opening the server info file\n");
    			exit(1);
  		}
      nfiles=countLines(serverFile);
      for (i =0; i < (2*nfiles); ++i) {
          if(i%2==0){
              ipAddress[i2] = malloc (128); /* allocate a memory slot of 128 chars */
              fscanf (serverFile, "%127s", ipAddress[i2]);
              i2++;
          }
          else{
              port[i3] = malloc (128); /* allocate a memory slot of 128 chars */
              fscanf (serverFile, "%127s", port[i3]);
              i3++;
          }
      }
      //If there are not the same number of ports and ip addresses the file is corrupt
  		if (i2!=i3){
  			printf("Error reading the server file");
  		}
  		fclose(serverFile);

      //Testing the servers loaded from the file
      for (i=0;i<nfiles;i++){
        sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port   = htons(atoi(port[i]));
        Inet_pton(AF_INET, ipAddress[i], &servaddr.sin_addr);

        strcpy(message,argv[3]);

        success[i]=udpTest(sockfd, (SA *) &servaddr, sizeof(servaddr),message,&file_size,&aux,i,1);
        newportNumber[i]=aux;
        if(success[i])
            serversAvaliable++;

        close(sockfd);
      }

      

      //All the servers are not availabe or don't have the file so we can not
      //retrieve it
      if (serversAvaliable==0){
        printf("Error, there aren't any available servers with the desired file\n ");
        exit(1);
      }

      printf("File size of %s is %i\n\n",argv[3],file_size);

      //Selecting the number of servers the client is going to connect to
      //If the number specified is greater than the servers avaliable
      //It is going to connect multiple times to the same server
      nServers=atoi(argv[2]);
      if (serversAvaliable<nServers)
          concServ=nServers/serversAvaliable;
      aux3=nServers%serversAvaliable;
      //Stablishing the offset and the chuksize for every thread connection
      offset[0]=0;
      chunkSize[0]=file_size/nServers+1;
      if (nServers==1)
          chunkSize[0]=file_size/nServers;
      for (i=1;i<nServers;i++){
          offset[i]=offset[i-1]+file_size/nServers+1;
          chunkSize[i]=file_size/nServers+1;
          if(i==nServers-1)
            chunkSize[i]=file_size-(nServers-1)*chunkSize[0];
      }

      //If the file exists in the client it removes it so later one can be
      //copied easilly
      received_file = fopen(argv[3], "w");
      fclose(received_file);

      //Calling the threads for every working server. A struct of arguments is
      //created for every thread so there are not any data races
      int serverIdentifier=0;
      for (i=0;i<nfiles;i++){
          //Only create threads for the servers the client managed to get the filesize from
          if (success[i]==true){
              //For the servers that need mulitple connections
              for (int aux2=0;aux2<concServ;aux2++){
                  if (aux2>0){
                      sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
                      bzero(&servaddr, sizeof(servaddr));
                      servaddr.sin_family = AF_INET;
                      servaddr.sin_port   = htons(atoi(port[i]));
                      Inet_pton(AF_INET, ipAddress[i], &servaddr.sin_addr);

                      strcpy(message,argv[3]);
                      //We get the new port from the server and we check if the server is up 
                      udpTest(sockfd, (SA *) &servaddr, sizeof(servaddr),message,&file_size,&aux,i,0);

                      sprintf(newport,"%i",aux);
                  }
                  else
                      sprintf(newport,"%i",newportNumber[i]);
                  //Establishing the arguments for the threat
                  strcpy(threadArg[serverIdentifier].addrress,ipAddress[i]);
                  
                  strcpy(threadArg[serverIdentifier].port,newport);
                  sprintf(threadArg[serverIdentifier].offset, "%i", offset[serverIdentifier]);
                  sprintf(threadArg[serverIdentifier].chunkSize, "%i", chunkSize[serverIdentifier]);
                  threadArg[serverIdentifier].file_size=file_size;
                  strcpy(threadArg[serverIdentifier].nameFile,argv[3]);
                  printf("Retrieving chunk from server with %s:%s (Connection number %i)\n",ipAddress[i],newport,aux2+1);
                  printf("Offset: %s, chunksize: %s\n",threadArg[serverIdentifier].offset,threadArg[serverIdentifier].chunkSize);

                  //If the number of concurrent connections for all the servers 
                 // if(serverIdentifier==nServers-2 && nServers%serversAvaliable!=0)
                   //   concServ++;
                  if(aux2==concServ-1 && aux3>0){
                      concServ++;
                      aux3--;
                  }

                  //Creating the threads
                  if (pthread_create(&tid[serverIdentifier], NULL,connectionHandler,(void *)&threadArg[serverIdentifier])!=0){
                      fprintf (stderr, "Unable to create connection thread\n");
                      exit(1);
                  }
                  
                  serverIdentifier++;
                  
                  
            }
          }
      }
      //Join the threads
      for (i=0;i<serverIdentifier;i++){
          pthread_join(tid[i],NULL);
      }

      printf("The file %s was successfully received\n", argv[3]);
      //Dealocating addrress variable spaces
      for (i=0;i<nfiles;i++){
          free(port[i]);
          free(ipAddress[i]);
      }

  	exit(0);
}
