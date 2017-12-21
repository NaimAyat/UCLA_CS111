// NAME: Naim Ayat
// EMAIL: naimayat@ucla.edu
// UID: 000000000
#include "SortedList.h"
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#define _GNU_SOURCE


int len = 0, ins = 0, del = 0, look = 0, num_iterations = 1, num_threads = 1, opt_yield = 0, num_lists = 1, num_elements = 0;
long lt = 0, lo;
char s, yout[5], end = '\0';
pthread_mutex_t * mutex;
SortedList_t * list; SortedListElement_t * elements;
int  * mlock, * hashcont;

unsigned long hash(const char * key) 
{
	unsigned long listid = 1;
	int i = 0;
	while (key[i] != end)
	{
		listid += key[i] - 'a';
		i++;
	}
	return listid % num_lists;
}

void kg()
{
	srand(time(NULL));
	int timer = 0;
	while (timer < num_elements)
	{
		int len = rand() % 5 + 18, k = rand() % 18;
		char* key = malloc((len + 1) * sizeof(char));
		int i = 0;
		while (i < len)
		{
			key[i] = 'a' + k;
			k = rand() % 18;
			i++;
		}
		key[len] = end;
		elements[timer].key = key;
		timer++;
	}

}

void locks()
{
	switch (s) {

		case 's':
			mlock = malloc(num_lists * sizeof(int));
			int i = 0;
			while (i < num_lists)
			{
				mlock[i] = 0;
				i++;
			}
			break;

		case 'm':
			mutex = malloc(num_lists * sizeof(pthread_mutex_t));
			int y = 0;
			while (y < num_lists)
			{
				pthread_mutex_init(&mutex[y], NULL);
				y++;
			}
			break;

		default:
			(void)s;
	}
}

void * threadedlist(void * thread)
{
	int bound = num_iterations * num_threads;

	for (int i = *(int *)thread; i < bound; i += num_threads)
	{
		struct timespec startA, endA;
		(void)startA; (void)endA;
		switch (s) {
			case 'm': {
				int ttracker = clock_gettime(CLOCK_MONOTONIC, &startA);
				pthread_mutex_lock(&mutex[hashcont[i]]);
				ttracker = clock_gettime(CLOCK_MONOTONIC, &endA);
				lo++;
				(void)ttracker;
				lt += 1000000000*(endA.tv_sec - startA.tv_sec) + (endA.tv_nsec - startA.tv_nsec);
				SortedList_insert(&list[hashcont[i]], &elements[i]);
				pthread_mutex_unlock(&mutex[hashcont[i]]);
				break; 
			}

			case 's': {
				int ttracker = clock_gettime(CLOCK_MONOTONIC, &startA);
				(void)ttracker;
				while (__sync_lock_test_and_set(&mlock[hashcont[i]], 1));
				ttracker = clock_gettime(CLOCK_MONOTONIC, &endA);
				lo++;
				lt += 1000000000*(endA.tv_sec - startA.tv_sec) + (endA.tv_nsec - startA.tv_nsec);
				SortedList_insert(&list[hashcont[i]], &elements[i]);
				__sync_lock_release(&mlock[hashcont[i]]);
				break;
			}

			default:
				SortedList_insert(&list[hashcont[i]], &elements[i]);
		}
	}

	for (int k = *(int *)thread; k < bound; k += num_threads)
	{
		struct timespec startA, endB;
		(void)startA; (void)endB;
		switch (s) {
		case 'm':
			pthread_mutex_lock(&mutex[hashcont[k]]);
			SortedList_delete(SortedList_lookup(&list[hashcont[k]], elements[k].key));
			pthread_mutex_unlock(&mutex[hashcont[k]]);
			break;

		case 's':
			while (__sync_lock_test_and_set(&mlock[hashcont[k]], 1));
			SortedList_delete(SortedList_lookup(&list[hashcont[k]], elements[k].key));
			__sync_lock_release(&mlock[hashcont[k]]);
			break;

		default:
			SortedList_delete(SortedList_lookup(&list[hashcont[k]], elements[k].key));
		}
	}
	int x = 1;
	(void)x;
	return (void *)1;
}

void get_yield_opts(char * opts)
{
	int i = 0;
	while ((*(opts + i)) != end)
	{
		if ((*(opts + i)) == 'i') {
			opt_yield = opt_yield | INSERT_YIELD;
			ins = 1;
		}
		else if ((*(opts + i)) == 'd') {
			opt_yield = opt_yield | DELETE_YIELD;
			del = 1;
		}
		else if ((*(opts + i)) == 'l') {
			opt_yield = opt_yield | LOOKUP_YIELD;
			look = 1;
		}
		else {
			fprintf(stderr, "ERROR: incorrect yield usage. Correct usage: --sync (i/d/l) \n");
			exit(1);
		}
		i++;
	}

	if (ins && del && look) { strcpy(yout, "idl"); }
	if (ins && del && !look) { strcpy(yout, "id"); }
	if (ins && !del && look) { strcpy(yout, "il"); }
	if (!ins && del && look) { strcpy(yout, "dl"); }
	if (ins && !del && !look) { strcpy(yout, "i"); }
	if (!ins && del && !look) { strcpy(yout, "d"); }
	if (!ins && !del && look) { strcpy(yout, "l"); }
}

int main(int argc, char ** argv)
{
	int yield_flag = 0, mutex_flag = 0, spin_flag = 0, opt = 0;
	(void)yield_flag;
	char sout[5] = "none";
	strcpy(yout, "none");
	struct timespec start, end;

	static struct option long_opts[] =
	{
		{"threads", required_argument,	NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{"lists", required_argument, NULL, 'l'}
	};


	while ((opt = getopt_long(argc, argv, "t:i:y:s:l", long_opts, NULL)) > 0)
	{
		if (opt == 't')
			num_threads = atoi(optarg);

		else if (opt == 'i')
			num_iterations = atoi(optarg);

		else if (opt == 'y') {
			get_yield_opts(optarg);
			yield_flag = 1;
		}

		else if (opt == 'l')
			num_lists = atoi(optarg);

		else if (opt == 's') {
			if (optarg[0] == 'm') {
				mutex_flag = 1;
				strcpy(sout, "m");
				s = 'm';
			}
			if (optarg[0] == 's') {
				spin_flag = 1;
				strcpy(sout, "s");
				s = 's';
			}
		}

		else {
			fprintf(stderr, "ERROR: incorrect usage. Correct usage: ./lab2_list --threads=(num) --iterations=(num) --yield --lists --sync=(m/s) \n");
			exit(1);
		}
	}


	list = malloc(num_lists * sizeof(SortedList_t));
	int i = 0;
	while (i < num_lists)
	{
		list[i].next = &list[i];
		list[i].prev = &list[i];
		list[i].key = NULL;
		i++;
	}

	elements = malloc(num_iterations * num_threads * sizeof(SortedListElement_t));
	num_elements = num_iterations * num_threads;
	kg();
	locks();
	hashcont = malloc(num_elements * sizeof(int));

	i = 0;
	while (i < num_elements) {
		hashcont[i] = hash(elements[i].key);
		i++;
	}

	pthread_t * threads = malloc(num_threads*sizeof(pthread_t));
	int * thread_id = malloc(num_threads * sizeof(int));

	if(clock_gettime(CLOCK_MONOTONIC, &start) < 0)
	{
		fprintf(stderr, "ERROR: Failed to start clock \n");
		exit(1);
	}

	int z = 0;
	while (z < num_threads)
	{
		thread_id[z] = z;
		if((pthread_create(threads + z, NULL, threadedlist, &thread_id[z]))
			|| (pthread_join(threads[z], NULL))) {
			fprintf(stderr, "ERROR: Thread creation failed \n");
			exit(1);
		}
		z++;
	}

	if(clock_gettime(CLOCK_MONOTONIC, &end) < 0) { 
		fprintf(stderr, "ERROR: Failed to keep time \n");
		exit(1);
	}

	long timer = 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	int o = 3* num_threads * num_iterations;
	long q = timer/o;
	if(mutex_flag || spin_flag)
	{
		lt = lt/lo;
	}
	fprintf(stdout,"list-%s-%s,%d,%d,%d,%d,%ld,%ld,%ld\n",yout,sout,num_threads,num_iterations,num_lists,o,timer,q,lt);
	free(hashcont);
	free(mutex);
	free(mlock);
	free(list);
	free(thread_id);
	free(elements);
	free(threads);
	if(len != 0)
		 exit(2);
	exit(0);
}