//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 104733125

// Implements and tests a shared variable add function, implements
// the specified command line options, and produces the specified 
// output stats.

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

long long thread_count = 1, iteration_count = 1, spin_lock = 0, ct = 0;
char sync_opt;
int yield_opt = 0;
struct timespec start_time, end_time;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void add(long long *pointer, long long value);

void *add_thread();

int main(int argc, char *argv[]) {

	static struct option long_opts[] = {
		{ "threads", required_argument, 0, 't' },
		{ "iterations", required_argument, 0, 'i' },
		{ "yield", no_argument, 0, 'y' },
		{ "sync", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	int opt = 0;
	while ((opt = getopt_long(argc, argv, "t:i:", long_opts, NULL)) > 0)
	{
		if (opt == 't') {
			thread_count = atoi(optarg);
		}
		else if (opt == 'i')
			iteration_count = atoi(optarg);
		else if (opt == 'y')
			yield_opt = 1;
		else if (opt == 's')
			if (strlen(optarg) == 1 && optarg[0] == 'm') {
				sync_opt = 'm';
			}
			else if (strlen(optarg) == 1 && optarg[0] == 's') {
				sync_opt = 's';
			}
			else if (strlen(optarg) == 1 && optarg[0] == 'c') {
				sync_opt = 'c';
			}
			else {
				fprintf(stderr, "ERROR: incorrect sync usage. Correct usage: --sync (m/s/c) \n");
				exit(1);
			}
		else {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab2_add --threads=(num) --iterations=(num) --yield --sync=(m/s/c) \n");
			exit(1);
		}
	}

	pthread_t *thread_info = malloc(thread_count * sizeof(pthread_t));

	if ((clock_gettime(CLOCK_MONOTONIC, &start_time)) == -1) {
		fprintf(stderr, "ERROR: couldn't start clock. \n");
		exit(2);
	}

	long long i = 0;
	while (i < thread_count) {
		pthread_create(&thread_info[i], NULL, add_thread, NULL);
		i++;
	}

	long long j = 0;
	while (j < thread_count) {
		pthread_join(thread_info[j], NULL);
		j++;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
		fprintf(stderr, "ERROR: couldn't stop the clock. \n");
		exit(2);
	}

	free(thread_info);
	long long total_time = 1000000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec);
	int operation_count = thread_count * iteration_count * 2;
	long long tpo = total_time / operation_count;
	long long info[6] = { thread_count, iteration_count, operation_count, total_time, tpo, ct };

	printf("add");
	if (yield_opt)
		printf("-yield");
	if (sync_opt == 'c')
		printf("-c");
	if (sync_opt == 'm')
		printf("-m");
	if (sync_opt == 's')
		printf("-s");
	if (sync_opt == 0)
		printf("-none");

	int y;
	for (y = 0; y < 6; y++) {
		printf(",%lld", info[y]);
	}
	printf("\n");

	if (ct != 0) {
		fprintf(stderr, "ERROR: counter failed.");
		exit(1);
	}
	exit(0);
}

void addf(long long *pointer, long long value) {
	// Lock
	if (sync_opt == 'm') { pthread_mutex_lock(&mutex); }
	if (sync_opt == 's') { while (__sync_lock_test_and_set(&spin_lock, 1)); }

	long long res = value + *pointer;
	if (yield_opt) { sched_yield(); }
	*pointer = res;

	if (sync_opt == 's') { __sync_lock_release(&spin_lock); }
	if (sync_opt == 'm') { pthread_mutex_unlock(&mutex); }
}

void *add_thread() {
	long long p = 0, c = 0, it = iteration_count;

	long long i;
	for (i = 1; i <= it; i++) {
		if (sync_opt != 'm' && sync_opt != 's') {
			do {
				p = ct;
				c = p+1;
				if (yield_opt > 0) { sched_yield(); }
			} while (__sync_val_compare_and_swap(&ct, p, c) != p);
		}
		else {
			addf(&ct, 1);
		}
	}

	long long j;
	for (j = 1; j <= it; j++) {
		if (sync_opt != 'm' && sync_opt != 's') {
			do {
				p = ct;
				c = p-1;
				if (yield_opt > 0)
					sched_yield();
			} while (__sync_val_compare_and_swap(&ct, p, c) != p);
		}
		else {
			addf(&ct, -1);
		}
	}
	return NULL;
}