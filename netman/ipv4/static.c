/*
	Glidix Network Manager

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

#include <sys/glidix.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <gxnetman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define	SPACES				" \t"

typedef struct
{
	struct in_addr			addr;
	struct in_addr			mask;
	int				domain;
} IPNetIfAddr4;

typedef struct
{
	IPNetIfAddr4*			addrs;
	size_t				addrcount;
	_glidix_gen_route*		routes;
	size_t				routecount;
	struct in_addr*			dns;
	size_t				dnscount;
} StaticConfig;

int netman_static_init(NetmanIfConfig *config)
{
	StaticConfig *statconf = (StaticConfig*) malloc(sizeof(StaticConfig));
	memset(statconf, 0, sizeof(StaticConfig));
	
	config->data = statconf;
	return 0;
};

static int getDomainByName(const char *name)
{
	if (strcmp(name, "global") == 0)
	{
		return _GLIDIX_DOM_GLOBAL;
	}
	else if (strcmp(name, "link-local") == 0)
	{
		return _GLIDIX_DOM_LINK;
	}
	else if (strcmp(name, "loopback") == 0)
	{
		return _GLIDIX_DOM_LOOPBACK;
	}
	else if (strcmp(name, "site-local") == 0)
	{
		return _GLIDIX_DOM_SITE;
	}
	else if (strcmp(name, "multicast") == 0)
	{
		return _GLIDIX_DOM_MULTICAST;
	}
	else if (strcmp(name, "non-default") == 0)
	{
		return _GLIDIX_DOM_NODEFAULT;
	}
	else
	{
		int domain;
		if (sscanf(name, "%d", &domain) == 1)
		{
			return domain;
		}
		else
		{
			return -1;
		};
	};
};

static void writeMask(uint8_t *put, unsigned int netsize)
{
	memset(put, 0, 4);
	while (netsize >= 8)
	{
		*put++ = 0xFF;
		netsize -= 8;
	};
	
	if (netsize != 0)
	{
		unsigned int numZeroes = 8 - netsize;
		uint8_t invmask = (1 << numZeroes) - 1;
		*put = ~invmask;
	};
};

void netman_static_line(NetmanIfConfig *config, int lineno, char *line)
{
	char *cmd = strtok(line, SPACES);
	if (cmd == NULL)
	{
		return;
	};
	
	if (strcmp(cmd, "address") == 0)
	{
		IPNetIfAddr4 addr;
		char *cidr = strtok(NULL, SPACES);
		
		if (cidr == NULL)
		{
			fprintf(stderr, "static: if.conf line %d error: expected address in CIDR notation, not EOL\n", lineno);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
		
		uint8_t *addrput = (uint8_t*) &addr.addr;
		unsigned int netsize;
		
		if (sscanf(cidr, "%hhu.%hhu.%hhu.%hhu/%u", &addrput[0], &addrput[1], &addrput[2], &addrput[3], &netsize) != 5)
		{
			fprintf(stderr, "static: if.conf line %d error: invalid CIDR address: %s\n", lineno, cidr);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
		
		if (netsize > 32)
		{
			fprintf(stderr, "static: if.conf line %d error: maximum network size is 32, got %u\n", lineno, netsize);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
		
		writeMask((uint8_t*)&addr.mask, netsize);
		addr.domain = _GLIDIX_DOM_GLOBAL;
		
		char *domname = strtok(NULL, SPACES);
		if (domname != NULL)
		{
			int domain = getDomainByName(domname);
			if (domain == -1)
			{
				fprintf(stderr, "static: if.conf line %d: invalid address domain '%s'\n", lineno, domname);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
			
			addr.domain = domain;
		};
		
		StaticConfig *statconf = (StaticConfig*) config->data;
		statconf->addrs = realloc(statconf->addrs, sizeof(IPNetIfAddr4)*(statconf->addrcount+1));
		memcpy(&statconf->addrs[statconf->addrcount], &addr, sizeof(IPNetIfAddr4));
		statconf->addrcount++;
		
		statconf->routes = realloc(statconf->routes, sizeof(_glidix_gen_route)*(statconf->routecount+1));
		memset(&statconf->routes[statconf->routecount], 0, sizeof(_glidix_gen_route));
		_glidix_in_route *inroute = (_glidix_in_route*) &statconf->routes[statconf->routecount];
		statconf->routecount++;
		
		strcpy(inroute->ifname, config->ifname);
		uint32_t prefix = (*((uint32_t*)&addr.addr)) & (*((uint32_t*)&addr.mask));
		memcpy(&inroute->dest, &prefix, 4);
		memcpy(&inroute->mask, &addr.mask, 4);
		memset(&inroute->gateway, 0, 4);
		inroute->domain = addr.domain;
		inroute->flags = 0;
	}
	else if (strcmp(cmd, "route") == 0)
	{
		StaticConfig *statconf = (StaticConfig*) config->data;
		
		statconf->routes = realloc(statconf->routes, sizeof(_glidix_gen_route)*(statconf->routecount+1));
		memset(&statconf->routes[statconf->routecount], 0, sizeof(_glidix_gen_route));
		_glidix_in_route *inroute = (_glidix_in_route*) &statconf->routes[statconf->routecount];
		statconf->routecount++;
		
		strcpy(inroute->ifname, config->ifname);
		memset(&inroute->gateway, 0, 4);
		inroute->domain = _GLIDIX_DOM_GLOBAL;
		inroute->flags = 0;
		
		uint8_t *put = (uint8_t*) &inroute->dest;
		unsigned int netsize;
		
		char *cidr = strtok(NULL, SPACES);
		if (cidr == NULL)
		{
			fprintf(stderr, "static: if.conf line %d error: expected CIDR route destination, not EOL\n", lineno);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
		
		if (strcmp(cidr, "default") == 0)
		{
			memset(&inroute->dest, 0, 4);
			memset(&inroute->mask, 0, 4);
		}
		else
		{
			if (sscanf(cidr, "%hhu.%hhu.%hhu.%hhu/%u", &put[0], &put[1], &put[2], &put[3], &netsize) != 5)
			{
				fprintf(stderr, "static: if.conf line %d error: invalid destination '%s'\n", lineno, cidr);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
		
			if (netsize > 32)
			{
				fprintf(stderr, "static: if.conf line %d error: maximum prefix size is 32, got %u\n",
					lineno, netsize);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
		
			writeMask((uint8_t*)&inroute->mask, netsize);
		};
		
		char *nextKeyword = strtok(NULL, SPACES);
		if (nextKeyword != NULL)
		{
			if (strcmp(nextKeyword, "via") == 0)
			{
				char *gateway = strtok(NULL, SPACES);
				if (gateway == NULL)
				{
					fprintf(stderr, "static: if.conf line %d error: expected gateway IP after 'via'"
							" keyword\n", lineno);
					config->status = NETMAN_STATUS_ERROR;
					return;
				};
				
				if (!inet_pton(AF_INET, gateway, &inroute->gateway))
				{
					fprintf(stderr, "static: if.conf line %d error: invalid gateway address '%s'\n",
							lineno, gateway);
					config->status = NETMAN_STATUS_ERROR;
					return;
				};
				
				nextKeyword = strtok(NULL, SPACES);
			};
		};
		
		if (nextKeyword != NULL)
		{
			if (strcmp(nextKeyword, "domain") != 0)
			{
				fprintf(stderr, "static: if.conf line %d error: expected 'via' or 'domain', not '%s'\n",
						lineno, nextKeyword);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
			
			char *domname = strtok(NULL, SPACES);
			if (domname == NULL)
			{
				fprintf(stderr, "static: if.conf line %d error: expected domain name, not EOL\n", lineno);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
			
			int domain = getDomainByName(domname);
			if (domain == -1)
			{
				fprintf(stderr, "static: if.conf line %d error: invalid route domain '%s'\n", lineno, domname);
				config->status = NETMAN_STATUS_ERROR;
				return;
			};
			
			inroute->domain = domain;
		};
	}
	else if (strcmp(cmd, "nameserver") == 0)
	{
		StaticConfig *statconf = (StaticConfig*) config->data;
		
		char *nameserver = strtok(NULL, SPACES);
		if (nameserver == NULL)
		{
			fprintf(stderr, "static: if.conf line %d error: expected name server\n", lineno);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
		
		statconf->dns = realloc(statconf->dns, 4*(++statconf->dnscount));
		if (!inet_pton(AF_INET, nameserver, &statconf->dns[statconf->dnscount-1]))
		{
			fprintf(stderr, "static: if.conf line %d error: invalid name server address '%s'\n", lineno, nameserver);
			config->status = NETMAN_STATUS_ERROR;
			return;
		};
	}
	else
	{
		fprintf(stderr, "static: if.conf line %d error: unknown command '%s'\n", lineno, cmd);
		config->status = NETMAN_STATUS_ERROR;
		return;
	};
};

void netman_static_ifup(NetmanIfConfig *config)
{
	printf("[static] attempting configuration of %s\n", config->ifname);
	StaticConfig *statconf = (StaticConfig*) config->data;
	
	// write the DNS servers
	char dnspath[256];
	sprintf(dnspath, "/run/dns/ipv4/%s.if", config->ifname);
	
	FILE *fp = fopen(dnspath, "w");
	if (fp == NULL)
	{
		fprintf(stderr, "[static] WARNING: failed to open %s: cannot configure DNS servers!\n", dnspath);
	}
	else
	{
		size_t i;
		fprintf(fp, "# DNS servers from static configuration\n");
		
		for (i=0; i<statconf->dnscount; i++)
		{
			char buffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &statconf->dns[i], buffer, INET_ADDRSTRLEN);
			fprintf(fp, "%s\n", buffer);
		};
		
		fclose(fp);
	};
	
	// remove all routes
	if (_glidix_route_clear(AF_INET, config->ifname) != 0)
	{
		fprintf(stderr, "[static] cannot clear IPv4 routes of %s: %s\n", config->ifname, strerror(errno));
		return;
	};
	
	// assign addresses
	if (_glidix_netconf_addr(AF_INET, config->ifname, statconf->addrs, sizeof(IPNetIfAddr4)*statconf->addrcount) != 0)
	{
		fprintf(stderr, "[static] failed to configure %s: %s\n", config->ifname, strerror(errno));
		return;
	};
	
	// add all routes
	int i;
	for (i=0; i<statconf->routecount; i++)
	{
		if (_glidix_route_add(AF_INET, -1, &statconf->routes[i]))
		{
			fprintf(stderr, "[static] failed to add route to %s: %s\n", config->ifname, strerror(errno));
			_glidix_netconf_addr(AF_INET, config->ifname, NULL, 0);
			_glidix_route_clear(AF_INET, config->ifname);
			unlink(dnspath);
			return;
		};
	};
	
	// wait until we're put down
	free(statconf);
	while (netman_running()) pause();

	printf("[static] removing configuration\n");	
	_glidix_netconf_addr(AF_INET, config->ifname, NULL, 0);
	_glidix_route_clear(AF_INET, config->ifname);
	unlink(dnspath);
};
