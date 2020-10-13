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

#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>
#include <err.h>
#include <regex.h>
#include <fcntl.h>
#include <locale.h>

#include "srn_config.h"
#include "srn_dn.h"
#include "srn_log.h"
#include "srn_socket.h"
#include "srn_http.h"
#include "srn_response.h"
#include "srn_handler.h"
#include "srn_date.h"
#include "srn_moved.h"
#include "srn_pin.h"
#include "srn_main.h"


extern FILE    *fp_log;
extern char	gmdate[33];
extern char	gmdatelog[33];
extern char	guest_download[254 + sizeof("/store/")];
extern char	guest_download_ok[254 + sizeof("/store/")];
jmp_buf  env;

struct hostserv {
	char		hostname[NI_MAXHOST];
	char		servname[NI_MAXSERV];
	uint8_t		canonname;
};

int client_hostserv(struct hostserv *hs, struct sockaddr_in *usrinaddr);

/*
 * Print a very short explanation of this program.
 */

static _Noreturn void
usage(const char *progname)
{
	fprintf(stderr, "Usage: %s\n"
		"\tWithout arguments run the storage/retrieve daemon\n"
		"\tThe daemon will try to bind a socket to the domain\n"
		"\tpointed to by %s\n\n", progname, DOMAIN_NAME);
	exit(0);
}

int
client_hostserv(struct hostserv *hs, struct sockaddr_in *usrinaddr)
{
	char	       *phostname;
	int		ecode;

	memset(hs->hostname, 0, NI_MAXHOST);
	memset(hs->servname, 0, NI_MAXSERV);

	if ((ecode = getnameinfo((struct sockaddr *)usrinaddr,
				 usrinaddr->sin_len,
				 hs->hostname,
				 NI_MAXHOST,
				 hs->servname,
				 NI_MAXSERV,
				 NI_NUMERICSERV)) != 0) {
		fprintf(stderr, "Error in %s at %d with getnameinfo (%s).\n",
			__FILE__,
			__LINE__,
			gai_strerror(ecode));
		return -1;
	}
	phostname = hs->hostname;

	while (isdigit(*phostname) != 0 || *phostname == '.')
		if (*phostname == '\0')
			break;
		else
			++phostname;

	if (*phostname == '\0')
		hs->canonname = 0;
	else
		hs->canonname = 1;

	return 1;
}


int
main(int argc, char **argv)
{
	struct sockaddr_in servsin[7], usersin;
	char		addrstr[7][INET_ADDRSTRLEN];
	int		sserv[7];
	int		s;
	int		suser = 0;
	socklen_t	slen;
	struct request_line rline;
	struct hdr_nv	hdrnv[MAX_HEADERS];
	struct response	resp;
	struct hostserv	hs;
	char	       *phdr_host;
	regex_t		reg1, reg2;
	const struct timeval tmvrcv = {2, 266666};
	const struct timeval tmvsnd = {2, 266666};
	const unsigned short size = 1024;
	int cnt = 0;
	int usrconncnt = 0;

	/* Without arguments run the daemon otherwise print usage */
	if (argc > 1)
		usage(argv[0]);

	for (cnt = 0; cnt < 7; cnt++)
		if (create_socket(&sserv[cnt]) < 0)
			exit(1);

	memset(addrstr, 0, INET_ADDRSTRLEN * 7);
	cnt = dn2sin(DOMAIN_NAME, servsin, addrstr);

	while (--cnt)
		if (fork() == 0)
			break;

	open_log(addrstr[cnt]);

	fprintf(fp_log, "Starting StockRecupNET with following " \
		        "domain name: %s\n", DOMAIN_NAME);

	fprintf(fp_log, "This file logs network events relative to" \
			" address: %s\n", addrstr[cnt]);

	fflush(fp_log);

	createdb();

	s = bind_socket(&sserv[cnt], &servsin[cnt]);
	listen(s, 24);

	printf("Accepting connections on %s\n", addrstr[cnt]);

	set_gmdate();

	signal(SIGPIPE, SIG_IGN);

	memset(&reg1, 0, sizeof(regex_t));
	memset(&reg2, 0, sizeof(regex_t));

	/* TLD we will accept for incoming connections */

	if (regcomp(&reg1, "(.org|.net|.it|.es|.eu|.com|.fr|.arpa|.us|.de|.uk|.io|.se|.nl)$",
		    REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
		printf("regcomp() error\n");
		return -1;
	}

	/* TLD we will reject for incoming connections */

	if (regcomp(&reg2, "(ipip.net|frontiernet.net|grapeshot.co.uk|hwclouds-dns.com|"
		    "rima-tde.net|tedata.net|ahrefs.com|bezeqint.net|"
		 "googleusercontent.com|semrush.com|sogou.com|yandex.com)$",
		    REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
		printf("regcomp() error\n");
		return -1;
	}
	memset(guest_download, 0, 254 + sizeof("/store/"));
	memset(guest_download_ok, 0, 254 + sizeof("/store/"));
	memset(guest_download_ok, 2, 253 + sizeof("/store/"));

	if (setjmp(env) == 1) {
		char		c;
		while (recv(suser, &c, 1, 0) > 0);
		shutdown(suser, SHUT_RDWR);
		close(suser);
		fprintf(fp_log, "exception_error\n");
		fflush(fp_log);
	}
	for (;;) {
                if (((usrconncnt++ + 7) % 33) == 0) {
                        system("clear");
                        usrconncnt = 0;
                }

		memset(&usersin, 0, sizeof(struct sockaddr_in));
		slen = sizeof(struct sockaddr_in);

		suser = accept(s, (struct sockaddr *)&usersin, &slen);
		if (suser < 0) {
			warn("At line %d in file %s: ",
			     __LINE__,
			     __FILE__);
			shutdown(suser, SHUT_RDWR);
			close(suser);
			continue;
		}
		setsockopt(suser, SOL_SOCKET, SO_SNDTIMEO, &tmvsnd, sizeof(struct timeval));
		setsockopt(suser, SOL_SOCKET, SO_RCVTIMEO, &tmvrcv, sizeof(struct timeval));
		setsockopt(suser, SOL_SOCKET, SO_RCVBUF, &size, sizeof(unsigned short));


		memset(&hs, 0, sizeof(struct hostserv));
		client_hostserv(&hs, &usersin);

		set_gmdate();
		set_gmdatelog();

		printf("Oncoming connection from %s:%s\n", hs.hostname, hs.servname);

		fprintf(fp_log, "%s:oncoming:%s:%s:", gmdatelog,
					              hs.hostname,
						      hs.servname);

		if (hs.canonname) {
			if (regexec(&reg1, hs.hostname, 0, NULL, 0) != 0 ||
			    regexec(&reg2, hs.hostname, 0, NULL, 0) == 0) {
				fprintf(fp_log, "blocked_request\n");
				shutdown(suser, SHUT_RDWR);
				close(suser);
				continue;
			}
		}
		memset(&rline, 0, sizeof(struct request_line));
		memset(hdrnv, 0, sizeof(struct hdr_nv) * MAX_HEADERS);
		memset(&resp, 0, sizeof(struct response));

		get_http_request(suser, &rline, hdrnv);

		phdr_host = hdr_nv_value(hdrnv, "Host");

		if (phdr_host == NULL || *phdr_host == '\0')
			longjmp(env, 1);

		if (strncasecmp(phdr_host, ROOT_DOMAIN_NAME,
					   sizeof(ROOT_DOMAIN_NAME) - 1) == 0) {
			send_moved_perm(suser);
			fprintf(fp_log, "root_redirection:");
			goto redirect;
		} else if (strncasecmp(phdr_host, DOMAIN_NAME,
						  sizeof(DOMAIN_NAME) - 1) != 0) {
			longjmp(env, 1);
		}

		fprintf(fp_log, "%lu:%s:", rline.method, rline.entry.resource);

		srn_handle(suser, &rline, hdrnv, hs.hostname);

redirect:
		shutdown(suser, SHUT_RDWR);
		close(suser);

		fprintf(fp_log, "sent\n");
		fflush(fp_log);
	}

}
