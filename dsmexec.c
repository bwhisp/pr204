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
		int port, k, r, listen_sock, nfds;
		int fdout[2], fderr[2];
		char ** machines;
		char ** newargv;
		char *buf = malloc(MAXNAME);
		struct pollfd * pfds;
		struct sockaddr_in c_addr;
		socklen_t addrlen = (socklen_t) sizeof(struct sockaddr_in);

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
		char *my_ip = get_my_ip();
		/* creation de la socket d'ecoute */
		/* + ecoute effective */
		listen_sock = creer_socket(SOCK_STREAM, &port);
		puts("Socket d'écoute initialisée");
		printf("port %d \n", port);

		/* On alloue la place nécessaire au stockage des fd des tubes que l'on créera*/
		pfds = malloc(num_procs * 2 * sizeof(struct pollfd)); // 2 tubes par processus fils
		proc_array = malloc(num_procs * sizeof(dsm_proc_t));

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

				free(proc_array); // car les fils ne s'en servent pas , cest le pere qui s'en charge

				/* redirection stderr */
				close(fderr[0]);
				close(STDERR_FILENO);
				dup(fderr[1]);
				close(fderr[1]);
				puts("redirection stderr faite");

				/* redirection stdout */
				close(fdout[0]);
				close(STDOUT_FILENO);
				dup(fdout[1]);
				close(fdout[1]);
				puts("redirection stdout faite");

				puts("arguments ssh");

				/* Creation du tableau d'arguments pour le ssh */
				newargv = malloc((argc + 4) * sizeof(char *));

				newargv[0] = "ssh";
				newargv[1] = machines[i];
				newargv[2] = "./proj_sys/Phase1/bin/dsmwrap";
				newargv[3] = my_ip;
				char temp[235];
				sprintf(temp, "%d", ntohs(port));
				newargv[4] = temp;

				for (k = 5; k < argc + 3; k++)
					newargv[k] = argv[k - 3];

				newargv[argc + 3] = NULL;

				/* jump to new prog : */
				puts("Execution de ssh");
				fflush(stdout);
				execvp("ssh", newargv);

			} else if (pid > 0) { /* pere */
				/* fermeture des extremites des tubes non utiles */
				close(fdout[1]);
				close(fderr[1]);

				// on remplit le tablea des pollfd
				pfds[3 * i].fd = fdout[0]; // 3i car on a besoin de 3 fd par processus distant
				pfds[3 * i].events = POLLIN | POLLHUP;
				pfds[3 * i + 1].fd = fderr[0]; //
				pfds[3 * i + 1].events = POLLIN | POLLHUP;

				// on remplit proc array
				proc_array[i].pid = pid;
				sprintf(proc_array[i].info.machine, "%s", machines[i]);
				proc_array[i].info.rank = i;
				num_procs_creat++;
			}
		}
		for (i = 0; i < num_procs; i++) {

			/* on accepte les connexions des processus dsm*/
			proc_array[i].info.sockfd = accept(listen_sock, (struct sockaddr *) &c_addr, &addrlen);

			// On rajoute la socket à notre tableau de descripteurs monitorés par poll
			pfds[3 * i + 2].fd = proc_array[i].info.sockfd;
			pfds[3 * i + 2].events = POLLIN | POLLHUP;

			if (proc_array[i].info.sockfd < 0) {
				perror("Erreur de connection");
			}
			/*  On recupere le nom de la machine distante */
			/* 1- d'abord la taille de la chaine */

			/* 2- puis la chaine elle-meme */
			read(proc_array[i].info.sockfd, buf, sizeof(buf));
			printf("Le nom de la machine: %s \n", buf);

			/* On recupere le pid du processus distant  */
			read(proc_array[i].info.sockfd, buf, sizeof(buf));
			printf("Le pid du processus: %s \n", buf); // ??

			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
			read(proc_array[i].info.sockfd, buf, sizeof(buf));
			printf("Le port de la socket d'ecoute: %s \n", buf);
		}

		/* envoi du nombre de processus aux processus dsm*/
		for (k = 0; k < num_procs; k++) {
			sprintf(buf, "%d", num_procs);
			write(proc_array[k].info.sockfd, buf, sizeof(buf));
		}
		/* envoi des rangs aux processus dsm */
		for (k = 0; k < num_procs; k++) {
			sprintf(buf, "%d", proc_array[k].info.rank);
			write(proc_array[k].info.sockfd, buf, sizeof(buf));
		}
		/* envoi des infos de connexion aux processus */

		/* gestion des E/S : on recupere les caracteres */
		/* sur les tubes de redirection de stdout/stderr */
		nfds = num_procs * 3;
		while (1) {
			/* je recupere les infos sur les tubes de redirection
			 jusqu'à ce qu'ils soient inactifs (ie fermes par les
			 processus dsm ecrivains de l'autre cote ...)*/
			poll(pfds, nfds, -1);
			for (i = 0; i < nfds; i++) {
				if (pfds[i].revents == POLLIN) {
					r = read(pfds[i].fd, buf, MAXNAME);
					if (!r) {
						perror("Erreur de lecture");
						memmove(pfds + i, pfds + i + 1, nfds - (i + 1));
						nfds--;
					} else {
						printf("Le message: %s \n", buf);
					}
				} else if (pfds[i].revents == POLLHUP) {
					memmove(pfds + i, pfds + i + 1, nfds - (i + 1));
					nfds--;
				}

			}
		}

		/* on attend les processus fils */
		for (k = 0; k < num_procs; k++) {
			wait(NULL);
		}

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

