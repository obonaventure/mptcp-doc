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

// required for MPTCP

#include <linux/tcp.h>
#define MAX_SUBFLOWS 32

/* Example for the MPTCP get sub sockeet option */

#define PORT 80 // the port client will be connecting to
#define PORT_STR "80"
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
  char *get="HEAD / HTTP/1.0\nHost:www.multipath-tcp.org\nX-Header:jlkjlksjdlkqsjdlsqkjlqsjdlqsjdlqjdlkqsjdlksqjdlkqsjdlksqjd\n\n";
  char buf[BUFSIZE];
  char *hostname="www.multipath-tcp.org"; // "163.172.27.136"; //

  struct sockaddr *server;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;


  if ((rv = getaddrinfo(hostname, PORT_STR, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      fprintf(stderr, "failed to connect :\n");
      continue;
    }
    server=p->ai_addr;
    if(server->sa_family==AF_INET) {
      struct sockaddr_in * sin4;
      sin4=(struct sockaddr_in *)server;
      printf("server port :%d\n",ntohs(sin4->sin_port));
    }
    
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo); // all done with this structure


  struct mptcp_sub_ids *ids;
  int i,j,optlen;
  // expected length of the structure returned
  optlen =MAX_SUBFLOWS * sizeof(struct mptcp_sub_status);
  ids = (struct mptcp_sub_ids *)malloc(optlen);
  if(ids==NULL) {
    perror("malloc");
    exit(-1);
  }

  printf("sending %d bytes\n",(int)strlen(get));

  for(j=0; j<strlen(get);j+=2) {

    err=getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen );
    printf("err=%d\n",err);
    if(err==-1)
      perror("sockopt");
    printf("Number of subflows : %i\n", ids->sub_count);

    for(i = 0; i < ids->sub_count; i++){
      printf("\tI've got sub id : %i\n", ids->sub_status[i].id);
      printf("\t established : %i\n", ids->sub_status[i].fully_established);
      printf("\t attached : %i\n", ids->sub_status[i].attached);
      printf("\t low_prio : %i\n", ids->sub_status[i].low_prio);
      printf("\t pre_established : %i\n", ids->sub_status[i].pre_established);


      struct mptcp_sub_tuple *sub_tuple;
      struct sockaddr *sin;
      optlen = sizeof(struct mptcp_sub_tuple)+2*sizeof(struct sockaddr_in6); // max length of structure with IPv6
      sub_tuple = (struct mptcp_sub_tuple *)malloc(optlen);
      sub_tuple->id = ids->sub_status[i].id; // subflow identifier
      printf("subflow :%d\n",i);
      err =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_TUPLE, sub_tuple, &optlen);
      if(err<0)
	perror("getsockopt");
      sin = (struct sockaddr*) &sub_tuple->addrs[0];
      int sport, dport;
      if(sin->sa_family == AF_INET) {
	struct sockaddr_in *sin4;
	char sbuff[INET_ADDRSTRLEN+1];
	char dbuff[INET_ADDRSTRLEN+1];
	// local endpoint
        sin4 = (struct sockaddr_in*) &sub_tuple->addrs[0];
	sport = ntohs(sin4->sin_port);
	inet_ntop(AF_INET, &sin4->sin_addr.s_addr, sbuff,INET_ADDRSTRLEN);
	sin4++;
	// remote endpoint
        dport =  ntohs(sin4->sin_port);
	inet_ntop(AF_INET, &sin4->sin_addr.s_addr, dbuff, INET_ADDRSTRLEN);
	printf("\tip src : %s src port : %hu\n", sbuff, sport);
	printf("\tip dst : %s dst port : %hu\n", dbuff, dport);

      }
      if(sin->sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6;
	char sbuff[INET6_ADDRSTRLEN+1];
	char dbuff[INET6_ADDRSTRLEN+1];
	// local endpoint
	sin6 = (struct sockaddr_in6*) &sub_tuple->addrs[0];
	sport = ntohs(sin6->sin6_port);
	inet_ntop(AF_INET6, &sin6->sin6_addr.s6_addr, sbuff,INET6_ADDRSTRLEN);
	sin6++;
	// remote endpoint
	dport =  ntohs(sin6->sin6_port);
	inet_ntop(AF_INET6, &sin6->sin6_addr.s6_addr, dbuff, INET6_ADDRSTRLEN);
	printf("\tip src : %s src port : %hu\n", sbuff, sport);
	printf("\tip dst : %s dst port : %hu\n", dbuff, dport);

       }

      if ( rand() < (RAND_MAX/10)) {
	printf("closing subflow %d\n",ids->sub_status[i].id);
	struct mptcp_close_sub_id close_id;
	
	optlen = sizeof(struct mptcp_close_sub_id);
	close_id.id = ids->sub_status[i].id;
      
	getsockopt(sockfd, IPPROTO_TCP, MPTCP_CLOSE_SUB_ID, &close_id,
		   &optlen);
      }
   
    }
    numbytes=send(sockfd, (char *)(get+j), 2,0);
    printf("sent %d bytes\n",numbytes);
    sleep(5);
    
    // create additional subflow

    struct mptcp_sub_tuple *new_sub_tuple;
    struct sockaddr_in *addr;
    int err;

    optlen = sizeof(struct mptcp_sub_tuple) + 2 * sizeof(struct sockaddr_in);
    new_sub_tuple = malloc(optlen);
    
    new_sub_tuple->id = 0;
    //    new_sub_tuple->prio = 0;

    addr = (struct sockaddr_in*) &new_sub_tuple->addrs[0];
    // client

    addr->sin_family = AF_INET;
    addr->sin_port = htons(0);
    inet_pton(AF_INET, "192.168.33.10", &addr->sin_addr);
    
    addr++;
    // server
    addr->sin_family = AF_INET;
    addr->sin_port = htons(80);
    inet_pton(AF_INET, "130.104.230.45", &addr->sin_addr);

    printf("adding new subflows %d\n",j);

    err =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_OPEN_SUB_TUPLE,
			new_sub_tuple, &optlen);
    


  }

  if ((numbytes = recv(sockfd, buf, BUFSIZE, 0)) == -1) {
    exit(1);
  }

  printf("client: received '%s'\n",buf);

  close(sockfd);

}
