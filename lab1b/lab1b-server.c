//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000

#include <termios.h>
#include <mcrypt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>

int eflag = 0, key_len = 16, key_fd = -1, dflag = 0, to_sh = 1, to_sock = 0;
char *enckey, *enckeyf, *IV;
MCRYPT td;
int pipe_shell[2];
int pipe_pipe[2];
pid_t cpid = -1;

void error_exit(const char *msg, int num) {
	(void)num; // bypass -Wextra compiler warning
	fprintf(stderr, "ERROR %s (%d): %s", msg, errno, strerror(errno));
	exit(1);
}

void prexit() {
	fprintf(stderr, "ERROR: incorrect usage. Correct usage: server --port=number [--encrypt=filename] \n");
	exit(1);
}

int sread(int fd, char *buffer, int s) {
	ssize_t nbytes = read(fd, buffer, s);
	if (nbytes <= -1) error_exit("failed to read", errno);
	return nbytes;
}

int swrite(int fd, char *buffer, int s) {
	ssize_t nbytes = write(fd, buffer, s);
	if (nbytes <= -1) error_exit("failed to write", errno);
	return nbytes;
}

void cleanup() {
	int lstats;
	pid_t w = waitpid(cpid, &lstats, 0);
	if (w <= -1) error_exit("waitpid failed", errno);
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(lstats), WEXITSTATUS(lstats));
	exit(0);
}

void handler(int signal) {
	if (signal == SIGINT) {
		kill(cpid, SIGINT);
	}
}

void setup_encryption() {
	td = mcrypt_module_open("twofish", NULL, "cfb", NULL); 
	if (td == MCRYPT_FAILED) error_exit("mcrypt failed", errno);

	key_fd = open(enckeyf, O_RDONLY);
	if (key_fd <= -1) error_exit("unable to open key file", errno);

	enckey = malloc(key_len);
	if (enckey == NULL) error_exit("failed to allocate memory for key", errno);

	int iv_size = mcrypt_enc_get_iv_size(td);
	IV = malloc(iv_size);
	if (IV == NULL) error_exit("failed to allocate memory for IV", errno);

	memset(enckey, 0, key_len);
	int bytes_read = read(key_fd, enckey, key_len);
	if (bytes_read != key_len) error_exit("failed to read key", errno);

	int c = close(key_fd);
	if (c <= -1) error_exit("failed to close key file", errno);

	int i=0;
	while (i < iv_size) {
		IV[i] = 0;
		i++;
	}

	if (mcrypt_generic_init(td, enckey, key_len, IV) <= -1) error_exit("failed to start encryption process", errno);

	free(enckey);
	free(IV);
}

void do_write(char *buffer, int ofd, int nbytes, int to) {
	int i=0;
	while (i < nbytes) {
		char ch = *(buffer + i);
		if (ch == 0x03 && to == to_sh && dflag) {
			if (kill(cpid, SIGINT) <= -1) error_exit("KILL FAILED", errno);
		}
		if (ch == 0x04 && to == to_sh && dflag) {
			close(pipe_shell[1]);
		}
		else {
			swrite(ofd, buffer + i, 1);
		}
		i++;
	}
}
int main(int argc, char *argv[]) {
	int opt, portnum, socket_fd, sock_tds;
	int port_flag = 0;

	static struct option long_options[] = {
		{ "port", required_argument, 0, 'p' },
		{ "encrypt", required_argument, 0, 'e' },
		{ "debug", no_argument, 0, 'd' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = getopt_long(argc, argv, "p:e:d", long_options, NULL)) >= 0) {
		if (opt == 'p') {
			port_flag = 1;
			portnum = atoi(optarg);
		}
		else if (opt == 'e') {
			eflag = 1;
			enckeyf = optarg;
		}
		else if (opt == 'd') {
			dflag = 1;
		}
		else prexit();
	}

	if (!port_flag) prexit();

	if (eflag) setup_encryption();

	if (signal(SIGINT, &handler) == SIG_ERR) error_exit("signal failed", errno);
	if (signal(SIGPIPE, &handler) == SIG_ERR) error_exit("signal failed", errno);
	if (signal(SIGTERM, &handler) == SIG_ERR) error_exit("signal failed", errno);

	struct sockaddr_in serv_addr, cli_addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd <= -1) error_exit("failed to open socket", errno);
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnum);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) <= -1) error_exit("binding error \n", errno);

	if (listen(socket_fd, 5) <= -1) error_exit("socket failure", errno);
	unsigned int clilen = sizeof(cli_addr);
	sock_tds = accept(socket_fd, (struct sockaddr *) &cli_addr, &clilen);

	if (sock_tds <= -1) error_exit("failed pipe", errno);
	if (pipe(pipe_shell) <= -1) error_exit("failed pipe", errno);
	if (pipe(pipe_pipe) <= -1) error_exit("failed pipe", errno);

	cpid = fork();
	if (cpid <= -1) error_exit("failed fork", errno);
	if (cpid == 0) {
		close(pipe_shell[1]);
		close(pipe_pipe[0]);
		dup2(pipe_shell[0], STDIN_FILENO);
		dup2(pipe_pipe[1], STDOUT_FILENO);
		dup2(pipe_pipe[1], STDERR_FILENO);
		close(pipe_shell[0]);
		close(pipe_pipe[1]);

		char **arguments = { NULL };
		if (execvp("/bin/bash", arguments) == -1) {
			fprintf(stderr, "ERROR in executing bash\n");
			exit(1);
		}
	}
	else {
		if (close(pipe_shell[0]) <= -1) error_exit("close failed", errno);
		if (close(pipe_pipe[1]) <= -1) error_exit("close failed", errno);

		struct pollfd poll_s[2];
		poll_s[0].fd = sock_tds;
		poll_s[1].fd = pipe_pipe[0];

		poll_s[0].events = POLLIN | POLLHUP | POLLERR;
		poll_s[1].events = POLLIN | POLLHUP | POLLERR;

		for (;;) {

			if (poll(poll_s, 2, 0) <= -1) {
				fprintf(stderr, "poll failed");
				exit(1);
			}

			if (poll(poll_s, 2, 0) == 0) continue;

			if (poll_s[0].revents & POLLIN) {
				char fsocket[256];
				int bflag = read(sock_tds, fsocket, 256);
				if (bflag < 1) {
					if (kill(cpid, SIGTERM) <= -1) error_exit("failed to kill", errno);
					cleanup();
					break;
				}
				if (eflag) {
					int n = 0;
					while (n < bflag) {
						if (fsocket[n] != '\r' && fsocket[n] != '\n' && mdecrypt_generic(td, &fsocket[n], 1) != 0) {
							error_exit("failed to encrypt", errno);
						}
						n++;
					}
				}
				do_write(fsocket, pipe_shell[1], bflag, to_sh);
			}

			if (poll_s[1].revents & POLLIN) {
				char pipe_pipe_buffer[256];
				int bflag = sread(pipe_pipe[0], pipe_pipe_buffer, 256);
				if (bflag == 0) {
					cleanup();
					break;
				}
				if (eflag && mcrypt_generic(td, &pipe_pipe_buffer, bflag) != 0) {
					error_exit("failed to encrypt", errno);
				}
				do_write(pipe_pipe_buffer, sock_tds, bflag, to_sock);
			}
			if (poll_s[1].revents & (POLLHUP | POLLERR)) {
				if (close(poll_s[1].fd) <= -1) error_exit("failed to close", errno);
				cleanup();
				break;
			}
		}
	}
	exit(0);
}