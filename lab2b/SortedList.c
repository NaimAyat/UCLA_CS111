// NAME: Naim Ayat
// EMAIL: naimayat@ucla.edu
// UID: 000000000
#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>

#define _GNU_SOURCE

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	if (list == 0) return;
	if (element == 0) return;

	SortedListElement_t *curr = list->next;

	while (curr != list) {
		if (strcmp(element->key, curr->key) < 1)
			break;
		curr = curr->next;
	}

	if (opt_yield & INSERT_YIELD) sched_yield();

	element->next = curr;
	element->prev = curr->prev;
	curr->prev = element;
	(element->prev)->next = element;
}

int SortedList_delete(SortedListElement_t *element)
{
	if (element->prev == 0 ||
		(element->prev)->next == 0 ||
		(element->next)->prev == 0) {
		return 1;
	}
	if (opt_yield & DELETE_YIELD) sched_yield();
	element->prev->next = element->next;
	element->next->prev = element->prev;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	if (list == 0) { return NULL; }
	if (key == 0) { return NULL; }
	SortedListElement_t *x = list->next;
	while(x != list)
	{
		if(strcmp(x->key, key) == 0)
			return x;
		if(opt_yield & LOOKUP_YIELD)
			sched_yield();
		x = x->next;
	}
	return 0;
}

int SortedList_length(SortedList_t *list) {
	if (list == 0) return -1;
	SortedListElement_t *curr = list->next;
	int x = 0;
	while (curr != list) {
		x++;
		if (opt_yield & LOOKUP_YIELD)
			sched_yield();
		curr = curr->next;
	}
	return x;
}