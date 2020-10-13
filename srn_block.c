#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>


#define PATH_BLOCKED    "./blocked/"
#define MAX_ATTEMPT 	10

extern FILE *fp_log;
extern char     gmdate[33];  


int isblocked(char *usraddr);
void addblocked(char *usraddr, char *ua);

void
addblocked(char *usraddr, char *ua)
{
	FILE	       *fp_blocked;
	char		path_blocked[NI_MAXHOST + sizeof(PATH_BLOCKED)];
	char		buffer[NI_MAXHOST + 1024 + 33 + 20];

	memset(path_blocked, 0, NI_MAXHOST + sizeof(PATH_BLOCKED));
	strcpy(path_blocked, PATH_BLOCKED);
	strcat(path_blocked, usraddr);

	fp_blocked = fopen(path_blocked, "a");
	if (fp_blocked == NULL) {
		fprintf(stderr, "error: %s (%s)", strerror(errno), path_blocked);
		return;
	} else {
		memset(buffer, 0, NI_MAXHOST + 1024 + 33 + 20);
		sprintf(buffer, "%s:%s:wrong_pin\n", gmdate, ua);

		fwrite(buffer, strlen(buffer), 1, fp_blocked);

		fclose(fp_blocked);
	}

	return;
}


int
isblocked(char *usraddr)
{
	FILE	       *fp_blocked;
	char		path_blocked[NI_MAXHOST + sizeof(PATH_BLOCKED)];
	char		buffer[NI_MAXHOST + 286];
	int8_t		cnt;

	memset(path_blocked, 0, NI_MAXHOST + sizeof(PATH_BLOCKED));
	strcpy(path_blocked, PATH_BLOCKED);
	strcat(path_blocked, usraddr);

	fp_blocked = fopen(path_blocked, "r");
	if (fp_blocked == NULL)
		return 0;

	for (cnt = 0; !feof(fp_blocked) && cnt < MAX_ATTEMPT; cnt++)
		fgets(buffer, NI_MAXHOST + 286, fp_blocked);

	fclose(fp_blocked);

	if (cnt == MAX_ATTEMPT) {
		printf("Attempts over...\n");
		fprintf(fp_log, "attempts_over:");
		return 1;
	}
	return 0;
}
