#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


// required for MPTCP socket API

#include <linux/tcp.h>


/* simple MPTCP client */

#define PORT "80" // the port client will be connecting to
#define SERVER "www.multipath-tcp.org"
#define BUFSIZE 8000


void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char **argv) {

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int err, numbytes;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char *get="HEAD / HTTP/1.0\nHost:www.multipath-tcp.org\n\n";

  char buf[BUFSIZE];
  char *hostname=SERVER; 

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype,
		    p->ai_protocol);
    if(sockfd<0) {
      continue;
    }
    else {
      int enable=1;
      err=setsockopt(sockfd,IPPROTO_TCP,MPTCP_ENABLED, &enable, sizeof(enable));
    }
    // try to connect with this socket
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      fprintf(stderr, "connect :\n");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect to %s \n",hostname);
    return 2;
  }

  //  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
  //          s, sizeof s);

  freeaddrinfo(servinfo); // all done with this structure


  /*  struct mptcp_sub_ids *ids;
  int i,j,optlen;
  // expected length of the structure returned
  optlen = 4+32*sizeof(struct mptcp_sub_status);
  ids = (struct mptcp_sub_ids *)malloc(optlen);
  if(ids==NULL) {
    perror("malloc");
    exit(-1);
  }
  */
  numbytes=send(sockfd, get, strlen(get),0);
  
  if ((numbytes = recv(sockfd, buf, BUFSIZE, 0)) == -1) {
    exit(1);
  }

  printf("client: received '%s'\n",buf);

  close(sockfd);

}
