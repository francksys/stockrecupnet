/*
 * Copyright (c) 2020 Franck Lesage <francksys@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <err.h>
#include <string.h>
#include <stdio.h>

#include "srn_dn.h"

void sin2str(struct sockaddr_in *sin, char str[INET_ADDRSTRLEN]);

/* Stringify a binary network format of a sin_addr. */

void
sin2str(struct sockaddr_in *sin, char str[INET_ADDRSTRLEN])
{
	memset(str, 0, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, (void *)&(sin->sin_addr.s_addr), str, INET_ADDRSTRLEN);
	return;
}

/*
 * Try to find IPv4 address of domain_name we will be listening to if found
 * stringify it.
 */

int
dn2sin(const char *domain_name, struct sockaddr_in sin[7], char addrstr[7][INET_ADDRSTRLEN])
{
	struct addrinfo	hints, *res0, *res0dup;
	int		r;
	int cnt = 0;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	r = getaddrinfo(domain_name, "www", &hints, &res0);
	if (r)
		errx(1, "Unable to get IPv4 address for %s (%s) exiting..\n",
		     domain_name, gai_strerror(r));

	res0dup = res0;

	do {
		memset(&sin[cnt], 0, sizeof(struct sockaddr_in));
		memcpy(&sin[cnt], res0->ai_addr, sizeof(struct sockaddr_in));
		sin2str(&sin[cnt], addrstr[cnt]);
		res0 = res0->ai_next;
		cnt++;
	} while(res0 != NULL && cnt < 7);

	freeaddrinfo(res0dup);

	return cnt;
}
