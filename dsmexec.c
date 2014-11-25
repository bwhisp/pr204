#include "common_impl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>

/* variables globales */
#define MAXNAME 256

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void) {
	fprintf(stdout, "Usage : dsmexec machine_file executable arg1 arg2 ...\n");
	fflush(stdout);
	exit(EXIT_FAILURE);
}

void sigchld_handler(int sig) {
	/* on traite les fils qui se terminent */
	/* pour eviter les zombies */
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage();
	} else {
		pid_t pid;
		int num_procs = 0;
		int i = 0;
		char ** machines;
		char ** newargv;
		struct pollfd * pfds;
		//int * fdouts;
		//int * fderrs;
		int port;
		int fdout[2];
		int fderr[2];
		int k, r;
		int listen_sock;
		char *buf = malloc(MAXNAME);

		FILE * machinefile = fopen(argv[1], "r"); //ouvrir le fichier machinefile
		if (machinefile == NULL) {
			perror("fopen failed : "); // vérifier la sortie de fopen
		}

		/* Mise en place d'un traitant pour recuperer les fils zombies*/
		/* XXX.sa_handler = sigchld_handler; */

		/* lecture du fichier de machines */
		/* 1- on recupere le nombre de processus a lancer */
		while (!feof(machinefile)) {
			if (fgetc(machinefile) == '\n')
				num_procs++;
		}
		printf("Nombre de machines lues : %i\n", num_procs);
		fseek(machinefile, 0, SEEK_SET); //reprendre le fichier dés le début

		machines = malloc(num_procs * MAXNAME);
		for (i = 0; i < num_procs; i++) {
			machines[i] = malloc(MAXNAME);
		}
		/* 2- on recupere les noms des machines : le nom de */
		/* la machine est un des elements d'identification */
		i = 0;
		while (!feof(machinefile)) // si on est pas à la fin du fichier
		{
			fscanf(machinefile, "%s\n", machines[i]);
			printf("machine : %s num : %d\n", machines[i], i); // on affiche les noms pour tester
			i++;
		}

		/* creation de la socket d'ecoute */
		/* + ecoute effective */
		listen_sock = creer_socket(SOCK_STREAM, &port);
		puts("Socket d'écoute initialisée");

		/* On alloue la place nécessaire au stockage des fd des tubes que l'on créera*/
		pfds = malloc(num_procs * 2 * sizeof(struct pollfd)); // 2 tubes par processus fils
//		fdouts = malloc(num_procs * sizeof(int));
//		fderrs = malloc(num_procs * sizeof(int));

		/* creation des fils */
		for (i = 0; i < num_procs; i++) {
			/* creation du tube pour rediriger stdout */
			if (pipe(fdout) == -1)
				perror("Erreur tube fdout");
			/* creation du tube pour rediriger stderr */
			if (pipe(fderr) == -1)
				perror("Erreur tube fderr");

			pid = fork();
			if (pid == -1)
				ERROR_EXIT("fork");

			if (pid == 0) { /* fils */

				/* redirection stderr */
				close(fderr[0]);
				close(STDERR_FILENO);
				dup(fderr[1]);
				close(fderr[1]);
				puts("redirection faite");

				/* redirection stdout */
				close(fdout[0]);
				close(STDOUT_FILENO);
				dup(fdout[1]);
				close(fdout[1]);

				puts("arguments ssh");
				/* Creation du tableau d'arguments pour le ssh */
				newargv = malloc(argc * MAXNAME);
				for (k = 0; k < argc; k++) {
					newargv[k] = malloc(MAXNAME);
				}
				newargv[0] = "ssh";
				newargv[1] = machines[i];
				for (k = 2; k < argc; k++)
					newargv[k] = argv[k];

				/* jump to new prog : */
				puts("Execution de ssh");
				execvp("ssh", newargv);

			} else if (pid > 0) { /* pere */
				/* fermeture des extremites des tubes non utiles */
				close(fdout[1]);
				close(fderr[1]);
				pfds[2 * i].fd = fdout[0];  // 2i
				pfds[2 * i].events = POLLIN | POLLPRI;
				pfds[2 * i + 1].fd = fderr[0]; // 2i+1
				pfds[2 * i + 1].events = POLLIN | POLLPRI;
				num_procs_creat++;

			}
		}

		for (i = 0; i < num_procs; i++) {

			/* on accepte les connexions des processus dsm */
			//accept();
			/*  On recupere le nom de la machine distante */
			/* 1- d'abord la taille de la chaine */
			/* 2- puis la chaine elle-meme */

			/* On recupere le pid du processus distant  */

			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
		}

		/* envoi du nombre de processus aux processus dsm*/

		/* envoi des rangs aux processus dsm */

		/* envoi des infos de connexion aux processus */

		/* gestion des E/S : on recupere les caracteres */
		/* sur les tubes de redirection de stdout/stderr */
		while (1) {
			/* je recupere les infos sur les tubes de redirection
			 jusqu'à ce qu'ils soient inactifs (ie fermes par les
			 processus dsm ecrivains de l'autre cote ...)*/
			poll(pfds, num_procs * 2, -1);
			for (i = 0; i < num_procs * 2; i++) {
				if (pfds[i].revents == POLLIN) {
					r = read(pfds[i].fd, buf, MAXNAME);
					if (!r) {
						perror("Erreur de lecture");
					} else {
						printf("le message: %s \n", buf);
					}
				}
			}
		}

		/* on attend les processus fils */

		/* on ferme les descripteurs proprement */
		for (i = 0; i < 2 * num_procs; i++) {
			close(pfds[i].fd);
		}
		free(pfds);
		free(buf);
		/* on ferme la socket d'ecoute */
		close(listen_sock);
	}
	exit(EXIT_SUCCESS);
}

