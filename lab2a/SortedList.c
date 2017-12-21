//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000

/**
* SortedList (and SortedListElement)
*
*A doubly linked list, kept sorted by a specified key.
*This structure is used for a list head, and each element
*of the list begins with this structure.
*
*The list head is in the list, and an empty list contains
*only a list head.  The list head is also recognizable because
*it has a NULL key pointer.
*/

#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>

/**
* SortedList_insert ... insert an element into a sorted list
*
*The specified element will be inserted in to
*the specified list, which will be kept sorted
*in ascending order based on associated keys
*
* @param SortedList_t *list ... header for the list
* @param SortedListElement_t *element ... element to be added to the list
*/

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
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


/**
* SortedList_delete ... remove an element from a sorted list
*
*The specified element will be removed from whatever
*list it is currly in.
*
*Before doing the deletion, we check to make sure that
*next->prev and prev->next both point to this node
*
* @param SortedListElement_t *element ... element to be removed
*
* @return 0: element deleted successfully, 1: corrtuped prev/next pointers
*
*/

int SortedList_delete(SortedListElement_t *element) {
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



/**
* SortedList_lookup ... search sorted list for a key
*
*The specified list will be searched for an
*element with the specified key.
*
* @param SortedList_t *list ... header for the list
* @param const char * key ... the desired key
*
* @return pointer to matching element, or NULL if none is found
*/

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if(list == 0) return NULL;
  if (key == 0) return NULL;
  SortedListElement_t *curr = list->next;
  while(curr != list) {
    if(strcmp(curr->key, key) < 1)
		return curr;
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    curr = curr->next;
  }
  return NULL;
}


/**
* SortedList_length ... count elements in a sorted list
*While enumeratign list, it checks all prev/next pointers
*
* @param SortedList_t *list ... header for the list
*
* @return int number of elements in list (excluding head)
*   -1 if the list is corrupted
*/

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

