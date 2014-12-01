#include "common_impl.h"
#include <sys/socket.h>

int main(int argc, char **argv) {

	int l_sock;
	int sock;
	struct sockaddr_in * addr = malloc(sizeof(struct sockaddr_in));
	char *buf = malloc(MAXNAME);
	char ** newargv;
	int k;
	int port;

	/* processus intermediaire pour "nettoyer" */
	/* la liste des arguments qu'on va passer */
	/* a la commande a executer vraiment */
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;

	/* creation d'une socket pour se connecter au */
	/* au lanceur et envoyer/recevoir les infos */
	/* necessaires pour la phase dsm_init */
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(sock, (struct sockaddr*) addr, sizeof(*addr)) < 0)
		perror("Erreur de connection");

	/* Envoi du nom de machine au lanceur */
	gethostname(buf, sizeof(buf));
	write(sock, buf, sizeof(buf));

	/* Envoi du pid au lanceur */
	sprintf(buf, "%d", getpid()); // On récupère le PID du processus distant
	write(sock, buf, sizeof(buf));

	/* Creation de la socket d'ecoute pour les */
	/* connexions avec les autres processus dsm */
	l_sock = creer_socket(SOCK_STREAM, &port);
	printf("%d\n", l_sock);

	/* Envoi du numero de port au lanceur */
	/* pour qu'il le propage à tous les autres */
	/* processus dsm */
	sprintf(buf, "%d", port); // On récupère le port de la socket d'ecoute
	write(sock, buf, sizeof(buf));

	/* on execute la bonne commande */
	/* Creation du tableau d'arguments pour le ssh */
	newargv = malloc((argc - 2) * MAXNAME);
	for (k = 0; k < (argc - 2); k++) {
		newargv[k] = malloc(MAXNAME);
	}
	newargv[0] = argv[2];

	for (k = 1; k < argc - 2; k++)
		newargv[k] = argv[k + 2];

	/* jump to new prog : */
	puts("Execution du prog");
	execvp(argv[2], newargv);

	return 0;
}
