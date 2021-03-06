/*
	Glidix kernel

	Copyright (c) 2014-2017, Madd Games.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.
	
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __glidix_socket_h
#define __glidix_socket_h

#include <glidix/util/common.h>
#include <glidix/net/netif.h>
#include <glidix/fs/vfs.h>
#include <glidix/thread/semaphore.h>

#define	SOCK_STREAM			1
#define	SOCK_DGRAM			2
#define	SOCK_RAW			3
#define	SOCK_SEQPACKET			4

#define	SOCK_TYPEMASK			0xF
#define	SOCK_CLOEXEC			(1 << 4)
#define	SOCK_ALLFLAGS			(SOCK_TYPEMASK | SOCK_CLOEXEC)
#define	SOCK_BITFLAGS			(SOCK_CLOEXEC)

#define	SHUT_RD				1
#define	SHUT_WR				2
#define	SHUT_RDWR			3

#define	MSG_EOR				(1 << 0)
#define	MSG_OOB				(1 << 1)
#define	MSG_PEEK			(1 << 2)
#define	MSG_WAITALL			(1 << 3)

#define	GSO_RCVTIMEO			0
#define	GSO_SNDTIMEO			1
#define	GSO_SNDFLAGS			2
#define	GSO_UNICAST_HOPS		3
#define	GSO_MULTICAST_HOPS		4
#define	GSO_COUNT			5

#define	MCAST_JOIN_GROUP		0
#define	MCAST_LEAVE_GROUP		1

#define	MCAST_MAX_GROUPS		256

/* return values from packet() */
#define	SOCK_CONT			0			/* continue looking for sockets (considered the default) */
#define	SOCK_STOP			1			/* stop looking for sockets (used mainly by TCP to avoid new connections arriving at a listening socket unnecessarily) */

typedef struct Socket_
{
	File*				fp;
	int				domain, type, proto;
	
	struct Socket_*			prev;
	struct Socket_*			next;
	
	/**
	 * Socket options.
	 */
	uint64_t			options[GSO_COUNT];
	
	/**
	 * Name of the interface to which the socket is bound. Empty string means all interfaces.
	 */
	char				ifname[16];

	/**
	 * Called to handle a bind() on this socket.
	 */
	int (*bind)(struct Socket_ *sock, const struct sockaddr *addr, size_t addrlen);
	
	/**
	 * Called to handle a write(), send() or sendto(). For write() and send(), "addr" is set to NULL
	 * and "addrlen" to 0; for sendto(), they are the values passed to the function.
	 */
	ssize_t (*sendto)(struct Socket_ *sock, const void *message, size_t size, int flags, const struct sockaddr *addr, size_t addrlen);
	
	/**
	 * Similarly, this is called by read(), recv() and recvfrom().
	 */
	ssize_t (*recvfrom)(struct Socket_ *sock, void *buffer, size_t len, int flags, struct sockaddr *addr, size_t *addrlen);
	
	/**
	 * This is called by close() and shutdown(). In the case of close(), 'how' is set to SHUT_RDWR.
	 */
	void (*shutdown)(struct Socket_ *sock, int how);
	
	/**
	 * This is called by close() after calling shutdown(). This function is responsible for freeing any resources
	 * used by the socket. You are also repsonsible for freeing the socket itself. If this function is NULL, the
	 * socket is freed by close() itself. The socket is to be freed using FreeSocket().
	 */
	void (*close)(struct Socket_ *sock);

	/**
	 * This is called on every socket upon the reception of a packet. "src" and "dest" represent the source and
	 * destination address of the packet (they both have the same address family, either AF_INET or AF_INET6).
	 * "addrlen" is the size of both address structures. "packet" and "size" point to the packet (excluding the
	 * IP header), and specify the size of the packet. "proto" is the protocol given on the IP header.
	 *
	 * Returns one of the statuses:
	 *	SOCK_CONT (typical status) - the kernel shall keep looking for sockets to put the packet in.
	 *	SOCK_STOP - do not look for more sockets
	 */
	int (*packet)(struct Socket_ *sock, const struct sockaddr *src, const struct sockaddr *dest, size_t addrlen,
			const void *packet, size_t size, int proto);

	/**
	 * Called to handle a connect() on this socket.
	 */
	int (*connect)(struct Socket_ *sock, const struct sockaddr *addr, size_t addrlen);
	
	/**
	 * Called to handle getsockname() on this socket.
	 */
	int (*getsockname)(struct Socket_ *sock, struct sockaddr *addr, size_t *addrlenptr);
	
	/**
	 * Called to handle getpeername() on this socket.
	 */
	int (*getpeername)(struct Socket_ *sock, struct sockaddr *addr, size_t *addrlenptr);
	
	/**
	 * Called to join/leave an IPv6 multicast group. 'op' is either MCAST_JOIN_GROUP or MCAST_LEAVE_GROUP.
	 */
	int (*mcast)(struct Socket_ *sock, int op, const struct in6_addr *addr, uint32_t scope);
	
	/**
	 * Get polling information; see vfs.h.
	 */
	void (*pollinfo)(struct Socket_ *sock, Semaphore **sems);
	
	/**
	 * Called to handle listen() on this socket.
	 */
	int (*listen)(struct Socket_ *sock, int backlog);
	
	/**
	 * Called to handle accept() on this socket. Returns NULL on error (and sets ERRNO), otherwise
	 * returns the new socket.
	 */
	struct Socket_* (*accept)(struct Socket_ *sock, struct sockaddr *addr, size_t *addrlenptr);
	
	/**
	 * Get the current socket error and clear it.
	 */
	int (*geterr)(struct Socket_ *sock);
} Socket;

typedef struct
{
	uint8_t			type;
	uint8_t			code;
	uint16_t		checksum;
	uint32_t		rest;
	char			payload[];
} ICMPPacket;

typedef struct
{
	uint8_t			type;
	uint8_t			code;
	uint16_t		checksum;
	uint16_t		id;
	uint16_t		seq;
	char			payload[];
} PingPongPacket;
	
/**
 * This is called from onPacket() once an address is obtained. This will try to deliver the packet to all interested
 * sockets.
 */
void passPacketToSocket(const struct sockaddr *src, const struct sockaddr *dest, size_t addrlen,
			const void *packet, size_t size, int proto, uint64_t dataOffset, const char *ifname);

/**
 * Called by passPacketToSocket() or the IP reassembler once we have a full transport-layer packet.
 */
void onTransportPacket(const struct sockaddr *src, const struct sockaddr *dest, size_t addrlen, const void *packet, size_t size, int proto, const char *ifname);

/**
 * Create a socket file description. The returned description will be marked with the O_SOCKET flag, and the 'fsdata'
 * field will point to a structure prefixed with a Socket structure. On error, NULL is returned and errno set accordingly.
 */
File* CreateSocket(int domain, int type, int proto);

/**
 * Attempts to bind the socket referred to by the given file description to the specified address. The return value and errno
 * setting are standard for the POSIX bind() function. Returns ENOTSOCK if the description is not a socket.
 */
int BindSocket(File *fp, const struct sockaddr *addr, size_t addrlen);

/**
 * Sends a message through a socket - this implements both send() and sendto() userspace functions.
 */
ssize_t SendtoSocket(File *fp, const void *message, size_t len, int flags, const struct sockaddr *addr, size_t addrlen);

/**
 * Receive a message from a socket - this implements both recv() and recvfrom() userspace functions.
 */
ssize_t RecvfromSocket(File *fp, void *message, size_t len, int flags, struct sockaddr *addr, size_t *addrlen);

/**
 * Implements getsockname().
 */
int SocketGetsockname(File *fp, struct sockaddr *addr, size_t *addrlenptr);

/**
 * Implements shutdown().
 */
int ShutdownSocket(File *fp, int how);

/**
 * Implements connect().
 */
int ConnectSocket(File *fp, const struct sockaddr *addr, size_t addrlen);

/**
 * Implements getpeername().
 */
int SocketGetpeername(File *fp, struct sockaddr *addr, size_t *addrlenptr);

/**
 * Specify which interface a socket is to be bound to.
 */
int SocketBindif(File *fp, const char *ifname);

/**
 * Try claiming a specific unique local address (used by bind()). Returns 0 if the claim was successful and no other
 * socket currently uses the address; in this case, the address is copied into "dest". Otherwise, -1 is returned and
 * "dest" is unmodified. This is thread-safe as long as copying the address into "dest" is enough to rename your socket.
 *
 * 'ifname' is either an interface name to bind to or an empty string.
 */
int ClaimSocketAddr(const struct sockaddr *addr, struct sockaddr *dest, const char *ifname);

/**
 * Allocate an ephemeral port, and mark it as used. The value is returned in network byte order, and can therefore be
 * assigned directly to sin_port or sin6_port. Returns 0 if no ephemeral port is available!
 */
uint16_t AllocPort();

/**
 * Free a port. The port is given in network byte order. If it is an ephemeral port, it is marked as free; otherwise,
 * nothing happens (deleting the socket that calls this function is enough to release it).
 */
void FreePort(uint16_t port);

/**
 * Get/set a socket option. This is used by the C library functions setsockopt() and getsockopt(). All Glidix socket
 * options are passed as 64-bit values. 'proto' is currently reserved and must be set to zero (SOL_SOCKET).
 * Additionally, GetSocketOption() sets errno to 0 on success. SetSocketOption() returns 0 on success, or -1 on error.
 */
int SetSocketOption(File *fp, int proto, int option, uint64_t value);
uint64_t GetSocketOption(File *fp, int proto, int option);

/**
 * Multicast configuration. 'op' is either MCAST_JOIN_GROUP or MCAST_LEAVE_GROUP.
 */
int SocketMulticast(File *fp, int op, const struct in6_addr *addr, uint32_t scope);

/**
 * Implements listen().
 */
int SocketListen(File *fp, int backlog);

/**
 * Implements accept().
 */
File* SocketAccept(File *fp, struct sockaddr *addr, size_t *addrlenptr);

/**
 * Get a socket error and clear it.
 */
int SocketGetError(File *fp);

/**
 * Remove a socket from the list and then free it.
 */
void FreeSocket(Socket *sock);

#endif
