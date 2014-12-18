#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "common_impl.h"

/*	Créer une socket d'écoute sur un port choisi dynamiquement 	*/
int creer_socket(int type, int *port_num) {
	// Creer un struct addr
	struct sockaddr_in * addr = malloc(sizeof(*addr));
	socklen_t size = sizeof(*addr);
	*addr = init_addr(*addr);

	/* fonctions de creation et d'attachement */
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
	getsockname(fd, (struct sockaddr *) addr, &size);
	*port_num = addr->sin_port;

	return fd;
}

/*	Récuperer l'adresse IP de notre machine dans une chaine de caractère	*/
char *get_ip(const char * s) {

	struct hostent *host = gethostbyname(s);
	if (host != NULL) {
		struct in_addr **adr;
		for (adr = (struct in_addr **) host->h_addr_list; *adr; adr++) {
			return inet_ntoa(**adr);
		}
	}
	return NULL;
}

struct sockaddr_in init_addr(struct sockaddr_in addr) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 0; // 0: pour choisir un numero de port quelconque
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	return addr;
}

/*	Fonctions pour la lecture et l'écriture dans les sockets	*/
ssize_t do_read(int fd, char * buf) {
	ssize_t rl;
	ssize_t r;
	char *taille = malloc(sizeof(size_t));

	rl = read(fd, taille, sizeof(size_t));
	if (rl == 0) {
		return 0;
	}
	r = read(fd, buf, (size_t) atoi(taille));
	return r;
}

int do_write(int fd, void * buf) {
	char *taille = malloc(sizeof(size_t));
	size_t len = strlen(buf);

	sprintf(taille, "%d", (int) len);
	write(fd, taille, sizeof(size_t));

	write(fd, buf, len);

	return 0;
}

int do_send(int fd, void * buf) {
	char *taille = malloc(sizeof(size_t));
	size_t len = strlen(buf);

	sprintf(taille, "%d", (int) len);
	send(fd, taille, sizeof(size_t),0);

	send(fd, buf, len,0);

	return 0;
}

void redirections(int fderr[2], int fdout[2]) {
	/* fermeture des extremités inutiles */
	close(fderr[0]);
	close(fdout[0]);

	/* redirection stderr */
	close(STDERR_FILENO);
	dup(fderr[1]);
	close(fderr[1]);
	puts("[dsmexec] Redirection stderr faite");

	/* redirection stdout */
	close(STDOUT_FILENO);
	dup(fdout[1]);
	close(fdout[1]);
	puts("[dsmexec] Redirection stdout faite");
}

/* fonctions dans common_impl.h */
