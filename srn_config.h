#include <stdio.h>
#include <stdint.h>

#define HTML_DIRECTORY_ALL     "./"
#define HTML_DIRECTORY_DESKTOP "./html/"
#define HTML_DIRECTORY_MOBILE  "./html_mobile/"

struct static_entry {
	uint64_t allow_method;
	char *resource;
	char *type;
	char *type_opt;
};

extern struct static_entry static_entries[];
extern struct static_entry dyn_entries[];
