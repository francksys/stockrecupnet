#define ROOT_DOMAIN_NAME	"stockrecup.net"
#define DOMAIN_NAME		"www.stockrecup.net"

int dn2sin(const char *domain_name, struct sockaddr_in sin[7], char addrstr[7][INET_ADDRSTRLEN]);
