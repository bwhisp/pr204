#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* autres includes (eventuellement) */
#define BACKLOG 20
#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}
#define MAXNAME 256

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   char machine[MAXNAME];
   int sockfd; //socket communiquée
   int port;
};
typedef struct dsm_proc_conn dsm_proc_conn_t; 

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {   
  pid_t pid;
  dsm_proc_conn_t info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int type, int *port_num);
struct sockaddr_in init_addr (struct sockaddr_in addr_in);

ssize_t do_read(int fd, char * buf);
int do_write(int fd, void * buf);
// pour recuperer l'@Ip à partir du hostname passé en argument à dsmwrap
char *get_ip(const char * s) ;
void redirections(int fderr[2],int fdout[2]);
int do_send(int fd, void * buf);
