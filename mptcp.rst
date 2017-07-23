NAME
====

    mptcp - the Multipath TCP protocol

SYNOPSIS
========

    | #include <linux/tcp.h>


DESCRIPTION
===========

Multipath TCP, defined in RFC6824, is an extension to the TCP protocol
that allows a connection to use multiple paths. The utilisation of 
Multipath TCP on Linux requires a kernel patch that is distributed
from https://www.multipath-tcp.org 

TO be continued

Multipath TCP contains several functional modules that can be 
configured independently. The path manager controls the creation of
subflows. The congestion control mechanism controls how the stack
reacts to congestion events on each subflow. The scheduler controls 
the subflow over which each data is transmitted. 


Path Managers
-------------

Three path managers are currently supported by the Multipath TCP
implementation in the Linux kernel. They control the advertisement
of addresses and the creation of subflows. You can select the
path manager that you want to use by setting the
**net.mptcp.mptcp_path_manager** variable to **default**, 
**fullmesh** or **ndiffports**.

default path manager
....................

The **default path manager** is a dummy path manager that does not
advertise any address and does not create subflows. It should be
used in combination with the enhanced socket API.

fullmesh path manager
......................

The **fullmesh** path manager tries to create a full-mesh of subflows
between the client and the server. On the client, this path manager 
listens to the addresses advertised by the server. It then
tries to create one subflow from each IP address on the client
towards each of the IP addresses advertised by the server,
including the address used for the initial subflow. On the server,
the path manager simply advertises all the server addresses and
does not create any subflow. Since MPTCP v0.90 it is possible to 
create multiple subflows for each pair of IP addresses. This
can be obtained by setting 
**/sys/module/mptcp_fullmesh/parameters/num_subflows** to the required
number of subflows (default value is 1).

Note that if the server and the clients are multihomed
and have several addresses, the utilisation of this path manager
may result in the creation of a large number of subflows. 


ndiffports path manager
.......................

This path manager is designed for single homed clients and servers
that are connected to a network that load balances the TCP
connections by using an Equal Cost MultiPath technique. 
In this case, Multipath TCP can obtain better performances 
than regular TCP by spreading
the load over several subflows that are routed over different
paths. The **ndiffports** path manager creates **n** subflows
from the IP address of the client by creating those subflows
with different source ports. The number of subflows can be controlled
by setting the **/sys/module/mptcp_ndiffports/parameters/num_subflow**
variable.

Schedulers
----------

When a Multipath TCP connection is composed of several active subflows,
the sender needs to decide over which subflow each new data will be 
transmitted. For each Multipath TCP connection, the implementation
maintains the list its subflows. A subflow is considered to
be active once its four-way handshake has finished. When new data
needs to be transmitted, any of the subflows in the active state can
be selected to transmit the data. 
This selection is done by the Multipath TCP scheduler. 

Three schedulers are currently available. They can be selected
at run-time provided that they have been compiled in the kernel. 
The scheduler is selected by setting the 
**net.mptcp.mptcp_scheduler** sysctl variable to **default**, 
**rounrobin** or **redundant**. 


The **default** scheduler
.........................

This scheduler is the recommended scheduler. When several subflows
are in the established state, the **default** scheduler always tries
to transmit each new data over the subflow with the lowest
round-trip-time that has space inside its congestion window. 
If the congestion window of a subflow is full, then this subflow cannot
be selected to transmit data until its congestion window reopens again.
Note that the round-trip-time is a dynamic metric that
evolves over time. An increased load on a subflow results usually in
an increased round-trip-time for this subflow. Furthermore, to avoid
transmitting data over lossy subflows, the scheduler tags a subflow as
potentially failed when the retransmission timer expires. The scheduler
ignores the potentially failed subflows if other subflows are available.
The potentially failed tag is removed when valid ata/ack is 
received on the subflow.


The **roundrobin** scheduler
............................

This scheduler tries to transmit data over the different subflows in a
round-robin fashion. When several subflows have space in their
congestion window, they are selected to transmit data one after
the other. By default, the **roundrobin** scheduler tries to send
one segment over a subflow before moving to the next one. However,
it is possible to configure the scheduler to send **b** segments over
each subflows by setting this value to the **num_segments** variable. 
This scheduler also uses the **cwnd_limited** boolean. When set to
**true** (the default value for this parameter), 
the scheduler will try to fill the congestion window on
all subflows. Otherwise (**cwnd_limited** set to **false**),
the scheduler will try to achieve real round-robin even if the
subflows have different bandwidth. This scheduler is not 
recommended for production and is mainly useful for testing purposes.

The **redundant** scheduler
...........................

This scheduler tries to transmit each data over all the subflows in
the established state. The objective is to reduce latency but
at the obvious cost of consuming much more bandwidth
This scheduler is not recommended.


Congestion control
------------------

Multipath TCP supports different congestion control algorithms like
regular TCP. Since the subflows of a Multipath TCP connection may
share the same bottleneck, the congestion control algorithm must
ensure that it is not unfair to regular TCP connections. This is
achieved by coupling the growth of the congestion window of the
different subflows. The available congestion controls are **lia** 
(alias Linked Increase Algorithm), **olia** (alias Opportunistic Linked 
Increase Algorithm), **wVegas** (alias Delay-based Congestion Control 
for MPTCP) and balia (alias Balanced Linked Adaptation Congestion 
Control Algorithm). The congestion control algorithm can
be changed by modifying the **sysctl net.ipv4.tcp_congestion_control**
variable.


Address formats
---------------

Multipath TCP is compatible with both IPv4 (**AF\_INET6**) and IPv6
(**AF\_INET6**). When a Multipath TCP connection is created, it
uses the address family of the associated socket. However, this does
not restrict the possibility of using another type of addresses
since the communicating hosts exchange their respective addresses
(both IPv4 and IPv6) during the connection.


/proc interfaces
----------------

System-wide Multipath TCP parameters can be accessed by files in the
**/proc/sys/net/mptcp** directory. Variables described as *Boolean* take an
integer value, with a nonzero value ("true") meaning that the
corresponding option is enabled, and a zero value ("false") meaning
that the option is disabled.

**mptcp_enabled** (Integer; default: 1; since MPTCP v0.87)

  Controls the utilisation of Multipath TCP. If set to **0**, Multipath
  TCP is completely disabled on the host. If set to **1**, Multipath
  TCP is enabled by default for all TCP connections on this host.
  If set to **2** (since MPTCP v0.89), Multipath TCP is disabled
  by default and the application has to use the **MPTCP\_ENABLED**
  socket option to enable the utilisation of Multipath TCP:

  |  int enable = 1;
  |  setsockopt(fd, IPPROTO_TCP, MPTCP_ENABLED, &enable, sizeof(enable));


**mptcp_checksum** (Boolean; default: enabled; since MPTCP v0.87)

  Enable/Disable the Multipath TCP checksum included in the DSS option.
  It is recommanded to only disable this checksum if it is known that
  there is no middlebox that could modify the TCP payload. Note
  that a Multipath TCP connection will use the DSS checksum if either
  of the communicating hosts has enabled it. Both communicating hosts
  need to disable mptcp_checksum by setting the **net.mptcp.mptcp_checksum**
  to zero to disable the DSS checksum on their connections. 

**mptcp_syn_retries** (Integer; default: 3; since MPTCP v0.87)

  There are unfortunately some middleboxes that block SYN segments
  containing the MP\_CAPABLE option. To deal with this problem and
  preserve connectivity, Multipath TCP can remove the MP\_CAPABLE 
  option from the SYN that it sends. The **net.mptcp.mptcp_syn_retries**
  variable controls the number of retransmissions of the SYN 
  with the MP\_CAPABLE option. 

**mptcp_version** (Integer; default: 0; since MPTCP v0.92)
 
  Controls the support for the ADD\_ADDR2 option as specified in 
  draft-ietf-mptcp-rfc6824bis. This is experimental since it is
  only a partial implementation of this option. It is not recommended
  for use in production at this stage. To use this new option, 
  set **systctl net.mptcp.mptcp_version* to **1**.

**mptcp_debug** (Boolean; default: disabled)

  Controls the utilisation of additional debugging output. Should only
  be enabled to debug a problem in the Multipath TCP implementation 


Socket API
----------

The simplest way to use Multipath TCP is to use the standard socket
API. You can thus use the standard sockets on a Multipath TCP-enabled
host to automatically benefit from Multipath TCP. The operation of
the underlying Multipath TCP connection will depend on the path managers
and schedulers that are configured as default on the host.

TODO

New in v0.92: Alternatively, you can select the path-manager through the socket-option MPTCP_PATH_MANAGER (defined as 44) by doing:
    char *pathmanager = "ndiffports";
    setsockopt(fd, SOL_TCP, MPTCP_PATH_MANAGER, pathmanager, sizeof(pathmanager));




Enhanced Socket API
-------------------

Patches have been recently introduced with an enhanced socket API for
Multipath TCP. This API entirely relies on socket options but
allows an application to manage the subflows that compose a 
Multipath TCP connection. This API allows to query the subflows
of an existing Multipath TCP connection, create a new subflow,
remove and existing subflow and apply socket options on specific 
subflows. These operations are realised through the **getsockopt** 
system call with the following options :

 - **MPTCP\_GET\_SUB\_IDS** is used to retrieve the identifiers of
   the subflows of the Multipath TCP connection associated to the socket
 - 


The symbols used by this enhanced API are defined in 
**/usr/include/linux/tcp.h**

The **MPTCP\_GET\_SUB\_IDS** socket option
...........................................

A Multipath TCP connection is composed of a number of subflows. This
number can change during the connection lifetime based on the
packet losses in the network and the reaction of the communicating
hosts. Inside the Linux kernel, each subflow is identified by
a unique identifier. The Linux Multipath TCP implementation
uses the **mptcp\_sub\_ids** structured, defined in
**<linux/tcp.h>** and shown below.

  | struct mptcp_sub_status {
  |     __u8     id;
  |     __u16    slave_sk:1,
  |              fully_established:1,
  |              attached:1,
  |              low_prio:1,
  |              pre_established:1;
  | };
  |
  | struct mptcp_sub_ids {
  |     __u8             sub_count;
  |     struct mptcp_sub_status sub_status[];
  | };


The **mptcp\_sub\_ids** structure contains a variable number of
subflows. The number of subflows is indicated with the **sub\_count**
field. The **sub\_status** contains the information related to each
of the active subflows. For each subflow, the **mptcp\_sub\_status**
structure contains the flow identifier (field **id**) and a flag
(field  **low\_prio**) that indicates whether the subflow is a
backup subflow (**low\_prio** set) or not.

**Benjamin** explain other flags.

To retrieve the list of subflow identifiers, you need to
pass a memory block that is large enough to store the information
about all the subflows of the connection. The current implementation
of Multipath TCP in the Linux kernel limits the number of subflows
to 32. If the memory block is not large enough, **getsockopt** returns
error EINVAL.

The code below illustrates the utilisation of the **MPTCP\_GET\_SUB\_IDS**
socket option.

    | struct mptcp_sub_ids *ids;
    | int i,err;
    | // expected length of the structure returned
    | optlen = 32*sizeof(struct mptcp_sub_status);  
    | ids = malloc(optlen);
    |
    | err=getsockopt(cud->sockfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
    |
    | printf("Number of subflows : %i\n", ids->sub_count);
    |
    | for(i = 0; i < ids->sub_count; i++){
    |         printf("\tI've got sub id : %i\n", ids->sub_status[i].id);
    |         printf("\t established : %i\n", ids->sub_status[i].fully_established);
    | }

The **MPTCP\_GET\_SUB\_TUPLE** socket option
............................................

The **MPTCP\_GET\_SUB\_TUPLE** allows to retrieve the local and
remote endpoints of a specific subflow. It uses the 
**mptcp\_sub\_tuple** structure defined in **<linux/tcp.h>**. This
structure contains two fields :

   | struct mptcp_sub_tuple {
   |      __u8 id;
   |      __u8 addrs[0];
   | };

The first field (**id**) is the identifier of the subflow. The
second field (**addrs**) is a pair of addresses. This structure has
a variable length since it may contain either two IPv6 or two IPv4
addresses.

By using the **MPTCP\_GET\_SUB\_TUPLE** socket option, an application
can perform the equivalent of **getpeername** for a specific subflow.
This is illustrated in the code snipet below.

**Benjamin** provide exact optlen

   | struct mptcp_sub_tuple *sub_tuple;
   | struct sockaddr *sin;
   | optlen = 100;
   | sub_tuple = malloc(optlen); 
   | sub_tuple->id = sub_id; // subflow identifier
   | error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_TUPLE, sub_tuple, &optlen);

**Benjamin** explain return codes and errors

Once the **mptcp\_sub\_tuple** structure has been retrieved, the 
application can easily extract the addresses of the local and
remote endpoints as follows.

   |  sin = (struct sockaddr*) &sub_tuple->addrs[0];
   |
   |  if(sin->sa_family == AF_INET) {
   |          struct sockaddr_in *sin4;
   |          // local endpoint
   |          sin4 = (struct sockaddr_in*) &sub_tuple->addrs[0];
   |          sport = ntohs(sin4->sin_port);
   |          inet_ntop(AF_INET, &sin4->sin_addr.s_addr, sbuff,INET_ADDRSTRLEN);
   |          sin4++;
   |          // remote endpoint
   |          dport =  ntohs(sin4->sin_port);
   |          inet_ntop(AF_INET, &sin4->sin_addr.s_addr, dbuff, INET_ADDRSTRLEN);
   |  }
   |  if(sin->sa_family == AF_INET6) {
   |          struct sockaddr_in6 *sin6;
   |          // local endpoint
   |          sin6 = (struct sockaddr_in6*) &sub_tuple->addrs[0];
   |          sport = ntohs(sin6->sin6_port);
   |          inet_ntop(AF_INET6, &sin6->sin6_addr.s6_addr, sbuff,INET6_ADDRSTRLEN);
   |          sin6++;
   |          // remote endpoint
   |          dport =  ntohs(sin6->sin6_port);
   |          inet_ntop(AF_INET6, &sin6->sin6_addr.s6_addr, dbuff, INET6_ADDRSTRLEN);
   | }
   |
   |  printf("\tip src : %s src port : %hu\n", sbuff, sport);
   |  printf("\tip dst : %s dst port : %hu\n", dbuff, dport);


The **MPTCP\_OPEN\_SUB\_ID** socket option
...........................................

The **MPTCP\_OPEN\_SUB\_ID** socket option allows to manually create an
additional subflow on an existing Multipath TCP connection.


(...)
void duplicate_sub(int sockfd, uint8_t sub_id, uint16_t sport){
	int error;
	unsigned int optlen;
	struct mptcp_sub_tuple *sub_tuple;
	struct sockaddr *sin;

	optlen = 100;

	sub_tuple = malloc(optlen);
	if (!sub_tuple) {
		return ;
	}
	sub_tuple->id = sub_id;

	error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_GET_SUB_TUPLE,
sub_tuple,
			&optlen);
	if (error) {
		db_bug("Ooops something went wrong with get tuple
!%s","\n");
		free(sub_tuple);
		return;
	}

        sin = (struct sockaddr*) &sub_tuple->addrs[0];

        if(sin->sa_family == AF_INET) {
                struct sockaddr_in *sin4;
                sin4 = (struct sockaddr_in*) &sub_tuple->addrs[0];
                sin4->sin_port = htons(sport);
        }
        else {
                struct sockaddr_in6 *sin6;
                sin6 = (struct sockaddr_in6*) &sub_tuple->addrs[0];
                sin6->sin6_port = htons(sport);
        }

	error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_OPEN_SUB_TUPLE,
sub_tuple,
			&optlen);
	if (error) {
		perror(NULL);
		db_bug("Ooops something went wrong when duplicate tuple
!%s","\n");
		free(sub_tuple);
		return;
	}

	free(sub_tuple);
}

(...)



The **MPTCP\_CLOSE\_SUB\_ID** socket option
...........................................

close

Add a socket option to close the subflow based on its id.

Example of C user space code :

struct mptcp_close_sub_id close_id;

optlen = sizeof(struct mptcp_close_sub_id);
close_id.id = 2;

getsockopt(cud->sockfd, IPPROTO_TCP, MPTCP_CLOSE_SUB_ID, &close_id,
           &optlen);



EXAMPLES
========








socket options

struct mptcp_sub_getsockopt sub_gso;
struct tcp_info ti;

(...)

optlen = sizeof(struct mptcp_sub_getsockopt);
sub_optlen = sizeof(struct tcp_info);
sub_gso.id = sub_id;
sub_gso.level = IPPROTO_TCP;
sub_gso.optname = TCP_INFO;
sub_gso.optlen = &sub_optlen;
sub_gso.optval = (char *) &ti;

error =  getsockopt(sockfd, IPPROTO_TCP, MPTCP_SUB_GETSOCKOPT,
                   &sub_gso, &optlen);
if (error) {
        db_bug("Ooops something went wrong with get info !%s","\n");
        free(sub_tuple);
        return;
}
db_info("\tGeneric...\tBytes acked %llu\n", ti.tcpi_bytes_received);


** set sockopt

struct mptcp_sub_setsockopt sub_sso;
int val = 12;

optlen = sizeof(struct mptcp_sub_setsockopt);
sub_optlen = sizeof(int);
sub_sso.id = sub_id;
sub_sso.level = IPPROTO_IP;
sub_sso.optname = IP_TOS;
sub_sso.optlen = sub_optlen;
sub_sso.optval = (char *) &val;

error =  setsockopt(sockfd, IPPROTO_TCP, MPTCP_SUB_SETSOCKOPT,
		&sub_sso, optlen);
if (error) {
    db_bug("Ooops something went wrong with set tos !%s","\n");
    return;
}


BUGS
====

SEE ALSO
========
