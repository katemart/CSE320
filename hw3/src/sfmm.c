
/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#define BLOCK_SIZE_MASK (~(THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED | 1))

void *search_quick_lists(size_t block_size) {
	debug("SEARCH QUICK LISTS");
	for(int i = 0; i < NUM_QUICK_LISTS; i++) {
		/* check if the list is empty, if so return NULL */
		if(sf_quick_lists[i].first == NULL)
			continue;
		/*
		 * if list is not empty, get block size and check if passed in block size
		 * is equal. If so, return that block
		 */
		sf_block *curr_block = sf_quick_lists[i].first;
		size_t curr_block_size = curr_block->header&BLOCK_SIZE_MASK;
		if(curr_block_size == block_size) {
			/* set "first" to point to "next" in list */
			sf_quick_lists[i].first = sf_quick_lists[i].first->body.links.next;
			return curr_block;
		}
	}
	return NULL;
}

int find_class_index(size_t block_size) {
	debug("FIND CLASS INDEX");
	/* set minimum block size */
	int M = 32;
	/* if given block size is within partitioned class size, return corresponding index */
	for(int i = 0; i < NUM_FREE_LISTS; i++) {
		if(block_size <= M)
			return i;
		M *= 2;
	}
	/* set default index value */
	return NUM_FREE_LISTS-1;
}

void *search_free_lists(size_t block_size, int class_index) {
	debug("SEARCH FREE LISTS");
	/* search free lists, starting from the list for the determined size class */
	for(int i = class_index; i < NUM_FREE_LISTS; i++) {
		/* get valid "start" block of free_list */
		sf_block *first_block = sf_free_list_heads[i].body.links.next;
		/* set temp var */
		sf_block *temp_block = first_block;
		/* check block size through list until it has reached beginning again */
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

void *find_block(size_t block_size, int class_index) {
	debug("FINDING BLOCK IN LISTS");
	/* first search through quick lists */
	sf_block *block = search_quick_lists(block_size);
	/* if a block is not found in quick lists, search through free lists */
	if(block == NULL) {
		block = search_free_lists(block_size, class_index);
	}
	return block;
}

void insert_block_in_free_list(sf_block *block, int class_index) {
	debug("INSERTING BLOCK INTO FREE LIST");
	/* set dummy sentinel node */
	sf_block *sentinel_node = &sf_free_list_heads[class_index];
	/* set block's prev (i.e. dummy node) and next (i.e. block after dummy node)
	 * (this sets block as first in list after sentinel node)
	 */
	block->body.links.next = sentinel_node->body.links.next;
	block->body.links.prev = sentinel_node;
	/* set sentinel node's prev and next
	 * note: sentinel node's previous is the same as sentinel node's next's previous
	 * (this has to be called before overwritting it)
	 */
	(sentinel_node->body.links.next)->body.links.prev = block;
	sentinel_node->body.links.next = block;
}

void *attempt_split(sf_block *block, size_t block_size_needed) {
	/* get block size found */
	size_t block_size_found = block->header&BLOCK_SIZE_MASK;
	/* get remainder */
	size_t remainder = block_size_found - block_size_needed;
	/*
	 * if remainder results in splinter, return found block (w/ alloc flag set)
	 * otherwise split and return new block
	 */
	if(remainder < 32) {
		debug("NO SPLIT");
		block->header = block->header | THIS_BLOCK_ALLOCATED;
		return block;
	}
	debug("SPLIT");
	/*
	 * set header of "block"
	 * adjust size and flag of (original) block passed in, in order to satify
	 * request (this will be "lower part")
	 * note: we must account for PREV_BLOCK_ALLOCATED
	 */
	int prev_alloc = block->header & PREV_BLOCK_ALLOCATED;
	block->header = block_size_needed | THIS_BLOCK_ALLOCATED | prev_alloc;
	/*
	 * get address of remainder and put into new block (this will be "upper part")
	 * note: convert header pointer to char* in order to do ptr arithmetic
	 * (without having to divide by data type size)
	 */
	sf_block *new_block = (sf_block *)((char *)block + block_size_needed);
	/*
	 * set header of "new block"
	 * note: the prev block is "block", in which case prev_alloc flag is on
	 */
	size_t new_block_size = remainder;
	new_block->header = new_block_size | PREV_BLOCK_ALLOCATED;
	/* set footer of "new_block" */
	sf_block *new_block_footer = (sf_block *)((char *)new_block + new_block_size);
	new_block_footer->prev_footer = new_block->header ^ MAGIC;
	/* get index from free list corresponding to block */
	size_t block_size = new_block->header&BLOCK_SIZE_MASK;
	int class_index = find_class_index(block_size);
	debug("%d", class_index);
	/* insert "remainder" block into main free lists" */
	insert_block_in_free_list(new_block, class_index);
	//sf_show_free_lists();
	return block;
}

void *sf_malloc(size_t size) {
	debug("MAGIC %lu\n", sf_magic());
	/* if request size is not zero proceed, else return NULL */
	if(size > 0) {
		/*
		 * determine ACTUAL size of block to be allocated by adding header
		 * (64 bits = 8 bytes) and necessary padding for multiple of 16 (if needed)
		 */
		size_t alloc_block_size = size + 8;
		int remainder = alloc_block_size % 16;
		if(remainder != 0)
			alloc_block_size += 16 - remainder;
		/* if block size is smaller than 32, set to 32 as minimum */
		if(alloc_block_size < 32)
			alloc_block_size = 32;
		/* check if sf_malloc is called for the first time, if so "get" space */
		if(sf_mem_start() == sf_mem_end()) {
			debug("FIRST CALL");
			/* link the lists together in order to traverse */
			for(int i = 0; i < NUM_FREE_LISTS; i++) {
				sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
				sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
			}
			/* initialize for first allocation */
			sf_block *init_block = sf_mem_grow();
			if(init_block == NULL) {
				sf_errno = ENOMEM;
				return NULL;
			}
			size_t init_block_size = 4080;
			init_block->header = init_block_size;
			/* set footer of block */
			sf_block *init_block_footer = (sf_block *)((char *)init_block + init_block_size);
			init_block_footer->prev_footer = init_block->header ^ MAGIC;
			/* link block in list */
			int init_class_index = find_class_index(init_block_size);
			debug("%d", init_class_index);
			insert_block_in_free_list(init_block, init_class_index);
			//sf_show_free_lists();
		}
		/* find what class index from free-list the size belongs to */
		int class_index = find_class_index(alloc_block_size);
		/* check the quick and free lists to see if they contain a block of that size
		 * if they do not, then use sf_mem_grow to request more memory
		 */
		sf_block *block = find_block(alloc_block_size, class_index);
		/* if block is found, split if necessary
		  * else, if block is not found keep growing memory until found or we run out of mem
		 */
		if(block != NULL) {
			/* if block is found in lists, remove block from lists
			 * this is done by setting the block's prev's next to block's next
			 * and setting the block's next's prev to block's prev
			 */
			(block->body.links.prev)->body.links.next = block->body.links.next;
			(block->body.links.next)->body.links.prev = block->body.links.prev;
			/* now try to split (if possible) */
			block = attempt_split(block, alloc_block_size);
			/* XOR header w MAGIC to un-obfuscate */
			block->header ^= MAGIC;
		} else {
			/** TO DO **/
			debug("BLOCK NOT FOUND, EXTENDING MENORY");
			while(1) {
				sf_block *new_block = sf_mem_grow();
				/* if the allocator can't satisfy request, set errno and return NULL */
				if(new_block == NULL) {
					sf_errno = ENOMEM;
					return NULL;
				}
			}
		}
		/* return payload bc we dont want to overwrite the header */
		//sf_show_heap();
		//sf_show_free_lists();
		return block->body.payload;
	} else if(size < 0) {
		sf_errno = ENOMEM;
	}
	//sf_show_heap();
    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
