
/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#define BLOCK_SIZE_MASK (~(THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED | 1))

void *search_quick_lists(size_t block_size) {
	for(int i = 0; i < NUM_QUICK_LISTS; i++) {
		// check if the list is empty, if so return NULL
		if(sf_quick_lists[i].first == NULL)
			continue;
		/**
		 * if list is not empty, get block size and check if passed in block size
		 * is equal. If so, return that block
		 */
		sf_block *curr_block = sf_quick_lists[i].first;
		size_t curr_block_size = curr_block->header&BLOCK_SIZE_MASK;
		if(curr_block_size == block_size) {
			// set "first" to point to "next" in list
			sf_quick_lists[i].first = sf_quick_lists[i].first->body.links.next;
			return curr_block;
		}
	}
	return NULL;
}

int find_class_index(size_t block_size) {
	// set minimum block size
	int M = 32;
	// if given block size is within partitioned class size, return corresponding index
	for(int i = 0; i < NUM_FREE_LISTS; i++) {
		if(block_size <= M)
			return i;
		M *= 2;
	}
	// set default value
	return NUM_FREE_LISTS-1;
}

void *search_free_lists(size_t block_size) {
	for(int i = find_class_index(block_size); i < NUM_FREE_LISTS; i++) {
		// get valid "start" block of free_list
		sf_block *first_block = sf_free_list_heads[i].body.links.next;
		// set temp var
		sf_block *temp_block = first_block;
		// check block size through list until it has reached beginning again
		while(temp_block->body.links.next != first_block) {
			size_t temp_block_size = temp_block->header&BLOCK_SIZE_MASK;
			if(temp_block_size >= block_size) {
				return temp_block;
			}
			temp_block = temp_block->body.links.next;
		}
	}
	return NULL;
}

void *sf_malloc(size_t size) {
	// if request size is not zero proceed, else return NULL
	if(size > 0) {
		/**
		 * determine ACTUAL size of block to be allocated by adding header
		 * (64 bits = 8 bytes) and necessary padding for multiple of 16 (if needed)
		 */
		size_t alloc_block_size = size + 8;
		int remainder = alloc_block_size % 16;
		if(remainder != 0)
			alloc_block_size += 16 - remainder;
		// if block size is smaller than 32, set to 32 as minimum
		if(alloc_block_size < 32)
			alloc_block_size = 32;
		// check if sf_malloc is called for the first time, if so "get" space
		if(sf_mem_start() == sf_mem_end()) {
			debug("FIRST CALL");
			sf_mem_grow();
			// first, link the lists together in order to traverse
			for(int i = 0; i < NUM_FREE_LISTS; i++) {
				sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
				sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
			}
		}
		// check the quick lists to see if they contain a block of that size
		search_quick_lists(alloc_block_size);

	}
	sf_show_heap();
    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
