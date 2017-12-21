//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000

#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <mraa.h>
#include <aio.h>

int log_flag, iflag = 0, scale = 'F', logOpt = 0, rep = 1;
long duration = 1;
FILE *myFile;
char *myFH;
mraa_gpio_context buttonSensor;
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

void scaleHandler(char flg){
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

void pConverter(char *buffer){
    char *s = buffer + strlen("PERIOD="), *e;
    duration = strtol(s, &e, 10);
    if(logOpt)
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

void * checkButton() {
	for (;;) {
		if (mraa_gpio_read(buttonSensor)) {
			char tarr[50];
			convertTime(tarr);
			if (logOpt)
				fprintf(myFile, "%s %s\n", tarr, "SHUTDOWN");
			exit(0);
		}
	}
	return 0;
}

void * logTemperature() {
	for (;;) {
		if (rep > 0) {
			float newTemp = tempConverter(mraa_aio_read(tempSensor));
			if (scale == 'F')
				newTemp = 32 + (1.8*newTemp);
			char tarr[50];
			convertTime(tarr);
			if (log_flag && logOpt)
				fprintf(myFile, "%s %.1f\n", tarr, newTemp);
			if (log_flag)
				fprintf(stdout, "%s %.1f \n", tarr, newTemp);
			if (iflag == 0)
				iflag = 1;
			sleep(duration);
		}
	}
	return 0;
}

void * readInput(){
    char buffer[1024];
    for(;;) {
		if (iflag == 0)
			continue;
        int readIn = read(STDIN_FILENO, buffer, 1024); 
        if(readIn <= -1) exit(1); 
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
	static struct option long_opts[] = {
		{"period", required_argument, NULL, 'p'},
		{"scale", required_argument, NULL, 's'},
		{"log", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};
	int opt;
	while ((opt = getopt_long(argc, argv, "p:s:l:", long_opts, NULL)) >= 0) {
		if (opt == 'p') {
			duration = atoi(optarg);
		}
		else if (opt == 's') {
			switch (optarg[0]) {
			case 'C':
				scale = 'C';
				break;
			case 'F':
				scale = 'F';
				break;
			default:
				fprintf(stderr, "ERROR: incorrect scale ussage. Correct usage: --scale=(C/F)\n");
				exit(2);
			}
		}
		else if (opt == 'l') {
			myFH = optarg;
			logOpt = 1;
		}
		else {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab4b --period=(duration) --scale=(C/F) --log\n");
			exit(1);
		}
	}
	if (logOpt > 0)
		myFile = fopen(myFH, "a");
	tempSensor = mraa_aio_init(1);
	if (tempSensor == NULL) {
		fprintf(stderr, "ERROR: failed to detect temperature sensor\n");
		exit(1);
	}
	buttonSensor = mraa_gpio_init(73);
	if (buttonSensor == NULL) {
		fprintf(stderr, "ERROR: failed to detect button\n");
		exit(1);
	}
	pthread_t *trd = malloc(sizeof(pthread_t)*3);
	pthread_create(trd, 0, logTemperature, 0);
	pthread_create(trd + 1, 0, checkButton, 0);
	pthread_create(trd + 2, 0, readInput, 0);
	pthread_join(*(trd), 0);
	pthread_join(*(trd + 1), 0);
	pthread_join(*(trd + 2), 0);
	free(trd);
	if (logOpt > 0)
		fclose(myFile);
	mraa_gpio_close(buttonSensor);
	mraa_aio_close(tempSensor);
	exit(0);
}