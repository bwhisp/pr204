#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "common_impl.h"

int creer_socket(int type, int *port_num) {
	// Creer un struct addr
	struct sockaddr_in * addr = malloc(sizeof(*addr));
	socklen_t size = sizeof(*addr);
	*addr = init_addr(*addr);

	/* fonction de creation et d'attachement */
	/* d'une nouvelle socket */
	int fd = socket(AF_INET, type, IPPROTO_TCP);
	if (fd == -1)
		perror("Erreur socket :");

	if (bind(fd, (struct sockaddr *) addr, sizeof(*addr)) == -1)
		perror("Erreur bind :");

	if (listen(fd, BACKLOG) == -1)
		perror("Erreur listen :");
	/* renvoie le numero de descripteur */
	/* et modifie le parametre port_num */
	// Ici on va recuperer le port
	getsockname(fd, (struct sockaddr *) addr, &size);
	*port_num = addr->sin_port;
	return fd;
}

// récupération de notre adresse IP

char *get_my_ip(void) {

	char s[256];
	if (!gethostname(s, sizeof s)) {
		struct hostent *host = gethostbyname(s);
		if (host != NULL) {
			struct in_addr **adr;
			for (adr = (struct in_addr **) host->h_addr_list; *adr; adr++) {
				return inet_ntoa(**adr);
			}
		}
	}
	return NULL;
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
struct sockaddr_in init_addr(struct sockaddr_in addr) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 0; // 0: pour choisir un numero de port quelconque
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	return addr;
}

int do_read () {

	return 0;
}

int do_write () {

	return 0;
}

/* fonctions dans common_impl.h */
