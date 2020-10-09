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

void *search_quick_lists(size_t block_size) {
	sf_block *curr_block;
	for(int i = 0; i < NUM_QUICK_LISTS; i++) {
		// check if the list is empty, if so return NULL
		if(sf_quick_lists[i].first == NULL)
			break;
		/**
		 * if list is not empty, get block size and check if passed in block size
		 * is equal. If so, return that block
		 */
		curr_block = sf_quick_lists[i].first;
		size_t header = sf_quick_lists[i].first->header;
		header = header&0x8;
		if(header == block_size) {
			// set "first" to point to "next" in list
			sf_quick_lists[i].first = sf_quick_lists[i].first->body.links.next;
			return curr_block;
		}
	}
	return NULL;
}

void *search_free_lists(size_t block_size) {
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
		int first_call_flag = 0;
		if(!first_call_flag) {
			sf_mem_grow();
			first_call_flag = 1;
		}
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
