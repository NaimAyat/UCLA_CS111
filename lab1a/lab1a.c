//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000

/* A program that establishes:
1. Character-at-a-time, full duplex terminal I/O
2. Polled I/O and passing input and output between two processes */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int die = -1;
struct termios old_termios;
int set = 0;
struct pollfd poll_list[2];

void set_die(int flag) {
	(void)flag;	// to bypass "unused variable" compiler warning
	die = 1;
}

void sig_handler(int signum) {
	if (signum == SIGPIPE) {
		close(poll_list[0].fd);
		close(poll_list[1].fd);
		set = 1;
	}
}

void revert_state() {
	tcsetattr(0, TCSAFLUSH, &old_termios);
}

int main(int argc, char **argv) {

	struct termios new_termios = old_termios;

	new_termios.c_iflag = ISTRIP;	// only lower 7 bits
	new_termios.c_oflag = 0;		// no processing
	new_termios.c_lflag = 0;		// no processing

	int opt = -1;

	struct option long_options[2] =
	{
	  {"shell", no_argument, &opt, 1},
	  {0, 0, 0, 0}
	};
	int oref;
	int ret = 0;
	while (1) {
		ret = getopt_long(argc, argv, "", long_options, &oref);

		if (ret < 0)
			break;

		else if (ret > 0) {
			fprintf(stderr, "ERROR: Unrecognized argument. Correct usage: ./lab1a --shell\n");
			exit(1);
		}
	}

	tcgetattr(0, &old_termios);

	if (tcsetattr(0, TCSAFLUSH, &new_termios) < 0) {
		fprintf(stderr, "ERROR: tcsetattr failed (%s).\n", strerror(errno));
		exit(1);
	}

	atexit(revert_state);

	if (opt > 0) {
		int in_pipe[2];
		int out_pipe[2];

		if (pipe(in_pipe) < 0) {
			fprintf(stderr, "ERROR: failed to pipe to input (%s).\n", strerror(errno));
			exit(1);
		}
		if (pipe(out_pipe) < 0) {
			fprintf(stderr, "ERROR: failed to pipe from output (%s). \n", strerror(errno));
			exit(1);
		}

		pid_t new_pid = fork();
		if (new_pid < 0) {
			fprintf(stderr, "ERROR: fork failed (%s). \n", strerror(errno));
			exit(1);
		}

		if (new_pid == 0) {
			char **arr = { NULL }; // to bypass "non-null required" compiler warning
			close(in_pipe[1]);
			close(out_pipe[0]);
			close(0);
			dup(in_pipe[0]);
			close(1);
			dup(out_pipe[1]);
			close(2);
			dup(out_pipe[1]);
			if (execvp("/bin/bash", arr)) {
				fprintf(stderr, "ERROR: execl failed (%s).\n", strerror(errno));
				exit(1);
			}
		}

		close(in_pipe[0]);
		close(out_pipe[1]);

		poll_list[0].fd = 0;
		poll_list[0].events = POLLIN;
		poll_list[1].fd = out_pipe[0];
		poll_list[1].events = POLLIN;

		signal(SIGPIPE, *sig_handler);

		int in_closed = -1;

		while (1) {
			if (poll(poll_list, 2, 0) < 0) {
				fprintf(stderr, "ERROR: failed to read from poll (%s).\n", strerror(errno));
				exit(1);
			}
			if (poll_list[0].revents & POLLIN) {
				char buf[1024];
				int bytes = read(0, buf, 1024);
				if (bytes < 0) {
					fprintf(stderr, "ERROR: failed to read from input (%s).\n", strerror(errno));
					exit(1);
				}
				char ebuffer[1024];
				int ebytes = 0;
				char sbuffer[1024];
				int eof = 0;
				int i = 0;
				while (i < bytes) {
					switch (buf[i]) {
						case ('\r'): {
							ebuffer[ebytes] = '\r';
							ebytes++;
							ebuffer[ebytes] = '\n';
							sbuffer[i] = '\n';
							break;
						}
						case ('\n'): {
							ebuffer[ebytes] = '\r';
							ebytes++;
							ebuffer[ebytes] = '\n';
							sbuffer[i] = '\n';
							break;
						}
						case (0x03): {
							ebuffer[ebytes] = '^';
							ebytes++;
							ebuffer[ebytes] = 'C';
							sbuffer[i] = ' ';
							kill(new_pid, SIGINT);
							break;
						}
						case (0x04): {
							ebuffer[ebytes] = '^';
							ebytes++;
							ebuffer[ebytes] = 'D';
							eof = 1;
							sbuffer[i] = ' ';
							break;
						}
						default: {
							ebuffer[ebytes] = buf[i];
							sbuffer[i] = buf[i];
						}
					}
					ebytes++;
					i++;		
				}
				if (write(1, ebuffer, ebytes) < 0) {
					fprintf(stderr, "ERROR: failed to write (%s).\n", strerror(errno));
					exit(1);
				}
				if ((in_closed < 0) && write(in_pipe[1], sbuffer, bytes) < 0 && (die == -1)) {
					fprintf(stderr, "ERROR: failed to write to pipe (%s).\n", strerror(errno));
					exit(1);
				}
				if ((eof > 0) && (in_closed < 0)) {
					if (close(in_pipe[1]) < 0) {
						fprintf(stderr, "ERROR: failed to close pipe to input (%s).\n", strerror(errno));
						exit(1);
					}
					in_closed = 1;
				}
			}
			if (poll_list[1].revents & POLLIN) {
				char buf[1024];
				int bytes = read(out_pipe[0], buf, 1024);
				if (bytes < 0) {
					fprintf(stderr, "ERROR: failed to read from output pipe (%s).\n", strerror(errno));
					exit(1);
				}
				char ebuffer[1024];
				int ebytes = 0;
				int i = 0; 
				while (i < bytes) {
					switch (buf[i]) {
						case ('\r'): {
							ebuffer[ebytes] = '\r';
							ebytes++;
							ebuffer[ebytes] = '\n';
							break;
						}
						case ('\n'): {
							ebuffer[ebytes] = '\r';
							ebytes++;
							ebuffer[ebytes] = '\n';
							break;
						}
						case (0x04): {
							ebuffer[ebytes] = buf[i];
							die = 1;
							break;
						}
						default: {
							ebuffer[ebytes] = buf[i];
						}
					}
					ebytes++;
					i++;
				}
				if (write(1, ebuffer, ebytes) < 0) {
					fprintf(stderr, "ERROR: failed to write (%s).\n", strerror(errno));
					exit(1);
				}
			}
			if ((poll_list[1].revents & POLLHUP) || (die > 0)) {
				if ((in_closed < 0) && (close(in_pipe[1]) < 0)) {
					fprintf(stderr, "ERROR: failed close input pipe (%s).\n", strerror(errno));
					exit(1);
				}
				int status;
				if (waitpid(new_pid, &status, 0) < 0) {
					fprintf(stderr, "ERROR: waitpid failed (%s).\n", strerror(errno));
					exit(1);
				}
				fprintf(stderr, "\r\n1SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(status), WEXITSTATUS(status));
				tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
				break;
			}
		}
	}
	else {
		int eof = -1;
		while (eof < 0) {
			char buf[1024];
			int bytes = read(0, buf, 1024);
			if (bytes < 0) {
				fprintf(stderr, "ERROR: read failed (%s).\n", strerror(errno));
				exit(1);
			}
			char ebuffer[1024];
			int ebytes = 0;
			int i = 0;
			while (i < bytes) {
				switch (buf[i]) {
					case ('\r'): {
						ebuffer[ebytes] = '\r';
						ebytes++;
						ebuffer[ebytes] = '\n';
						break;
					}
					case ('\n'): {
						ebuffer[ebytes] = '\r';
						ebytes++;
						ebuffer[ebytes] = '\n';
						break;
					}
					case (0x04): {
						ebuffer[ebytes] = '^';
						ebytes++;
						ebuffer[ebytes] = 'D';
						eof = 1;
						break;
					}
					default: {
						ebuffer[ebytes] = buf[i];
					}
				}
				ebytes++;
				i++;
			}
			if (write(1, ebuffer, ebytes) < 0) {
				fprintf(stderr, "ERROR: write failed (%s).", strerror(errno));
				exit(1);
			}
		}
	}
	return 0;
}