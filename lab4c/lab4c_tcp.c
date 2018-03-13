//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <netdb.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <mraa.h>
#include <aio.h>
#include <fcntl.h>

int iflag = 0, log_flag, logOpt = 0, rep = 1, IDFlag = 0, hostFlag = 0, scale = 'F', port = -1;
long duration = 1;
char *ID, *hostName, *FH;
FILE *myFile;
mraa_aio_context tempSensor;

void startRecording(int flg) {
	rep = flg;
	if (logOpt) {
		switch (flg) {
		case 0:
			fprintf(myFile, "STOP\n");
			break;
		case 1:
			fprintf(myFile, "START\n");
			break;
		default:
			(void)flg;
		}
	}
	return;
}

void scaleHandler(char flg) {
	scale = flg;
	if (logOpt) {
		switch (flg) {
		case 'C': {
			fprintf(myFile, "SCALE=C\n");
			break;
		}
		case 'F': {
			fprintf(myFile, "SCALE=F\n");
			break;
		}
		default:
			(void)flg;
		}
	}
	return;
}

void convertTime(char *systemTime) {
	time_t t1;
	time(&t1);
	struct tm* replace = localtime(&t1);
	if (replace == NULL) {
		exit(2);
	}
	strftime(systemTime, 9, "%H:%M:%S", replace);
	return;
}

void pConverter(char *buffer) {
	char *s = buffer + strlen("PERIOD="), *e;
	duration = strtol(s, &e, 10);
	if (logOpt)
		fprintf(myFile, "PERIOD=%lu\n", duration);
	return;
}

void shutDown() {
	char tarr[50];
	convertTime(tarr);
	fprintf(stdout, "%s %s\n", tarr, "SHUTDOWN");
	if (logOpt) {
		fprintf(myFile, "%s\n", "OFF");
		fprintf(myFile, "%s %s\n", tarr, "SHUTDOWN");
	}
	exit(0);
}

float tempConverter(int x) {
	// for RV5
	float temp5 = (1023 / x) - 1;
	temp5 = temp5 * 100000;
	float result = 0;
	result = 1 / (log(temp5 / 100000) / 4275 + 1 / 298.15) - 273.15;
	// for R3V3
	// float temp3 = (660/x)-1;
	// temp3 = temp3*100000;
	// result = 1/(log(temp3/100000)/4275+1/298.15)-273.15;
	return result;
}

void * writeData() {
	for (;;) {
		if (rep > 0) {
			float newTemp = tempConverter(mraa_aio_read(tempSensor));
			if (scale == 'F')
				newTemp = 32 + (1.8*newTemp);
			char tarr[20];
			convertTime(tarr);
			if (log_flag && logOpt)
				fprintf(myFile,"%s %.1f\n",tarr,newTemp);
			if (log_flag)
				fprintf(stderr,"%s %.1f\n",tarr,newTemp);
			if (iflag == 0)
				iflag = 1;
			sleep(duration);
		}
	}
	return 0;
}

void * getData() {
	char buffer[1024];
	for (;;) {
		if (iflag == 0)
			continue;
		int readIn = read(STDIN_FILENO, buffer, 1024);
		if (readIn <= -1) exit(1);
		else if (readIn >= 1) {
			int i = 0;
			int given = 0;
			while (i < readIn) {
				if (buffer[i] == '\n') {
					buffer[i] = 0;
					if (strcmp(buffer + given, "OFF") == 0)
						shutDown();
					else if (strncmp(buffer + given, "PERIOD=", strlen("PERIOD=")) == 0)
						pConverter(buffer + given);
					else if (strcmp(buffer + given, "SCALE=F") == 0)
						scaleHandler('F');
					else if (strcmp(buffer + given, "SCALE=C") == 0)
						scaleHandler('C');
					else if (strcmp(buffer + given, "STOP") == 0)
						startRecording(0);
					else if (strcmp(buffer + given, "START") == 0)
						startRecording(1);
					else exit(1);
					given = i + 1;
				}
				i++;
			}
		}
		else exit(1);
	}
	exit(0);
}

int main(int argc, char **argv) {
	log_flag = 1;
	static struct option long_options[] = {
		{ "period", required_argument, NULL, 'p' },
		{ "scale", required_argument, NULL, 's' },
		{ "log", required_argument, NULL, 'l' },
		{ "id", required_argument, NULL, 'i' },
		{ "host", required_argument, NULL, 'h' },
		{ 0, 0, 0, 0 }
	};
	int opt;
	while ((opt = getopt_long(argc, argv, "p:s:l:i:h:", long_options, NULL)) >= -1) {
		if (opt == 'p')
			duration = atoi(optarg);
		else if (opt == 's') {
			switch (optarg[0]) {
			case 'C':
				scale = 'C';
				break;
			case 'F':
				scale = 'F';
				break;
			default:
				fprintf(stderr, "ERROR: incorrect scale ussage. Correct usage: --scale=[C/F]\n");
				exit(2);
			}
		}
		else if (opt == 'l') {
			FH = optarg;
			logOpt = 1;
			break;
		}
		else if (opt == 'i') {
			ID = optarg;
			IDFlag = 1;
		}
		else if (opt == 'h') {
			hostName = optarg;
			hostFlag = 1;
		}
		else {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab4b --id=[num] --host=[num] --period=[duration] --scale=[C/F] --log\n");
			exit(1);
		}
	}

	int kill = 0;

	if (hostFlag < 1 || IDFlag < 1 || logOpt < 1) {
		fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab4b --id=[num] --host=[num] --period=[duration] --scale=[C/F] --log\n");
		exit(1);
	}

	int j = optind;
	while (j < argc) {
		if (kill > 0) {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab4b --id=[num] --host=[num] --period=[duration] --scale=[C/F] --log\n");
			exit(1);
		}
		port = atoi(argv[j]);
		kill = 1;
		j++;
	}

	if (port < 0) fprintf(stderr, "ERROR: port requires a positive value.\n");

	int skt; struct sockaddr_in addy; struct hostent *server;
	skt = socket(AF_INET, SOCK_STREAM, 0);
	if (skt <= -1) {
		fprintf(stderr, "ERROR: failed to open socket.\n");
		exit(1);
	}

	server = gethostbyname(hostName);
	if (server == 0) {
		fprintf(stderr, "ERROR: failed to reach host.\n");
		exit(1);
	}

	addy.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		(char *)&addy.sin_addr.s_addr, server->h_length);
	addy.sin_port = htons(port);

	if (connect(skt, (struct sockaddr*)&addy, sizeof(addy)) <= -1) {
		fprintf(stderr, "ERROR: failed to connect.\n");
		exit(1);
	}

	if (logOpt > 0) myFile = fopen(FH, "a");

	dup2(skt, 2);
	dup2(skt, 0);
	char wID[80] = "";
	strcat(wID, "ID=");
	strcat(wID, ID);
	strcat(wID, "\n");
	fprintf(stderr, "%s", wID);
	fprintf(myFile, "%s", wID);

	tempSensor = mraa_aio_init(1);
	if (tempSensor == 0) {
		fprintf(stderr, "ERROR: failed to detect temperature sensor.\n");
		exit(1);
	}

	pthread_t *trds = malloc(sizeof(pthread_t) * 2);
	pthread_create(trds, 0, writeData, 0);
	pthread_create((1 + trds), 0, getData, 0);
	pthread_join(*(1 + trds), 0);
	pthread_join(*(1 + trds), 0);

	if (logOpt > 0) fclose(myFile);
	mraa_aio_close(tempSensor);
	free(trds);
	exit(0);
}
