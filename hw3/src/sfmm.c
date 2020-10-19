
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

static sf_block *last_block = NULL;

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
		/* header value needs to be un-xored */
		size_t curr_block_size = (curr_block->header^MAGIC) & BLOCK_SIZE_MASK;
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
		sf_block *next_block = first_block;
		/* check block size through list until it has reached beginning again */
		while(next_block->body.links.next != first_block) {
			/* header value needs to be un-xored */
			size_t next_block_size = (next_block->header^MAGIC) & BLOCK_SIZE_MASK;
			if(next_block_size >= block_size) {
				return next_block;
			}
			next_block = next_block->body.links.next;
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
	/* note: the header needs to be XORed each time it is assigned and un-XORed each
	 * time it is retrieved
	 */
	/* get block size found */
	size_t block_size_found = (block->header^MAGIC) & BLOCK_SIZE_MASK;
	/* get remainder */
	size_t remainder = block_size_found - block_size_needed;
	/*
	 * if remainder results in splinter, return found block (w/ alloc flag set)
	 * otherwise split and return new block
	 */
	if(remainder < 32) {
		debug("NO SPLIT");
		block->header = ((block->header^MAGIC) | THIS_BLOCK_ALLOCATED)^MAGIC;
		return block;
	}
	debug("SPLIT");
	/*
	 * set header of "block"
	 * adjust size and flag of (original) block passed in, in order to satify
	 * request (this will be "lower part")
	 * note: we must account for PREV_BLOCK_ALLOCATED
	 */
	int prev_alloc = (block->header^MAGIC) & PREV_BLOCK_ALLOCATED;
	block->header = (block_size_needed | THIS_BLOCK_ALLOCATED | prev_alloc)^MAGIC;
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
	debug("REMAINDER %lu", remainder);
	new_block->header = (new_block_size | PREV_BLOCK_ALLOCATED)^MAGIC;
	/* set footer of "new_block" */
	sf_block *new_block_footer = (sf_block *)((char *)new_block + new_block_size);
	/* note: no need to XOR footer bc header is already XORed */
	new_block_footer->prev_footer = new_block->header;
	/* get index from free list corresponding to block */
	size_t block_size = (new_block->header^MAGIC) & BLOCK_SIZE_MASK;
	int class_index = find_class_index(block_size);
	debug("INDEX %d", class_index);
	/* insert "remainder" block into main free lists" */
	insert_block_in_free_list(new_block, class_index);
	/* set last block to be this free block */
	last_block = block;
	return block;
}

void coalesce(sf_block *prev_block, sf_block *curr_block) {
	debug("COALESCING");
	/* check if prev block and curr block are allocated */
	int prev_block_alloc = (prev_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	int curr_block_alloc = (curr_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	/* if previous block and current block are free, coalesce current block with previous block */
	if(!prev_block_alloc && !curr_block_alloc) {
		/* remove prev_block from free lists */
		(prev_block->body.links.prev)->body.links.next = prev_block->body.links.next;
		(prev_block->body.links.next)->body.links.prev = prev_block->body.links.prev;
		/* remove curr_block from free lists */
		(curr_block->body.links.prev)->body.links.next = curr_block->body.links.next;
		(curr_block->body.links.next)->body.links.prev = curr_block->body.links.prev;
		/* get block sizes */
		size_t prev_block_size = (prev_block->header^MAGIC) & BLOCK_SIZE_MASK;
		size_t curr_block_size = (curr_block->header^MAGIC) & BLOCK_SIZE_MASK;
		/* get size of new block */
		size_t new_block_size = (prev_block_size + curr_block_size);
		/* update prev block's header */
		prev_block->header = (new_block_size | prev_block_alloc)^MAGIC;
		/* update prev block's footer */
		sf_block *new_footer = (sf_block *)((char *)prev_block + new_block_size);
		new_footer->prev_footer = prev_block->header;
		/* insert "big" block into list */
		int index = find_class_index(new_block_size);
		insert_block_in_free_list(prev_block, index);
		//sf_show_free_lists();
	} else {
		/* update last_block pointer to be new_page */
		last_block = curr_block;
	}
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
				debug("SF_ERRNO: %s\n", strerror(sf_errno));
				return NULL;
			}
			size_t init_block_size = 4080;
			init_block->header = init_block_size^MAGIC;
			/* set footer of block */
			sf_block *init_block_footer = (sf_block *)((char *)init_block + init_block_size);
			init_block_footer->prev_footer = init_block->header;
			/* link block in list */
			int init_class_index = find_class_index(init_block_size);
			debug("INDEX %d", init_class_index);
			insert_block_in_free_list(init_block, init_class_index);
			/* set the last block to be this initial block (at this point) */
			last_block = init_block;
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
		} else {
			while(1) {
				debug("BLOCK NOT FOUND, EXTENDING MEMORY");
				debug("LAST BLOCK SIZE %lu", ((last_block->header)^MAGIC) & BLOCK_SIZE_MASK);
				/* extend heap by one additional page  of memory */
				void *extend_mem = sf_mem_grow();
				/* if the allocator can't satisfy request, set errno and return NULL */
				if(extend_mem == NULL) {
					sf_errno = ENOMEM;
					debug("SF_ERRNO: %s\n", strerror(sf_errno));
					return NULL;
				}
				sf_block *new_page = (sf_block *)(extend_mem - 16);
				int prev_alloc = (last_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
				/* create new block */
				size_t new_page_size = 4096;
				new_page->header = (new_page_size | prev_alloc)^MAGIC;
				sf_block *new_page_footer = (sf_block *)((char *)new_page + new_page_size);
				new_page_footer->prev_footer = new_page->header;
				/* add new_page to free lists */
				int new_page_class_index = find_class_index(new_page_size);
				insert_block_in_free_list(new_page, new_page_class_index);
				/* coalesce if possible */
				coalesce(last_block, new_page);
				/* try to satisfy request */
				sf_block *block = find_block(alloc_block_size, class_index);
				if(block != NULL) {
					(block->body.links.prev)->body.links.next = block->body.links.next;
					(block->body.links.next)->body.links.prev = block->body.links.prev;
					block = attempt_split(last_block, alloc_block_size);
					break;
				}
			}
		}
		//sf_show_heap();
		/* return payload bc we dont want to overwrite the header */
		return block->body.payload;
	}
	//sf_show_heap();
	debug("SF_ERRNO: %s\n", strerror(sf_errno));
    return NULL;
}

void sf_free(void *pp) {
	/* verify that the pointer being passed in belongs to an allocated block */
	/* if pointer is NULL, call abort to exit program */
	if(pp == NULL)
		abort();
	/* if pointer is NOT 16-byte aligned, call abort to exit program */
	if(((unsigned long)pp & 15) != 0)
		abort();
	/* get block
	 * note: need to do -16 because sf_mallco returns the block's payload, not actual block
	 */
	sf_block *block = pp - 16;
	/* get block size */
	size_t block_size = (block->header^MAGIC) & BLOCK_SIZE_MASK;
	/* get block's footer */
	sf_block *block_footer = (sf_block *)((char *)block + block_size);
	block_footer->prev_footer = block->header;
	/* get alloc and prev_alloc bits */
	int alloc = (block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	/* if block size is valid, if not call abort to exit program */
	if(block_size < 32)
		abort();
	/* if block size is NOT a multiple of 16, call abort to exit program */
	if((block_size % 16) != 0)
		abort();
	/* if the header of the block is before the start of first block of the heap OR
	 * the footer of the block is after the end of the last block in the heap,
	 * call abort to exit programs
	 */
	if((void *)(&(block->header)) < sf_mem_start() || (void *)(&(block_footer)) > sf_mem_end())
		abort();
	/* if the allocated bit in the header is 0, call abort to exit program */
	if(alloc == 0)
		abort();
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
