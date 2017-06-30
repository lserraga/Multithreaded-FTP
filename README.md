# Multithreaded-FTP
   Basic Server-Client

  What is it?
  -----------

  The basic Server-Client allows a client to download “chunks” of a file from
  multiple servers distributed over the Internet, and assemble the chunks to
  form the complete file

  Documentation
  -------------

  Included in the doc directory.

  Installation
  ------------

  Executing make in the main directory compiles all the files
  needed. The servers should be executed within each bin location.

  Files
  --------
  /bin/
      myserver: executable for the server program
      myclient : executable for myserver.c
      6 test files of different formats and sizes


  /doc/
      protocol.txt : A specification of the application-layer protocol.
      usage.txt : Documentation of the design usage.

  /src/
      myserver.c : contains the code for the implementation of the
      server. Imports functions from the following libraries: sys/types.h,
      sys/socket.h, strings.h, string.h, arpa/inet.h, stdio.h, stdlib.h,
      unistd.h and error.h
      myclient.c : contains the code for the implementation of the
      client. Imports functions from the following libraries: sys/types.h,
      sys/socket.h, strings.h, string.h, arpa/inet.h, stdio.h, stdlib.h,
      unistd.h and error.h
      error.c : contains plenty of functions for error handling and also
      wrapping functions for the sys/socket.h and arpa/inet.h functions.
      All used from the unp.h library included in the book UNIX Network
      Programming, Volume 1, Third Edition, The Sockets Networking API
      It also contains some functions for the adding and obtaining of
      the ACK and chunk number.
      error.h : contains the headers for the error.c functions
      Makefile : compiles both myclient and myserver using error.c and
      error.h as sources. Also contains a clean function


  Makefile : calls the Makefile in /src/ and also includes a clean
  function
  server-info.txt: contains the port and ip address information of the servers.

  Contacts
  --------

  Luis Serra Garcia, Creator: lserraga@ucsc.edu
