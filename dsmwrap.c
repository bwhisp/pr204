#include "common_impl.h"
#include <sys/socket.h>

int main(int argc, char **argv) {

	int sock, l_sock;
	int port;
	int k;
	char *buf = malloc(MAXNAME);
	char ** newargv;
	struct sockaddr_in * addr = malloc(sizeof(struct sockaddr_in));

	/* processus intermediaire pour "nettoyer" */
	/* la liste des arguments qu'on va passer */
	/* a la commande a executer vraiment */
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	inet_aton(argv[1], &(addr->sin_addr));
	addr->sin_port = (in_port_t) htons(atoi(argv[2]));

	/* creation d'une socket pour se connecter au */
	/* au lanceur et envoyer/recevoir les infos */
	/* necessaires pour la phase dsm_init */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	while (connect(sock, (struct sockaddr*) addr, sizeof(*addr)) < 0
			&& errno == ECONNREFUSED)
		;

	/* Envoi du nom de machine au lanceur */
	gethostname(buf, MAXNAME);
	do_write(sock, buf);

	/* Envoi de la taille du message d'abord */
	/* Puis du message lui même */
	/* Envoi du pid au lanceur */
	sprintf(buf, "%d", getpid()); // On récupère le PID du processus distant
	do_write(sock, buf);

	/* Creation de la socket d'ecoute pour les */
	/* connexions avec les autres processus dsm */
	l_sock = creer_socket(SOCK_STREAM, &port);

	/* Envoi du numero de port au lanceur */
	/* pour qu'il le propage à tous les autres */
	/* processus dsm */
	sprintf(buf, "%d", port); // On récupère le port de la socket d'ecoute
	do_write(sock, buf);

	/* on execute la bonne commande */
	/* Creation du tableau d'arguments pour le ssh */
	newargv = malloc((argc - 1) * MAXNAME);

	newargv[0] = argv[2];

	for (k = 1; k < argc - 2; k++)
		newargv[k] = argv[k + 2];

	newargv[argc - 2] = NULL;

	/* jump to new prog : */
	puts("Execution du prog");
	execvp(argv[2], newargv);

	return 0;
}
