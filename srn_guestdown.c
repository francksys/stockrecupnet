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
#include "srn_dn.h"


extern jmp_buf	env;
extern char	gmdate[33];

void create_guest_down(struct response *resp, char filename[254]);
void create_status_line_tempred(struct status_line *sline);

/*
 * Create a Temporary Redirect http header status line response
 */

void
create_status_line_tempred(struct status_line *sline)
{
	sline->code = "307";
	sline->mess = "Temporary Redirect";
	sline->vers = "HTTP/1.1";
	return;
}

/*
 * Create and open an access for download and guest with a redirect http
 * response the user to retrieve its file at store resource location
 */

void
create_guest_down(struct response *resp, char filename[254])
{
	struct hdr_nv  *phnv;
	char	       *clength;

	memset(resp, 0, sizeof(struct response));

	create_status_line_tempred(&resp->statusline);
	create_header(resp->hdrnv, 0, "application/octet-stream", NULL);

	clength = hdr_nv_value(resp->hdrnv, "Content-Length");
	*clength = '0';
	for (phnv = (struct hdr_nv *)&resp->hdrnv; phnv->pname != NULL; phnv++);

	phnv->pname = "Location";
	sprintf(phnv->value, "http://%s/store/%s", DOMAIN_NAME, filename);
	
	return;
}
