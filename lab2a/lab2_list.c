//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 104733125

// Implements the specified command line options and produces the
// specified output statistics.

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>

int opt_yield = 0, locker = 0;
pthread_mutex_t mutex;
char sync_flag = '_';

struct thread_info {
	SortedListElement_t *sortedarray;
	SortedList_t *head;
	size_t itr;
	pthread_t thread_id;
	size_t thread_number;
};

void *bthread(void *void_ptr) {
	struct thread_info ptr = *((struct thread_info*)void_ptr);
	size_t i = (ptr.thread_number)*(ptr.itr);
	while (i < (ptr.thread_number + 1)*(ptr.itr)) {
		if (sync_flag == '_') {
			SortedList_insert(ptr.head, &(ptr.sortedarray[i]));
		}
		else if (sync_flag == 'm') {
			pthread_mutex_lock(&mutex);
			SortedList_insert(ptr.head, &(ptr.sortedarray[i]));
			pthread_mutex_unlock(&mutex);
		}
		else if (sync_flag == 's') {
			while (__sync_lock_test_and_set(&locker, 1) == 1);
			SortedList_insert(ptr.head, &(ptr.sortedarray[i]));
			__sync_lock_release(&locker);
		}
		i++;
	}

	if (sync_flag == '_') {
		if (SortedList_length(ptr.head) < 0)
			exit(1);
	}
	else if (sync_flag == 'm') {
		pthread_mutex_lock(&mutex);
		if (SortedList_length(ptr.head) < 0)
			exit(1);
		pthread_mutex_unlock(&mutex);
	}
	else if (sync_flag == 's') {
		while (__sync_lock_test_and_set(&locker, 1) == 1);
		if (SortedList_length(ptr.head) < 0)
			exit(1);
		__sync_lock_release(&locker);
	}

	size_t j = (ptr.thread_number)*(ptr.itr);
	while (j < (ptr.thread_number + 1)*(ptr.itr)) {
		if (sync_flag == '_') {
			if (SortedList_lookup(ptr.head, ptr.sortedarray[j].key) == 0)
				exit(2);
			if (SortedList_delete(&(ptr.sortedarray[j])) == 1)
				exit(2);
		}
		else if (sync_flag == 'm') {
			pthread_mutex_lock(&mutex);
			if (SortedList_lookup(ptr.head, ptr.sortedarray[j].key) == 0)
				exit(2);
			if (SortedList_delete(&(ptr.sortedarray[j])) == 1)
				exit(2);
			pthread_mutex_unlock(&mutex);
		}
		else if (sync_flag == 's') {
			while (__sync_lock_test_and_set(&locker, 1) == 1);
			if (SortedList_lookup(ptr.head, ptr.sortedarray[j].key) == 0)
				exit(2);
			if (SortedList_delete(&(ptr.sortedarray[j])) == 1)
				exit(2);
			__sync_lock_release(&locker);
		}
		j++;
	}
	return NULL;
}

char* syncf() {
	char* bstr = (char*)malloc(100);
	bzero(bstr, 100);
	if (sync_flag == '-') {
		bstr[0] = 'n';
		bstr[1] = 'o';
		bstr[2] = 'n';
		bstr[3] = 'e';
	}
	else if (sync_flag == 's') {
		bstr[0] = 's';
	}
	else if (sync_flag == 'm') {
		bstr[0] = 'm';
	}
	return bstr;
}

char* optf() {
	char* ans = (char*)malloc(5 * sizeof(char));
	if (ans == 0) {
		fprintf(stderr, "ERROR: memory allocation failed. \n");
		exit(1);
	}
	bzero(ans, 5);
	int index = 0;
	if (opt_yield & INSERT_YIELD) {
		ans[index] = 'i';
		index++;
	}
	if (opt_yield & DELETE_YIELD) {
		ans[index] = 'd';
		index++;
	}
	if (opt_yield & LOOKUP_YIELD) {
		ans[index] = 'l';
		index++;
	}
	if (index == 0) {
		ans[0] = 'n';
		ans[1] = 'o';
		ans[2] = 'n';
		ans[3] = 'e';
	}
	return ans;
}

int main(int argc, char *argv[]) {
	static struct option long_options[] = {
		{ "threads", required_argument, 0, 't' },
		{ "iterations", required_argument, 0, 'i' },
		{ "yield", required_argument, 0, 'y' },
		{ "sync", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	char opt;
	unsigned int thread_count = 1;
	unsigned int iteration_count = 1;
	while ((opt = getopt_long(argc, argv, "y::s:t:i:", long_options, NULL))) {
		if (opt == -1) break;
		if (opt == 't') {
			if (optarg) thread_count = strtol(optarg, NULL, 10);
		}
		else if (opt == 'i') {
			if (optarg) iteration_count = strtol(optarg, NULL, 10);
		}
		else if (opt == 'y') {
			opt_yield = 0x00;
			for (size_t i = 0; i < strlen(optarg); i++) {
				if (optarg[i] == 'i') {
					opt_yield = 0x01 | opt_yield;
				}
				else if (optarg[i] == 'd') {
					opt_yield = 0x02 | opt_yield;
				}
				else if (optarg[i] == 'l') {
					opt_yield = 0x04 | opt_yield;
				}
				else {
					fprintf(stderr, "ERROR: incorrect yield usage. Correct usage: --yield=(i/l/d) \n");
					exit(1);
				}
			}
		}
		else if (opt == 's') {
			if (optarg) {
				if (*optarg == 'm') {
					pthread_mutex_init(&mutex, NULL);
					sync_flag = 'm';
				}
				else if (*optarg == 's') {
					sync_flag = 's';
				}
				else if (*optarg == 'c') {
					sync_flag = 'c';
				}
				else {
					fprintf(stderr, "ERROR: incorrect sync usage. Correct usage: --sync (m/s/c) \n");
					exit(1);
				}
			}
			else {
				fprintf(stderr, "ERROR: incorrect sync usage. Correct usage: --sync (m/s/c) \n");
				exit(1);
			}
		}
		else {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab2_list --threads=(num) --iterations=(num) --yield=(i/l/d) --sync=(m/s/c) \n");
			exit(1);
		}
	}

	SortedList_t *head = (SortedList_t*)malloc(sizeof(SortedList_t));
	SortedList_t temp = { head, head, NULL };
	memcpy(head, &temp, sizeof(SortedList_t));

	SortedListElement_t *sortedarray = (SortedListElement_t*)malloc(sizeof(SortedListElement_t)*(thread_count*iteration_count));
	srand(time(NULL));
	long long t = 0;
	while (t < thread_count*iteration_count) {
		char *temp_key = (char*)malloc(sizeof(char));
		*temp_key = (char)(rand() % 256);
		SortedListElement_t temp2 = { NULL, NULL, temp_key };
		memcpy(&sortedarray[t], &temp2, sizeof(SortedListElement_t));
		t++;
	}

	struct thread_info *threadi = calloc(thread_count, sizeof(struct thread_info));
	if (threadi == NULL) {
		fprintf(stderr, "ERROR: memory allocation failed. \n");
		exit(2);
	}
	char *yieldo;
	size_t threadn = 0;
	yieldo = optf();
	struct timespec start_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	while (threadn < thread_count) {
		threadi[threadn].thread_number = threadn;
		threadi[threadn].head = head;
		threadi[threadn].sortedarray = sortedarray;
		threadi[threadn].itr = iteration_count;

		int generate_res = pthread_create(&(threadi[threadn].thread_id),
			NULL,
			&bthread,
			(void*)(&threadi[threadn]));
		if (generate_res != 0)
			fprintf(stderr, "ERROR: thread creation failed. \n");
		threadn++;
	}

	threadn = 0;
	while (threadn < thread_count) {
		if (pthread_join(threadi[threadn].thread_id, NULL)) {
			fprintf(stderr, "ERROR: thread join failed. \n");
			exit(2);
		}
		threadn++;
	}

	struct timespec end_time;
	clock_gettime(CLOCK_REALTIME, &end_time);
	if (SortedList_length(head) != 0) exit(2);

	long long total_time = 1000000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec);
	char* bstr = syncf();
	fprintf(stdout, "list-%s-%s,%d,%d,1,%d,%lld,%d\n", 
			yieldo, bstr, thread_count, 
			iteration_count, 3*thread_count*iteration_count, 
			total_time, (int)(total_time / (3.0*thread_count*iteration_count)));
	free(threadi);
	exit(0);
}