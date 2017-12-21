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

int eflag = 0, dflag = 0, key_len = 16, tserver = 10, fserver = 5;
char *enckey, *enckeyf, *IV;
MCRYPT td;
int key_fd, logffd;
char buffer[256];
pid_t cpid;
struct termios old_termios;

void error_exit(const char *msg, int num) {
	(void)num; // bypass -Wextra compiler warning
	fprintf(stderr, "ERROR %s (%d): %s", msg, errno, strerror(errno));
	exit(1);
}

void pexit() {
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

void save_term() {
	if (tcgetattr(STDIN_FILENO, &old_termios) <= -1) 
		error_exit("save failed", errno);
}

void restore_term() {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &old_termios) <= -1)
		error_exit("restore failed", errno);
}

void do_write(char *buffer, int ofd, int nbytes) {
	int i = 0;
	while (i < nbytes) {
		char ch = *(buffer + i);
		if ((ch == '\r' || ch == '\n') && ofd == STDOUT_FILENO) {
			char plc[2] = { '\r', '\n' };
			swrite(ofd, plc, 2);
		}
		if ((ch == '\r' || ch == '\n') && ofd != STDOUT_FILENO) {
			char plc[0]; 
			plc[0] = '\n';
			swrite(ofd, plc, 1);
		}
		else { 
			swrite(ofd, buffer + i, 1); 
		}
		i++;
	}
}

void do_write_log(char *buffer, int nbytes, int loc) {
	if (loc == tserver) {
		if (dprintf(logffd, "SENT %d bytes: ", nbytes) <= -1) error_exit("write to log failed", errno);
		do_write(buffer, logffd, nbytes);
		swrite(logffd, "\n", 1);
	}
	if (loc == fserver) {
		if (dprintf(logffd, "RECEIVED %d bytes: ", nbytes) <= -1) error_exit("write to log failed", errno);
		do_write(buffer, logffd, nbytes);
		swrite(logffd, "\n", 1);
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
	int numread = read(key_fd, enckey, key_len);
	if (numread != key_len) error_exit("failed to read key", errno);

	int c = close(key_fd);
	if (c <= -1) error_exit("failed to close key file", errno);

	int i = 0;
	while (i < iv_size) {
		IV[i] = 0;
		i++;
	}

	if (mcrypt_generic_init(td, enckey, key_len, IV) <= -1) error_exit("failed to start encryption process", errno);

	free(enckey);
	free(IV);
}

int main(int argc, char **argv) {
	int opt, pnum;
	char *lfile;
	int lflag = 0, pflag = 0;
	static struct option long_options[] = {
		{ "port", required_argument, 0, 'p' },
		{ "log", optional_argument, 0, 'l' },
		{ "encrypt", required_argument, 0, 'e' },
		{ "debug", no_argument, 0, 'd' },
		{ 0, 0, 0, 0 }
	};
	while ((opt = getopt_long(argc, argv, "p:l:e:d", long_options, NULL)) != -1) {
		if (opt == 'p') {
			pnum = atoi(optarg);
			pflag = 1;
		}
		else if (opt == 'l') {
			lfile = optarg;
			lflag = 1;
		}
		else if (opt == 'e') {
			enckeyf = optarg;
			eflag = 1;
		}
		else if (opt == 'd') {
			dflag = 1;
		}
		else pexit();
	}

	if (!pflag) pexit();

	if (lflag) {
		logffd = creat(lfile, S_IRWXU);
		if (logffd <= -1) error_exit("failed creat()", errno);
	}

	if (eflag) setup_encryption();

	save_term();
	atexit(restore_term);

	struct termios non_canonical_input_mode;
	tcgetattr(STDIN_FILENO, &non_canonical_input_mode);

	non_canonical_input_mode.c_iflag = ISTRIP; //only lower 7 bits
	non_canonical_input_mode.c_oflag = 0; // no processing
	non_canonical_input_mode.c_lflag = 0; // no processing

	if (tcsetattr(STDIN_FILENO, TCSANOW, &non_canonical_input_mode) <= -1) error_exit("failed to establish terminal ", errno);

	struct sockaddr_in addy;
	struct hostent *server = gethostbyname("localhost");
	int socket_id = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_id <= -1) error_exit("failed to open socket", errno);
	memset((char *)&addy, 0, sizeof(addy));
	addy.sin_family = AF_INET;
	addy.sin_port = htons(pnum);
	memcpy((char *)&addy.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

	if (connect(socket_id, (struct sockaddr*)&addy, sizeof(addy)) <= -1) error_exit("error connecting \n", errno);

	struct pollfd poll_s[2];
	poll_s[0].fd = STDIN_FILENO;
	poll_s[1].fd = socket_id;

	poll_s[0].events = POLLIN | POLLHUP | POLLERR;
	poll_s[1].events = POLLIN | POLLHUP | POLLERR;

	for (;;) {

		if (poll(poll_s, 2, 0) <= -1) {
			fprintf(stderr, "poll failed");
			exit(1);
		}

		if (poll(poll_s, 2, 0) == 0) continue;

		if (poll_s[0].revents & POLLIN) {
			char frmstdin[256];
			int numread = sread(STDIN_FILENO, frmstdin, 256);
			do_write(frmstdin, STDOUT_FILENO, numread);
			if (eflag) {
				int n=0;
				while (n < numread) {
					if (frmstdin[n] != '\r' && frmstdin[n] != '\n' && (mcrypt_generic(td, &frmstdin[n], 1) != 0)) {
						error_exit("failed to encrypt", errno);
					}
					n++;
				}
			}
			if (lflag) do_write_log(frmstdin, numread, tserver);
			do_write(frmstdin, poll_s[1].fd, numread);
		}
		if (poll_s[1].revents & POLLIN) {
			char frmsrv[256];
			int numread = sread(poll_s[1].fd, frmsrv, 256);
			if (numread == 0) {
				if (close(socket_id) <= -1) error_exit("failed to close", errno);
				exit(0);
			}
			if (lflag) do_write_log(frmsrv, numread, fserver);
			if (eflag && (mdecrypt_generic(td, &frmsrv, numread) != 0))
				error_exit("failed to encrypt", errno);
			do_write(frmsrv, STDOUT_FILENO, numread);
		}
		if (poll_s[1].revents & (POLLHUP | POLLERR)) {
			if (close(socket_id) <= -1) error_exit("failed to close", errno);
			exit(0);
		}
	}
	exit(0);
}