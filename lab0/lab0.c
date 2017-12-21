// NAME: Naim Ayat
// EMAIL: naimayat@ucla.edu
// ID: 000000000

/* A program that copies its standard input to its standard output by reading from file 
descriptor 0 (until encountering an end of file) and writing to file descriptor 1. */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

// definitions to make options less verbose
#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 'f'
#define CATCH 'c'

// save a value for errno so fprintf does not affect it
extern int errno;

// exit with a return code of 4 if segmentation fault is caught
void signalHandler() {
	fprintf(stderr, "ERROR 4: Caught segmentation fault.\n");
	exit(4);
}

int main(int argc, char** argv)
{
	// long options
	struct option long_options[] =
	{
	  {"input", required_argument, NULL, INPUT},
	  {"output", required_argument, NULL, OUTPUT},
	  {"segfault", no_argument, NULL, SEGFAULT},
	  {"catch", no_argument, NULL, CATCH},
	  {0, 0, 0, 0}
	};

	int segFault = 0;
	int ret = 0;

	while (1) {
		ret = getopt_long(argc, argv, "fci:o:", long_options, NULL);

		if (ret == -1)
			break;

		switch (ret) {
			// use specified file as stdin
			case INPUT:
			{
				int inFile = open(optarg, O_RDONLY);
				if (inFile >= 0) {
					close(0);
					dup(inFile);
					close(inFile);
				}
				// exit with a return code of 2 if input file can't be opened

				else {
					fprintf(stderr, "ERROR 2: Unable to open the specified input file (%s). \n", strerror(errno));
					exit(2);
				}
				break;
			}
			// use specified file as stdout
			case OUTPUT:
			{
				int outFile = creat(optarg, 0666);
				if (outFile >= 0) {
					close(1);
					dup(outFile);
					close(outFile);
				}
				// exit with a return code of 3 if output file can't be created
				else {
					fprintf(stderr, "ERROR 3: Unable to create the specified output file (%s).\n", strerror(errno));
					exit(3);
				}
				break;
			}
			// trigger segmentation fault
			case SEGFAULT:
			{
				segFault = 1;
				break;
			}
			// register a SIGSEGV handler that catches the segmentation fault
			case CATCH:
			{
				signal(SIGSEGV, signalHandler);
				break;
			}

			// exit with a return code of 1 if invalid argument is encountered
			default:
			{
				fprintf(stderr, "ERROR 1: Unrecognized argument. Correct usage: ./lab0 [--input=file] [--output=file] [--catch] [--segfault]\n");
				exit(1);
			}
		}
	}
	
	// force segmentation fault
	if (segFault == 1)
	{
		char* ptr = NULL;
		*ptr = SEGFAULT;
	}

	// copy from stdin to stdout
	int x;
	char curr;
	x = read(0, &curr, sizeof(char));
	while (x > 0)
	{
		write(1, &curr, sizeof(char));
		x = read(0, &curr, sizeof(char));
	}

	// successful exit
	exit(0);
}
