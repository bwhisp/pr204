#include "dsm.h"

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID; /* rang (= numero) du processus */

dsm_proc_conn_t * machines;

/* indique l'adresse de debut de la page de numero numpage */
static char *num2address(int numpage) {
	char *pointer = (char *) (BASE_ADDR + (numpage * (PAGE_SIZE)));

	if (pointer >= (char *) TOP_ADDR) {
		fprintf(stderr, "[%i] Invalid address !\n", DSM_NODE_ID);
		return NULL;
	} else
		return pointer;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info(int numpage, dsm_page_state_t state,
		dsm_page_owner_t owner) {
	if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {
		if (state != NO_CHANGE)
			table_page[numpage].status = state;
		if (owner >= 0)
			table_page[numpage].owner = owner;
		return;
	} else {
		fprintf(stderr, "[%i] Invalid page number !\n", DSM_NODE_ID);
		return;
	}
}

static dsm_page_owner_t get_owner(int numpage) {
	return table_page[numpage].owner;
}

static dsm_page_state_t get_status(int numpage) {
	return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page(int numpage) {
	char *page_addr = num2address(numpage);
	mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE,
	MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return;
}

/* Changement de la protection d'une page */
static void dsm_protect_page(int numpage, int prot) {
	char *page_addr = num2address(numpage);
	mprotect(page_addr, PAGE_SIZE, prot);
	return;
}

static void dsm_free_page(int numpage) {
	char *page_addr = num2address(numpage);
	munmap(page_addr, PAGE_SIZE);
	return;
}

static void *dsm_comm_daemon(void *arg) {
	struct pollfd * pfds = malloc(DSM_NODE_NUM * sizeof(struct pollfd));
	char * buf = malloc(MAXNAME);
	int k, r;
	int nfds = DSM_NODE_NUM;

	for (k = 0; k < DSM_NODE_NUM; k++) {
		pfds[k].fd = machines[k].sockfd;
		pfds[k].events = POLLIN | POLLHUP;
	}

	while (1) {
		printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);

		if (poll(pfds, nfds, -1) == -1 && errno != EINTR)
			perror("Poll error :");

		for (k = 0; k < DSM_NODE_NUM; k++) {
			if (pfds[k].revents & POLLIN) {
				memset(buf, 0, MAXNAME);
				r = read(pfds[k].fd, buf, MAXNAME);

				if (!r) { // Socket fermé
					shutdown(machines[k].sockfd, 2);
					close(machines[k].sockfd);
					memmove(pfds + k, pfds + k + 1, nfds - (k + 1));
					nfds--;
				} else { // On a une requête, la traiter

				}
			}
			if (pfds[k].revents & POLLHUP) { // Socket fermé
				shutdown(machines[k].sockfd, 2);
				close(machines[k].sockfd);
				memmove(pfds + k, pfds + k + 1, nfds - (k + 1));
				nfds--;
			}
		}

		sleep(2);
	}
	return NULL;
}

static int dsm_send(int dest, void *buf, size_t size) {
	char *taille = malloc(sizeof(size_t));
	size_t len = strlen(buf);

	sprintf(taille, "%d", (int) len);
	send(dest, taille, sizeof(size_t), 0);

	send(dest, buf, len, 0);

	return 0;
}

static int dsm_recv(int from, void *buf, size_t size) {
	ssize_t rl;
	ssize_t r;
	char *taille = malloc(sizeof(size_t));

	memset(buf, 0, MAXNAME);

	rl = recv(from, taille, sizeof(size_t), 0);
	if (rl == 0) {
		return 0;
	}
	r = recv(from, buf, (size_t) atoi(taille), 0);

	return r;
}

static void dsm_handler(void) {
	/* A modifier */
	printf("[%i] FAULTY  ACCESS !!! \n", DSM_NODE_ID);
	abort();
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context) {
	/* A completer */
	/* adresse qui a provoque une erreur */
	void *addr = info->si_addr;
	/* Si ceci ne fonctionne pas, utiliser a la place :*/
	/*
	 #ifdef __x86_64__
	 void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
	 #elif __i386__
	 void *addr = (void *)(context->uc_mcontext.cr2);
	 #else
	 void  addr = info->si_addr;
	 #endif
	 */
	/*
	 pour plus tard (question ++):
	 dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
	 */
	/* adresse de la page dont fait partie l'adresse qui a provoque la faute */
	void *page_addr = (void *) (((unsigned long) addr) & ~(PAGE_SIZE - 1));

	if ((addr >= (void *) BASE_ADDR) && (addr < (void *) TOP_ADDR)) {
		printf("SEGFAULT À TRAITER");
		fflush(stdout);
		dsm_handler();
	} else {
		/* SIGSEGV normal : ne rien faire*/
		printf("SEGFAULT NORMAL");
		fflush(stdout);
		dsm_finalize();
		exit(EXIT_FAILURE);
	}
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv) {
	struct sigaction act;
	int index;
	int k, c;
	int sock = 3; //socket de communication avec la machine "lanceur"
	int l_sock = 4; // socket d'écoute
	char buf[MAXNAME];
	struct sockaddr_in c_addr;
	socklen_t addrlen = (socklen_t) sizeof(struct sockaddr_in);

	/* reception du nombre de processus dsm envoye */
	/* par le lanceur de programmes (DSM_NODE_NUM)*/
	dsm_recv(sock, buf, MAXNAME);
	DSM_NODE_NUM = atoi(buf);

	/* reception de mon numero de processus dsm envoye */
	/* par le lanceur de programmes (DSM_NODE_ID)*/
	dsm_recv(sock, buf, MAXNAME);
	DSM_NODE_ID = atoi(buf);
	printf("My rank is %i \n", DSM_NODE_ID);
	fflush(stdout);

	/* reception des informations de connexion des autres */
	/* processus envoyees par le lanceur : */
	/* nom de machine, numero de port, etc. */
	machines = malloc(DSM_NODE_NUM * sizeof(dsm_proc_conn_t));

	for (k = 0; k < DSM_NODE_NUM; k++) {
		dsm_recv(sock, buf, MAXNAME);
		sprintf(machines[k].machine, "%s", buf);
		dsm_recv(sock, buf, MAXNAME);
		machines[k].port = atoi(buf);
		machines[k].rank = k;
	}
	// Test reception des infos de connexion
	/*
	 for (k = 0; k < DSM_NODE_NUM; k++) {
	 printf("Machine : %s Port %i Rank %i \n", machines[k].machine,
	 machines[k].port, machines[k].rank);
	 fflush(stdout);
	 }
	 */

	/* initialisation des connexions */
	/* avec les autres processus : connect/accept */
	 //Bloc connect
	if (DSM_NODE_ID != DSM_NODE_NUM - 1) {
		for (k = DSM_NODE_ID + 1; k < DSM_NODE_NUM; k++) {
			memset(&c_addr, 0, addrlen);
			c_addr.sin_family = AF_INET;
			inet_aton(get_ip(machines[k].machine), &(c_addr.sin_addr));
			c_addr.sin_port = (in_port_t) machines[k].port;

			machines[k].sockfd = socket(AF_INET, SOCK_STREAM, 0);

			do {
				c = connect(machines[k].sockfd, (struct sockaddr*) &c_addr,
						addrlen);
			} while (c < 0 && (errno == ECONNREFUSED && errno == EINTR));
			if (c < 0)
				perror("Connect dsm : ");
			else {
				printf("[proc %i] Connection with machine %i established !\n",
						DSM_NODE_ID, k);
				fflush(stdout);
			}
		}
	}
	//Bloc accept
	for (k = DSM_NODE_ID - 1; k > -1; k--) {
		do {
			machines[k].sockfd = accept(l_sock, (struct sockaddr *) &c_addr,
					&addrlen);
		} while (machines[k].sockfd < 0 && errno == EINTR);

		if (machines[k].sockfd == -1)
			perror("Accept dsm : ");
		else {
			printf("[proc %i] Connection from machine %i accepted !\n",
					DSM_NODE_ID, k);
			fflush(stdout);
		}
	}
	/* Test des inter-connexions : decommenter pour tester
	 bzero(buf, MAXNAME);
	 sprintf(buf, "Hello I m %i", DSM_NODE_ID);

	 for (k = 0; k < DSM_NODE_NUM; k++) {
	 if (k != DSM_NODE_ID)
	 dsm_send(machines[k].sockfd, buf, MAXNAME);
	 }

	 for (k = 0; k < DSM_NODE_NUM; k++) {
	 if (k != DSM_NODE_ID) {
	 dsm_recv(machines[k].sockfd, buf, MAXNAME);
	 printf("%s \n", buf);
	 fflush(stdout);
	 }
	 }
	 */

	/* Allocation des pages en tourniquet */
	for (index = 0; index < PAGE_NUMBER; index++) {
		if ((index % DSM_NODE_NUM) == DSM_NODE_ID)
			dsm_alloc_page(index);
		dsm_change_info(index, WRITE, index % DSM_NODE_NUM);
	}

	/* mise en place du traitant de SIGSEGV */
	act.sa_flags = SA_SIGINFO | SA_RESTART;
	act.sa_sigaction = segv_handler;
	sigaction(SIGSEGV, &act, NULL);

	/* creation du thread de communication */
	/* ce thread va attendre et traiter les requetes */
	/* des autres processus */
	pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
	/* Adresse de début de la zone de mémoire partagée */
	return ((char *) BASE_ADDR);
}

void dsm_finalize(void) {
	int k;
	/* fermer proprement les connexions avec les autres processus */
	// Fermeture socket dsmexec <-> dsm
	shutdown(3, 2);
	close(3);
	// Fermeture socket d'ecoute
	shutdown(4, 2);
	close(4);
	for (k = 0; k < DSM_NODE_NUM; k++) {
		shutdown(machines[k].sockfd, 2);
		close(machines[k].sockfd);
	}

	/* terminer correctement le thread de communication */
	/* pour le moment, on peut faire : */
	pthread_cancel(comm_daemon);

	return;
}
