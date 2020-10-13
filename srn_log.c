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

#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>


#include "srn_date.h"
#include "srn_log.h"


extern char	gmdate[33];
FILE     *fp_log;


/*
 * Log filename is the inet addr concatenated with `.log', try to open or
 * create it, if an error occur exit. The directory holding the log files is
 * PATH_LOG.
 */

int
open_log(const char *ipstr)
{
	char		logpath[sizeof(PATH_LOG) - 1 + INET_ADDRSTRLEN - 1 + sizeof(".log")];

	if (access(PATH_LOG, X_OK | W_OK) < 0)
		if (mkdir(PATH_LOG, 0700) < 0)
			errx(1,
			     "Can't create the log directory with write and execute perms");

	memset(logpath, 0, sizeof(logpath));
	sprintf(logpath, "%s%s.log", PATH_LOG, ipstr);

	fp_log = fopen(logpath, "a");
	if (fp_log == NULL)
		errx(1, "Unable to open or create log file %s (%s) exiting ..",
		     logpath,
		     strerror(errno));

	set_gmdate();

	fprintf(fp_log, "\n\nLog file %s opened at %s\n",
		logpath,
		gmdate);



	return 1;
}
