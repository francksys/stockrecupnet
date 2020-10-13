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

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>

#include "srn_config.h"
#include "srn_http.h"
#include "srn_date.h"
#include "srn_response.h"


extern jmp_buf	env;
extern char	gmdate[33];
/*
 * Check user header value for User-Agent with two scenario that respectively
 * return -1, 1: i) user User-Agent is a mobile User-Agent ii) user
 * User-Agent is a desktop User-Agent
 */

static int
device_mobile(char *user_agent)
{
	if (!strnstr(user_agent, " Mobile", HEADER_VALUE_SIZE))
		return 0;

	return 1;
}

/*
 * Create a Ok http header status line
 */

void
create_status_line(struct status_line *sline)
{
	sline->code = "200";
	sline->mess = "Ok";
	sline->vers = "HTTP/1.1";
	return;
}

/* Create an Expires value. (This value is always now + one year) */

void
set_expires(char value[33])
{
	struct tm      *tmv;
	time_t		tloc;

	time(&tloc);
	tmv = gmtime(&tloc);

	memset(value, 0, 33);

	++(tmv->tm_year);
	strftime(value, 33, "%a, %d %b %Y %H:%M:%S GMT", tmv);
	--(tmv->tm_year);
	return;
}

/* Create server http header name/value pairs */

void
create_header(struct hdr_nv hdrnv[MAX_HEADERS],
	      uint64_t allow,
	      char *type,
	      char *cookie)
{
	struct hdr_nv  *pnv = hdrnv;

	pnv->pname = HTTP_HEADER_CACHE_CONTROL;
	(pnv++)->pvalue = HTTP_HEADER_CACHE_CONTROL_VALUE;

	pnv->pname = HTTP_HEADER_CONNECTION;
	(pnv++)->pvalue = HTTP_HEADER_CONNECTION_VALUE;

	pnv->pname = HTTP_HEADER_DATE;

	(pnv++)->pvalue = gmdate;

	pnv->pname = HTTP_HEADER_SERVER;
	(pnv++)->pvalue = HTTP_HEADER_SERVER_VALUE;

	if (strcmp(type, "text/html") == 0) {
		pnv->pname = HTTP_HEADER_VARY;
		(pnv++)->pvalue = HTTP_HEADER_VARY_VALUE;
	}
	if (allow > 0) {
		pnv->pname = HTTP_HEADER_ALLOW;

		if (allow & HTTP_ALLOW_GET) {
			strcat(pnv->value, "Get");
		}
		if (allow & HTTP_ALLOW_HEAD) {
			if (pnv->value[0] != '\0')
				strcat(pnv->value, ", ");
			strcat(pnv->value, "Head");
		}
		if (allow & HTTP_ALLOW_POST) {
			if (pnv->value[0] != '\0')
				strcat(pnv->value, ", ");
			strcat(pnv->value, "Post");
		}
		pnv++;
	}
	(pnv)->pname = HTTP_HEADER_CONTENT_LANGUAGE;
	(pnv++)->pvalue = "fr";

	(pnv++)->pname = HTTP_HEADER_CONTENT_LENGTH;

	pnv->pname = HTTP_HEADER_CONTENT_TYPE;
	(pnv++)->pvalue = type;

	pnv->pname = HTTP_HEADER_EXPIRES;
	set_expires((pnv++)->value);

	if (cookie != NULL && *cookie != '\0') {
		pnv->pname = HTTP_HEADER_SET_COOKIE;
		strcpy((pnv++)->value, cookie);
	}
	return;
}

/* Create full server header status line and header name/value pairs */

int
create_response(struct response *resp, struct hdr_nv hdrnv[MAX_HEADERS])
{
	char	       *user_agent_val = NULL;
	char	       *device_directory = NULL;
	int		r = 0;


	/* User-Agent not specified break visit */

	if ((user_agent_val = hdr_nv_value(hdrnv, "User-Agent")) == NULL ||
	    *user_agent_val == '\0')
		longjmp(env, 1);

	/*
	 * Requested content isn't of type text/html skip device relative
	 * routine
	 */

	if (strcmp(resp->entry.type, "text/html") != 0)
		goto no_texthtml;

	r = device_mobile(user_agent_val);
	switch (r) {
	case 0:
		device_directory = "./html/";
		break;
	case 1:
		device_directory = "./html_mobile/";
		break;
	default:
		/* NOT REACHED */

		longjmp(env, 1);
	}

	sprintf(resp->local_entry, "%s%s", device_directory, resp->entry.resource);

no_texthtml:

	create_status_line(&resp->statusline);
	create_header(resp->hdrnv, resp->entry.allow_method, resp->entry.type, NULL);

	return 1;
}

/*
 * Create full server header status line and header name/value pairs. This
 * fonction does concern only dynamic resource.
 */

char	       *
create_dynamic_response(struct response *resp,
			struct hdr_nv hdrnv[MAX_HEADERS],
			char *filename,
			...)
{
	char	       *user_agent_val = NULL;
	char	       *device_directory = NULL;
	int		r = 0;	/* User-Agent not specified break visit */
	va_list		ap;
	int		fd;
	char	       *content;
	char	       *msg;

	if ((user_agent_val = hdr_nv_value(hdrnv, "User-Agent")) == NULL || *user_agent_val == '\0')
		longjmp(env, 1);

	/*
	 * Requested content isn't of type text/html skip device relative
	 * routine
	 */

	if (strcmp(resp->entry.type, "text/html") != 0)
		goto no_texthtml;

	r = device_mobile(user_agent_val);
	switch (r) {
	case 0:
		device_directory = "./html/";
		break;
	case 1:
		device_directory = "./html_mobile/";
		break;
	default:
		/* NOT REACHED */
		longjmp(env, 1);
	}

	sprintf(resp->local_entry, "%s%s", device_directory, resp->entry.resource);
no_texthtml:
	create_status_line(&resp->statusline);

	fd = open(resp->local_entry, O_RDONLY);
	if (fd < 0) {
		perror("open");
		longjmp(env, 1);
	}
	content = malloc(8192);
	memset(content, 0, 8192);

	read(fd, content, 8192);

	close(fd);

	va_start(ap, filename);
	vasprintf(&msg, content, ap);
	va_end(ap);

	free(content);

	create_status_line(&resp->statusline);
	create_header(resp->hdrnv, resp->entry.allow_method,
		      resp->entry.type,
		      NULL);
	return msg;
}
