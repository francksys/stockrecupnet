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

#include "srn_config.h"
#include "srn_http.h"


/*
 * Static web site resources are declared here
 */

struct static_entry static_entries[] = {
	{
		HTTP_ALLOW_GET,
		"Accueil", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"RGPD_Ok", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"RGPD_Refus", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"CNIL", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"mentions-legales", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"aide", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"contact", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET | HTTP_ALLOW_POST,
		"stockage", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET | HTTP_ALLOW_POST,
		"recuperation", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"images/recup_ordi_sc.png", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/IMG_20200711_094942.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/small_IMG_20200711_094942.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/Screenshot_20200715-095316.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/mIMG_20200711_094942.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/Screenshot_20200713-173238.jpg", "image/jpeg", NULL,
	},
	{
		0,
	},
};

/*
 * Dynamic web site resource, ones which need to be filled with dynamic data
 * or which change according to user input. They are retourned only if errors
 * occur or success happen. They aren't indexable and used in a context where
 * the user use the store/retrieve service, therefore not for read only use
 * of the service.
 */

struct static_entry dyn_entries[] = {
	{
		0,
		"dyn_envoie_succes", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_fichier_existe", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_code_faux", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_code_bon", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_nom_fichier_inv", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
	},
};
