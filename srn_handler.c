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
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>


#include "srn_config.h"
#include "srn_http.h"
#include "srn_response.h"
#include "srn_send.h"
#include "srn_pin.h"
#include "srn_socket.h"
#include "srn_down.h"
#include "srn_dn.h"
#include "srn_block.h"
#include "srn_handler.h"


extern jmp_buf	env;
extern struct static_entry dyn_entries[];
extern FILE    *fp_log;
extern char	gmdate[33];

char		guest_download[254 + sizeof("/store/")];
char		guest_download_ok[254 + sizeof("/store/")];

/*
 * This code should be commented.
 */

int
srn_handle(int suser, struct request_line *rline,
	   struct hdr_nv hdrnv[MAX_HEADERS],
	   char *usraddr)
{
	int8_t		delete = 0;
	char		buffdel[sizeof("&supprimer=0")];

	if (rline->method == HTTP_POST && strcmp(rline->entry.resource, "stockage") == 0) {
		char		filename[254];
		char		boundary[255];
		char	       *clength;
		char	       *ctype;
		off_t		off_clength = 0;
		int		r;

		clength = hdr_nv_value(hdrnv, "Content-Length");
		if (clength == NULL || *clength == '\0')
			longjmp(env, 1);

		if (http_parse_length(clength, &off_clength) < 0)
			longjmp(env, 1);

		ctype = hdr_nv_value(hdrnv, "Content-Type");
		if (ctype == NULL || *ctype == '\0')
			longjmp(env, 1);

		if (rec_boundary(boundary, ctype) < 0)
			longjmp(env, 1);

		memset(filename, 0, 254);
		r = get_form_data(suser, filename, off_clength, boundary);
		if (r == -1) {
			struct response	resp;

			memset(&resp, 0, sizeof(struct response));
			memcpy(&resp.entry, &dyn_entries[4],
			       sizeof(struct static_entry));
			create_response(&resp, hdrnv);
			static_send_reply(suser, &resp);
		} else if (r == 0) {
			struct response	resp;

			memset(&resp, 0, sizeof(struct response));
			memcpy(&resp.entry, &dyn_entries[1],
			       sizeof(struct static_entry));
			create_response(&resp, hdrnv);
			static_send_reply(suser, &resp);
		} else if (r == 1) {
#define PIN_SIZE	6
			struct response	resp;
			char	       *pcontent;
			char	       *clen;
			char		buffer[8192];
			unsigned long	bufferlen = 0;
			unsigned char	pin[PIN_SIZE + 1];
			char		filenamedup[254];

			memset(&resp, 0, sizeof(struct response));
			memcpy(&resp.entry, &dyn_entries[0],
			       sizeof(struct static_entry));
			memset(filenamedup, 0, 254);

			gen_pin(pin, filenamedup);

			strcpy(filenamedup, filename);
			pcontent = create_dynamic_response(&resp, hdrnv,
							   filename,
							   filenamedup,
							   pin);

			memset(buffer, 0, 8192);
			memcpy(buffer, pcontent, 8192);
			free(pcontent);

			clen = hdr_nv_value(resp.hdrnv, "Content-Length");

			bufferlen = strlen(buffer);
			sprintf(clen, "%lu", bufferlen);

			send_status_line(suser, &resp.statusline);
			static_send_header(suser, resp.hdrnv);

			if (send_data(suser, buffer, bufferlen) < 0)
				longjmp(env, 1);

			rec_pin(pin, filename);
		}
	} else if (rline->method == HTTP_POST && strcmp(rline->entry.resource, "recuperation") == 0) {
		char	       *clength = NULL;
		unsigned char	pin[PIN_SIZE + 1];
		char		filename[254];

		clength = hdr_nv_value(hdrnv, "Content-Length");
		if (clength == NULL || *clength == '\0')
			longjmp(env, 1);

		if (strcmp(clength, "10") == 0 || strcmp(clength, "22") == 0) {
			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, (char*)pin, 4) != 4)
				longjmp(env, 1);

			if (strcmp((const char*)pin, "pin=") != 0)
				longjmp(env, 1);

			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, (char*)pin, PIN_SIZE) != PIN_SIZE)
				longjmp(env, 1);

		} else {
			goto clen_ne_pinsize;
		}

		if (strcmp(clength, "22") == 0) {
			memset(buffdel, 0, sizeof("&supprimer=0"));
			if (recv_data(suser, buffdel, sizeof("&supprimer=0") - 1) !=
			    sizeof("&supprimer=0") - 1)
				longjmp(env, 1);

			if (strcmp(buffdel, "&supprimer=0") == 0)
				delete = 0;
			else if (strcmp(buffdel, "&supprimer=1") == 0)
				delete = 1;
			else
				longjmp(env, 1);
		}

		if (isblocked(usraddr))
			longjmp(env, 1);

		if (!pin_exist(pin, filename)) {
			struct response	resp;
			char *pua;
clen_ne_pinsize:
			memset(&resp, 0, sizeof(struct response));
			memcpy(&resp.entry, &dyn_entries[2], sizeof(struct static_entry));
			create_response(&resp, hdrnv);
			static_send_reply(suser, &resp);


			pua = hdr_nv_value(hdrnv, "User-Agent");
			if (pua == NULL || *pua == '\0')
				longjmp(env, 1);

			addblocked(usraddr, pua);
			longjmp(env, 1);
		} else {
			struct response	resp;
			char	       *gdown;

			create_guest_down(&resp, filename);
			gdown = hdr_nv_value(resp.hdrnv, "Location");

			send_status_line(suser, &resp.statusline);
			static_send_header(suser, resp.hdrnv);

			memset(guest_download_ok, 0, 254 + sizeof("/store/"));
			gdown += sizeof("http://") - 1;
			gdown += sizeof(DOMAIN_NAME);
			strlcpy(guest_download_ok, gdown, 254 + sizeof("/store/"));
		}
	} else if ((rline->method == HTTP_GET || rline->method == HTTP_POST) &&
		   strcmp(rline->entry.resource, "store/") == 0) {
		struct response	resp;
		char	       *phdr_clen = NULL;
		struct stat	sb;
		int		fd;
		char		buffer[1024];
		char		buffer2[1024];
		ssize_t		readb;
		FILE	       *fp;
		char		pin[PIN_SIZE + 1];
		char	       *clength = NULL;
		long		fposmatch = 0;
		struct hdr_nv  *phnv;

		if (strcmp(guest_download_ok, guest_download) != 0)
			longjmp(env, 1);

		clength = hdr_nv_value(hdrnv, "Content-Length");
		if (clength == NULL || *clength == '\0')
			longjmp(env, 1);

		if (strcmp(clength, "10") == 0 || strcmp(clength, "22") == 0) {
			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, pin, 4) != 4)
				longjmp(env, 1);

			if (strcmp(pin, "pin=") != 0)
				longjmp(env, 1);

			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, pin, PIN_SIZE) != PIN_SIZE)
				longjmp(env, 1);
		} else {
			longjmp(env, 1);
		}
	
		if (strcmp(clength, "22") == 0) {
			memset(buffdel, 0, sizeof("&supprimer=0"));
			if (recv_data(suser, buffdel, sizeof("&supprimer=0") - 1) !=
			    sizeof("&supprimer=0") - 1)
				longjmp(env, 1);

			if (strcmp(buffdel, "&supprimer=0") == 0)
				delete = 0;
			else if (strcmp(buffdel, "&supprimer=1") == 0)
				delete = 1;
			else
				longjmp(env, 1);

		}

		sprintf(buffer, "%s:%s\n", pin, &guest_download[6]);

		if (isblocked(usraddr))
			longjmp(env, 1);

		if ((fp = fopen(SRN_DB, "r")) == NULL) {
			perror("fopen");
			errx(1, "Unable to open %s (%s)", SRN_DB, strerror(errno));
		}
		do {
			memset(buffer2, 0, 1024);
			fgets(buffer2, 1024, fp);
			if (strcmp(buffer, buffer2) == 0) {
				fposmatch = ftell(fp) - (long)strlen(buffer2);
				goto match;
			}
		} while (!feof(fp));

		/* printf("1:%s\n2:%s\n", buffer, buffer2); */

		if (feof(fp)) {
			char *pua;

			memset(guest_download, 0, 254 + sizeof("/store/"));
			memset(guest_download_ok, 0, 254 + sizeof("/store/"));
			memset(guest_download_ok, 2, 253 + sizeof("/store/"));

			fclose(fp);
			pua = hdr_nv_value(hdrnv, "User-Agent");
			if (pua == NULL || *pua == '\0')
				longjmp(env, 1);

			addblocked(usraddr, pua);
			longjmp(env, 1);
		}
match:

		fclose(fp);

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line(&resp.statusline);
		create_header(resp.hdrnv, HTTP_ALLOW_POST,
			      "application/octet-stream",
			      NULL);

		send_status_line(suser, &resp.statusline);

		memset(&sb, 0, sizeof(struct stat));
		stat(guest_download_ok, &sb);

		phdr_clen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(phdr_clen, "%lu", sb.st_size);

		for (phnv = (struct hdr_nv *)&resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Content-Disposition";
		strcpy(phnv->value, "attachment");

		static_send_header(suser, resp.hdrnv);

		memset(guest_download, 0, 254 + sizeof("/store/"));

		fprintf(fp_log, "%s:", guest_download_ok);

		fd = open(guest_download_ok, O_RDONLY);
		if (fd < 0) {
			warn("Unable to open %s (%s)", guest_download_ok, strerror(errno));
			longjmp(env, 1);
		}
		do {
			readb = read(fd, buffer, 1024);
			if (readb <= 0)
				break;
		} while (send_data(suser, buffer, (size_t)readb) > 0);

		close(fd);

		if (delete == 1) {
			rem_pin(fposmatch, (unsigned short)strlen(buffer2));
			remove(guest_download_ok);
			delete = 0;
		}
		memset(guest_download_ok, 0, 254 + sizeof("/store/"));
		memset(guest_download_ok, 2, 253 + sizeof("/store/"));
	} else if (rline->method == HTTP_GET &&
		   strcmp(rline->entry.resource, "RGPD_Ok") == 0) {
		struct response	resp;
		char		cookie[254];
		char		expires[33];
		struct hdr_nv  *phnv;
		char	       *preferer;
		char	       *pclen;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line_tempred(&resp.statusline);

		memset(cookie, 0, 254);

		strcpy(cookie, "RGPD=1; Domain=.stockrecup.net; ");
		strcat(cookie, "Expires=");

		memset(expires, 0, 33);
		set_expires(expires);
		strcat(cookie, expires);

		create_header(resp.hdrnv, 0, "text/html", cookie);

		for (phnv = resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Location";

		if ((preferer = hdr_nv_value(hdrnv, "Referer")) != NULL && *preferer != '\0')
			strlcpy(phnv->value, preferer, HEADER_VALUE_SIZE - 1);
		else
			sprintf(phnv->value, "http://%s/Accueil", DOMAIN_NAME);

		pclen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(pclen, "%lu", sizeof(DOMAIN_NAME) - 1);

		send_status_line(suser, &resp.statusline);
		static_send_header(suser, resp.hdrnv);

		send_data(suser, DOMAIN_NAME, sizeof(DOMAIN_NAME) - 1);

	} else if (rline->method == HTTP_GET &&
		   strcmp(rline->entry.resource, "RGPD_Refus") == 0) {
		struct response	resp;
		char		cookie[254];
		char		expires[33];
		struct hdr_nv  *phnv;
		char	       *preferer;
		char	       *pclen;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line_tempred(&resp.statusline);

		memset(cookie, 0, 254);

		strcpy(cookie, "RGPD=0; Domain=.stockrecup.net; ");
		strcat(cookie, "Expires=");

		memset(expires, 0, 33);
		set_expires(expires);
		strcat(cookie, expires);

		create_header(resp.hdrnv, 0, "text/html", cookie);

		for (phnv = resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Location";

		if ((preferer = hdr_nv_value(hdrnv, "Referer")) != NULL &&
		    *preferer != '\0' &&
		  strnstr(preferer, DOMAIN_NAME, HEADER_VALUE_SIZE) != NULL)
			strlcpy(phnv->value, preferer, HEADER_VALUE_SIZE - 1);
		else
			sprintf(phnv->value, "http://%s/Accueil", DOMAIN_NAME);

		pclen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(pclen, "%lu", sizeof(DOMAIN_NAME) - 1);

		send_status_line(suser, &resp.statusline);
		static_send_header(suser, resp.hdrnv);

		send_data(suser, "DOMAIN_NAME", sizeof(DOMAIN_NAME) - 1);
	} else {
		struct response	resp;
		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));
		create_response(&resp, hdrnv);
		static_send_reply(suser, &resp);
	}
	return 1;
}
