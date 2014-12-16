#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>

int main(int argc, char *argv[]) {
	int fd;
	int i;
	char str[1024];
	char exec_path[1024];
	char *wd_ptr = NULL;
	char buf[1024];

	wd_ptr = getcwd(str, 1024);
	bzero(buf, 1024);
	sprintf(buf, "Working dir is %s\n", str);
	write(STDOUT_FILENO, buf, 1024);

	bzero(buf, 1024);
	sprintf(buf, "Number of args : %i\n", argc);
	write(STDOUT_FILENO, buf, 1024);

	for (i = 0; i < argc; i++) {
		bzero(buf, 1024);
		sprintf(buf, "arg[%i] : %s\n", i, argv[i]);
		write(STDERR_FILENO, buf, 1024);
	}

	sprintf(exec_path, "%s/%s", str, "titi");
	fd = open(exec_path, O_RDONLY);
	if (fd == -1)
		perror("open");
	bzero(buf, 1024);
	sprintf(buf, "================ Valeur du descripteur : %i\n", fd);
	write(STDERR_FILENO, buf, 1024);

	fflush(stdout);
	fflush(stderr);
	return 0;
}
