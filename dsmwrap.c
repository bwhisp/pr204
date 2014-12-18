#include "common_impl.h"
#include <sys/socket.h>

int main(int argc, char **argv) {

	int sock, l_sock;
	int port;
	int k;
	int nb_proc, my_rank;
	int nb_arg;
	char *buf = malloc(MAXNAME);
	char ** newargv;
	struct sockaddr_in * addr = malloc(sizeof(struct sockaddr_in));
//	dsm_proc_conn_t * machines;

	/* processus intermediaire pour "nettoyer" */
	/* la liste des arguments qu'on va passer */
	/* a la commande a executer vraiment */
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	inet_aton(get_ip(argv[1]), &(addr->sin_addr));
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
	sprintf(buf, "%d", port);
	do_write(sock, buf);

	/* Ici nous allons recuperer le nombre de processus total et le rang du processus */
	/* envoyés précedemment par dsmexec et stocker tout ça dans des variables*/
//	memset(buf, 0, MAXNAME);
//	do_read(sock, buf);
//	nb_proc = atoi(buf);

//	memset(buf, 0, MAXNAME);
//	do_read(sock, buf);
//	my_rank = atoi(buf);

	/*	Récupération des infos de connexion pour chaque processus et stockage dans un tableau	*/
//	machines = malloc(nb_proc * sizeof(dsm_proc_conn_t));
	/*
	 for (k = 0; k < nb_proc; k++) {
	 // Nom de la machine
	 memset(buf, 0, MAXNAME);
	 do_read(sock, buf);
	 sprintf(machines[k].machine, "%s", buf);
	 // Son port
	 memset(buf, 0, MAXNAME);
	 do_read(sock, buf);
	 machines[k].port = atoi(buf);

	 // Attribution des ranks (ils correspondent à l'ordre dans le tableau par un heureux hasard)
	 machines[k].rank = k;
	 }
	 */


	newargv = malloc(argc - 3);

	newargv[0] = argv[3];

	for (k = 1; k < argc - 3; k++)
		newargv[k] = argv[k + 3];

	newargv[argc - 3] = NULL;

	/* jump to new prog : */
	sleep(1);
	if (execvp(argv[3], newargv) == -1)
		perror("Deuxieme exec");

	return 0;
}
