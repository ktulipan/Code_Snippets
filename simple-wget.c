/********************************\
** Kyle Tulipano -- ktulip2	**
** CS 450 - Network Programming	**
** HW1				**
** 				**
\********************************/



#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv){
  if (argc != 2) { fprintf(stderr, "usage: %s <url>\n", argv[0]); exit(1); }

  // start processing the input string... 'host' will become the string
  // we use for getaddrinfo (i.e. www.example.com) rest will become
  // what we need to HTTP GET request for.

  argv[1] += 7; //assumes http:// at beginning as per specification
  char *host, *rest, *slh = "/";
  int sock, i;
  char *search = strstr(argv[1], "/");
    if(search != NULL){
      //found '/' in this case 
     rest = search;
    }
    else{ 
      rest = slh;
    }
  int szrest = 0;
  if (search != NULL)
    szrest = strlen(search);
  int szinp = strlen(argv[1]);
  host = malloc(szinp);
  memset(host, 0, szinp);
  strncpy(host, argv[1], szinp - szrest);

  // done processing string, host and rest have what we need.
   
  struct addrinfo hint;
  struct addrinfo *result, *temp;
  
  memset(&hint, 0, sizeof(struct addrinfo));
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  i = getaddrinfo(host, "80", &hint, &result);
  if (i != 0) { perror("error populating getaddrinfo struct!"); exit(1); }
  // done. 
  // try to open a socket
  // and connect to it
  for (temp = result; temp != NULL; temp = temp->ai_next){
    sock = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
    if (sock == -1) continue;
    
    if (connect(sock, temp->ai_addr,temp->ai_addrlen) != -1) break;
  }
  freeaddrinfo(result);
  // here we are connected, sock has our socket and we will
  // create and send serv req.
  char *getreq = malloc(szinp + 50);
  char  *loc = getreq;
  memset(getreq, '\0', szinp + 50);
  sprintf(getreq, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", rest, host);
  printf("%s\n", getreq); 
  while((i = send(sock, loc, strlen(loc), 0))){
    if (i < 0) { perror("error sending data to server"); exit(1); }
    loc+=i;
  }
  free(getreq);
  char index[] = "index.html";
  char *fname, *fnametemp;
  
  //parse out the filename to save as
  if (rest == slh) { fname = index; }
  else {
    fname = strtok(rest, "/");
    while((fnametemp = strtok(NULL, "/"))){
      fname = fnametemp;
    }
  }
  loc = malloc(512);
  memset(loc, '\0', 512);
  i = recv(sock, loc, 511, 0); 
  if (i < 0) { perror("error recieving data"); exit(0); }
  fnametemp = strstr(loc, "200 OK");
  if (!fnametemp) { fprintf(stderr, "page does not exist or was moved.\n"); exit(1); } //we don't care about any other case so we exit
  //
  //find the end of the resp header
  fnametemp = strstr(loc, "\r\n\r\n");
  while(!fnametemp){
    memset(loc, '\0', 512);
    i = recv(sock, loc, 511, 0);
    if (i == 0) { perror("error recieving data"); exit(0); }
   fnametemp = strstr(loc, "\r\n\r\n");
  }
  fnametemp += 4;
  int wfile = open(fname, O_CREAT|O_RDWR|O_APPEND|O_TRUNC, S_IREAD|S_IWRITE);
  write(wfile, fnametemp, i - (fnametemp - loc)); 
  memset(loc, '\0', 512);
  while ((i = recv(sock, loc, 511, 0))){
    if (i<0) { perror("error recieving"); exit(1); }
    write(wfile, loc, i);
    memset(loc, '\0', 512);
  }
  close(wfile);
  free(loc);
  free(host);
  shutdown(sock, SHUT_RDWR);
  return 0;
}
