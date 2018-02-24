#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

#define DEBUG2(s1,s2) (debug_print[debug])(s1,s2,__LINE__)
#define DEBUG(s1) (debug_print[debug])(s1,NULL,__LINE__)


int debug = 0;  // debug on or off
int v6only =0;  // only use IPv6
int v4only =0;  // only use IPv4


void empty (char *str,char *str2, int line) {
  return;
}

void oneline(char *str, char *str2, int line) {
  if(str2!=NULL) {
    fprintf(stderr,"debug [%d]: %s %s \n" ,line,str,str2);
  } else {
    fprintf(stderr,"debug [%d]: %s \n" ,line,str);
  }
}

void (* debug_print[])(char *, char *, int) = { empty, oneline };


void send_data(int sockfd) {
  char *str="HEAD / /HTTP/1.0\nX-Header: hjhsqkdjhskdjhqskjdhsqkjhsqkjdhqskjhdksqjd\n\n";
  int i;
  for(i=0;i<strlen(str);i++) {
    int err=send(sockfd,str+i,1,0);
    sleep(1);
    
  }

}

/*
 * print TCP_INFO structure
 */
void print_tcp_info(struct tcp_info *ti) {
  if(ti==NULL)
    return;

  printf("tcpi_state: %d\n", ti->tcpi_state);
  printf("tcpi_ca_state: %d\n", ti->tcpi_ca_state);
  printf("tcpi_retransmits: %d\n",ti->tcpi_retransmits);
  printf("tcpi_probes: %d\n",ti->tcpi_probes);
  printf("tcpi_backoff: %d\n",ti->tcpi_backoff);
  printf("tcpi_options: %d\n",ti->tcpi_options);
  printf("tcpi_snd_wscale : %d\n",ti->tcpi_snd_wscale);
  printf("tcpi_rcv_wscale : %d\n",ti->tcpi_rcv_wscale);

  printf("tcpi_rto: %d\n",ti->tcpi_rto);
  printf("tcpi_ato: %d\n",ti->tcpi_ato);
  printf("tcpi_snd_mss : %d\n",ti->tcpi_snd_mss);
  printf("tcpi_rcv_mss : %d\n",ti->tcpi_rcv_mss);

  printf("tcpi_unacked : %d\n",ti->tcpi_unacked);
  printf("tcpi_sacked : %d\n",ti->tcpi_sacked);
  printf("tcpi_lost : %d\n", ti->tcpi_lost);
  printf("tcpi_retrans : %d\n",ti->tcpi_retrans);
  printf("tcpi_fackets : %d\n",ti->tcpi_fackets);
  
  /* times */

  printf("tcpi_last_data_sent : %d\n",ti->tcpi_last_data_sent);
  //u_int32_ttcpi_last_ack_sent;/* Not remembered, sorry.  */
  printf("tcpi_last_data_recv : %d\n",ti->tcpi_last_data_recv);
  printf("tcpi_last_ack_recv : %d\n",ti->tcpi_last_ack_recv);

  /* metrics */

  printf("tcpi_pmtu : %d\n",ti->tcpi_pmtu);
  printf("tcpi_rcv_ssthresh : %d\n",ti->tcpi_rcv_ssthresh);
  printf("tcpi_rtt : %d\n",ti->tcpi_rtt);
  printf("tcpi_rttvar : %d\n",ti->tcpi_rttvar);
  printf("tcpi_snd_ssthresh : %d \n",ti->tcpi_snd_ssthresh);
  printf("tcpi_snd_cwnd : %d\n",ti->tcpi_snd_cwnd);
  printf("tcpi_advmss : %d \n",ti->tcpi_advmss);
  printf("tcpi_reordering : %d\n",ti->tcpi_reordering);

  printf("tcpi_rcv_rtt : %d\n",ti->tcpi_rcv_rtt);
  printf("tcpi_rcv_space : %d\n",ti->tcpi_rcv_space);

  printf("tcpi_total_retrans : %d\n",ti->tcpi_total_retrans);

}

/* 
 *
 */
int get_all_tcp_info(int sockfd, struct tcp_info *ti) {
  struct mptcp_sub_ids *ids;
  int i,err;
  int optlen = MAX_SUBFLOWS*sizeof(struct mptcp_sub_status);
  ids = (struct mptcp_sub_ids *) malloc(optlen);
  if(ids==NULL) {
    return -1;
  }
   
  err=getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
  if(err<0) {
    return -1;
  }
  for(i = 0; i < ids->sub_count; i++){    
    struct tcp_info ti;
    printf("\n\nTCP_INFO for %d \n",ids->sub_status[i].id);
    err=get_tcp_info(sockfd, ids->sub_status[i].id, &ti);
    if(err<0) {
      return -1;
    }
    print_tcp_info(&ti);
  }
}


/*
 *  retrieves the TCP_INFO structure for the corresponding subflow
 */
int get_tcp_info(int sockfd, int subflow, struct tcp_info *ti) {
  if( (sockfd<0) || (ti==NULL))
    return -1;

  struct mptcp_sub_getsockopt sub_gso;
    
  int optlen = sizeof(struct mptcp_sub_getsockopt);
  int sub_optlen = sizeof(struct tcp_info);
  sub_gso.id = subflow;
  sub_gso.level = IPPROTO_TCP;
  sub_gso.optname = TCP_INFO;
  sub_gso.optlen = &sub_optlen;
  sub_gso.optval = (char *) ti;

  int error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_SUB_GETSOCKOPT,
			  &sub_gso, &optlen);
  if (error) {
    DEBUG2("Ooops something went wrong with get info !%s","\n");
    return -1;
  }
  return 0;
}

/*
 *  returns the TCP_USER_TIMEOUT of the specified subflow in milliseconds
 */
int get_tcp_timeout(int sockfd, int subflow) {
  if(sockfd<0)
    return -1;

  struct mptcp_sub_getsockopt sub_gso;
    
  int optlen = sizeof(struct mptcp_sub_getsockopt);
  int sub_optlen = sizeof(struct tcp_info);
  int timeout;
  sub_gso.id = subflow;
  sub_gso.level = IPPROTO_TCP;
  sub_gso.optname = TCP_USER_TIMEOUT;
  sub_gso.optlen = &sub_optlen;
  sub_gso.optval = (char *) &timeout;

  int error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_SUB_GETSOCKOPT,
			  &sub_gso, &optlen);
  if (error) {
    DEBUG2("Ooops something went wrong with get info !%s","\n");
    return -1;
  }
  return 0;
}

/*
 *  Set the USER_TIMEOUT on a specific subflow
 */
int set_tcp_timeout(int sockfd, int subflow, int timeout) {
  if(sockfd<0) 
    return -1;

  struct mptcp_sub_setsockopt sub_sso;
  
  int optlen = sizeof(struct mptcp_sub_setsockopt);
  int sub_optlen = sizeof(timeout);
  sub_sso.id = subflow;
  sub_sso.level = IPPROTO_TCP;
  sub_sso.optname = TCP_USER_TIMEOUT;
  sub_sso.optlen = sub_optlen;
  sub_sso.optval = (char *) &timeout;

  int error =  setsockopt(sockfd, IPPROTO_TCP, MPTCP_SUB_SETSOCKOPT,
			  &sub_sso, optlen);
  if (error) {
    DEBUG2("Ooops something went wrong with set sockopt !%s","\n");
    return -1;
  }
  return 0;
}

/*
 * creates one additional subflow for this MPTCP connection
 * if src==NULL, selects one of the addresses used as source by
 * the mptcp connection
 */
int add_subflow(int sockfd, char *server, char * port_str) {
  int port=(int)strtol(port_str, (char **)NULL, 10);
 
  if(port<=0 || port >65535) {
    return -1;
  }  
  struct sockaddr_in *src;
  struct sockaddr_in *dest;

  if(sockfd <0) {
    return -1;
  }
  if(server==NULL) {
    return -1;
  }

  // DNS lookup
  int err;
  struct addrinfo hints, *servinfo, *ptr;

  memset(&hints, 0, sizeof hints);
  if(v4only) {
    hints.ai_family = AF_INET;
  } else if(v6only) {
        hints.ai_family = AF_INET6;
  } else {
    hints.ai_family = AF_UNSPEC;
  }
  hints.ai_socktype = SOCK_STREAM;

  if ((err = getaddrinfo(server, port_str, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    return -1;
  }
  
  for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
    // try to create the subflow
    //(ptr->ai_addr, ptr->ai_addrlen);
    struct mptcp_sub_tuple *new_sub_tuple;
    struct sockaddr_in *addr;
    int err;
  
    int optlen = sizeof(struct mptcp_sub_tuple) + 2 * sizeof(struct sockaddr_in);
    new_sub_tuple = malloc(optlen);

    new_sub_tuple->id = 0;
    //    new_sub_tuple->prio = 0;

    addr = (struct sockaddr_in*) &new_sub_tuple->addrs[0];
    // client

    addr->sin_family = AF_INET;
    addr->sin_port = htons(0);
    inet_pton(AF_INET, "0.0.0.0", &addr->sin_addr);

    addr++;
    // server
    addr->sin_family = ptr->ai_family;
    memcpy(addr,ptr->ai_addr,ptr->ai_addrlen);

    err =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_OPEN_SUB_TUPLE,
		    new_sub_tuple, &optlen);
    if(err==0) {
      return err;
    }
    return -1;
  }
}

/*
 * kills the specified subflow and replaces it with a new subflow
 */
int refresh_subflow(int sockfd, int subflow) {

  if(sockfd <0) {
    return -1;
  }

 
  struct mptcp_sub_tuple *new_sub_tuple;
  struct sockaddr_in *addr;

  int error;
  struct mptcp_sub_tuple *sub_tuple;
  struct sockaddr *sin;

  int optlen = sizeof(struct mptcp_sub_tuple) + 2 * sizeof(struct sockaddr_in);
  
  sub_tuple = (struct mptcp_sub_tuple *)malloc(optlen);
  if (!sub_tuple) {
    return -1;
  }
  
  sub_tuple->id = subflow;

  error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_TUPLE,
		      sub_tuple,
		      &optlen);


  if (error) {
    DEBUG2("Ooops something went wrong with get tuple !%s","\n");
    free(sub_tuple);
    return -1;
  }


  sin = (struct sockaddr*) &sub_tuple->addrs[0];

  if(sin->sa_family == AF_INET) {
    struct sockaddr_in *sin4;
    sin4 = (struct sockaddr_in*) &sub_tuple->addrs[0];
  }
  else {
    struct sockaddr_in6 *sin6;
    sin6 = (struct sockaddr_in6*) &sub_tuple->addrs[0];
  }

  error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_OPEN_SUB_TUPLE,
		      sub_tuple,
		      &optlen);

  free(sub_tuple);

  if (error) {
    DEBUG2("Ooops something went wrong when duplicate tuple !%s","\n");
    return -1;
  }

  // closing subflow

  struct mptcp_close_sub_id close_id;

  optlen = sizeof(struct mptcp_close_sub_id);
  close_id.id = subflow;

  error=getsockopt(sockfd, IPPROTO_TCP, MPTCP_CLOSE_SUB_ID, &close_id,
	     &optlen);

  if (error) {
    DEBUG2("Ooops something went wrong when closing subflow !%s","\n");
    return -1;
  }
 
  return 0;
}

int get_all_subids(int sockfd, int ) {

 /*
  optlen = MAX_SUBFLOWS*sizeof(struct mptcp_sub_status);

  struct mptcp_sub_ids *ids;
  ids = (struct mptcp_sub_ids *) malloc(optlen);
  if(ids==NULL) {
    DEBUG2("Ooops something went wrong when using malloc !%s","\n");
    return -1;
  }
   
  error=getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
  free(ids);
  if(error<0) {
    DEBUG2("Ooops something went wrong with getsockopt MPTCP_GET_SUB_IDS!%s","\n");
    return -1;
  }
  if(ids->sub_count>0) {
    
    struct mptcp_close_sub_id close_id;

    optlen = sizeof(struct mptcp_close_sub_id);
    close_id.id = ids->sub_status[0].id;

    error=getsockopt(sockfd, IPPROTO_TCP, MPTCP_CLOSE_SUB_ID, &close_id,
	       &optlen);
    if(error<0) {
      DEBUG2("Ooops something went wrong with getsockopt MPTCP_GET_CLOSE_ID!%s","\n");
      return -1;
    }
    DEBUG("Closed subflow ");
  }
  */

}


/*
 * server is the name of the server that needs to be contacted
 * port is the port number.
 * Creates an MPTCP connection with one subflow towards this
 * server and returns the socket
 */

int mptcp_connect(char *server, char * port_str) {
  int port=(int)strtol(port_str, (char **)NULL, 10);
 
  if(port<=0 || port >65535) {
    return -1;
  }
  int err;
  int sockfd;
  struct addrinfo hints, *servinfo, *ptr;

  memset(&hints, 0, sizeof hints);
  if(v4only) {
    hints.ai_family = AF_INET;
  } else if(v6only) {
        hints.ai_family = AF_INET6;
  } else {
    hints.ai_family = AF_UNSPEC;
  }
  hints.ai_socktype = SOCK_STREAM;

  if ((err = getaddrinfo(server, port_str, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    return -1;
  }

  for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
    sockfd = socket(ptr->ai_family, ptr->ai_socktype,
		    ptr->ai_protocol);
    if(sockfd== -1) {
      continue;
    }

    err=connect(sockfd, ptr->ai_addr, ptr->ai_addrlen);
    if(err==-1) {
      close(sockfd);
      continue;
    }
    // socket is now connected
    return(sockfd);
  }

  if (ptr == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return -1;
  }
}


int main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind;
  int c, err = 0; 
  //  char *sname = "default_sname", *fname;//
  static char usage[] = "usage: %s [-d64] [-p port] server\n";
  char *server;
  char *port_str="80";    // default port
  int portnum=80;      // default port
  while ((c = getopt(argc, argv, "d64f:")) != -1)
    switch (c) {
    case 'd':
      debug = 1;
      DEBUG("debug mode enabled");
      break;
    case '6':
      v6only = 1;
      break;
    case '4':
      v4only = 1;
      break;
    case 'p':
      port_str = optarg;
      break;
    case '?':
      fprintf(stderr, usage, argv[0]);
      exit(1);
      break;
    }
  if(v4only && v6only) {
    fprintf(stderr,"Cannot request IPv4 only and IPv6 only at the same time.\n");
    fprintf(stderr, usage, argv[0]);
    exit(1);
  }
  if ((optind+1)!=argc) {
    fprintf(stderr, "%s: missing server name\n", argv[0]);
    fprintf(stderr, usage, argv[0]);
    exit(1);
  }
  server=argv[optind];


  int sockfd=mptcp_connect(server,port_str);
  //  char *pathmanager = "ndiffports";
  // err=setsockopt(sockfd, IPPROTO_TCP, MPTCP_PATH_MANAGER, pathmanager, sizeof(pathmanager));
  //  printf("err: %d");

  if(sockfd<0) {
    exit(1);
  }

  
  err=set_tcp_timeout(sockfd,1,10); // in milliseconds
  printf("user timeout: %d\n",get_tcp_timeout(sockfd,1));
  
  struct tcp_info ti;
  err=get_tcp_info(sockfd,1,&ti);
  printf("err=%d\n",err);
  print_tcp_info(&ti);
  err=add_subflow(sockfd,server, port_str);
  //err=add_subflow(sockfd,server, "8000");

  send_data(sockfd);
  err=get_all_tcp_info(sockfd,NULL);
}
