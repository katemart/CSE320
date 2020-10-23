
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
size_t header_size = sizeof(sf_header);
size_t footer_size = sizeof(sf_footer);

int find_class_index_quick_lists(size_t block_size) {
	debug("FIND CLASS INDEX QUICK LISTS");
	for(int i = 0; i < NUM_QUICK_LISTS; i++) {
		/* if given block size is within partitioned class size, return corresponding index */
		return ((block_size - 32) / 16);
	}
	return -1;
}

void *search_quick_lists(size_t block_size) {
	debug("SEARCH QUICK LISTS");
	for(int i = 0; i < NUM_QUICK_LISTS; i++) {
		/* get current block */
		sf_block *curr_block = sf_quick_lists[i].first;
		/* check if the list is empty, if so return NULL */
		if(curr_block == NULL)
			continue;
		/*
		 * if list is not empty, get block size and check if passed in block size
		 * is equal. If so, return that block
		 */
		/* header value needs to be un-xored */
		size_t curr_block_size = (curr_block->header^MAGIC) & BLOCK_SIZE_MASK;
		if(curr_block_size == block_size) {
			/* set "first" to point to "next" in list */
			curr_block = curr_block->body.links.next;
			return curr_block;
		}
	}
	return NULL;
}

int find_class_index_free_lists(size_t block_size) {
	debug("FIND CLASS INDEX FREE LISTS");
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
	debug("BLOCK SIZE SPLIT %lu", (block->header^MAGIC) & BLOCK_SIZE_MASK);
	int class_index = find_class_index_free_lists(block_size);
	debug("INDEX %d", class_index);
	/* insert "remainder" block into main free lists" */
	insert_block_in_free_list(new_block, class_index);
	/* set last block to be this free block */
	last_block = block;
	return block;
}

void coalesce(sf_block *prev_block, sf_block *curr_block) {
	debug("COALESCING");
	/* var to keep track of when coalescing is performed */
	int coalesce = 0;
	/* check if prev block and curr block are allocated */
	int prev_block_alloc = (prev_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	int curr_block_alloc = (curr_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	/* if previous block and current block are free, coalesce current block with previous block */
	if(!prev_block_alloc && !curr_block_alloc) {
		debug("ABLE TO COALESCE");
		/* set coalesce to true */
		coalesce = 1;
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
		int prev_block_prev_alloc = (prev_block->header^MAGIC) & PREV_BLOCK_ALLOCATED;
		prev_block->header = (new_block_size | prev_block_prev_alloc)^MAGIC;
		/* update prev block's footer */
		sf_block *next_block = (sf_block *)((char *)prev_block + new_block_size);
		next_block->prev_footer = prev_block->header;
		/* update next block's header (pal bit) */
		if((void *)(&(next_block->header)) < sf_mem_end() - header_size) {
			int curr_alloc = (prev_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
			int next_block_alloc = (next_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
			int next_block_size = (next_block->header^MAGIC) & BLOCK_SIZE_MASK;
			next_block->header = (next_block_size | next_block_alloc | curr_alloc)^MAGIC;
		}
		/* insert "big" block into list */
		int index = find_class_index_free_lists(new_block_size);
		insert_block_in_free_list(prev_block, index);
	}
	/* use coalesce var to set "last block" in heap accordingly */
	if(prev_block == last_block && !coalesce) {
		/* update last_block pointer to be new_page */
		last_block = curr_block;
	} else if(curr_block == last_block && coalesce) {
		/* update last_block pointer to be prev_block */
		last_block = prev_block;
	}
}

void *sf_malloc(size_t size) {
	debug("\n\nSF_MALLOC");
	/* if request size is not zero proceed, else return NULL */
	if(size > 0) {
		/*
		 * determine ACTUAL size of block to be allocated by adding header
		 * (64 bits = 8 bytes) and necessary padding for multiple of 16 (if needed)
		 */
		size_t alloc_block_size = size + header_size;
		int remainder = alloc_block_size % 16;
		if(remainder != 0)
			alloc_block_size += 16 - remainder;
		/* if block size is smaller than 32, set to 32 as minimum */
		if(alloc_block_size < 32)
			alloc_block_size = 32;
		/* if the alloc_block_size is less than the passed in size it could indicate overflow
		 * if overflow is present, we have a neg num - in which case return NULL and set sf_errno
		 */
		if(size > alloc_block_size) {
			sf_errno = ENOMEM;
			debug("OVERFLOW!");
			return NULL;
		}
		/* check if sf_malloc is called for the first time, if so "get" space */
		if(sf_mem_start() == sf_mem_end()) {
			debug("FIRST CALL SF_MALLOC");
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
			size_t init_block_size = PAGE_SZ - header_size - footer_size;
			init_block->header = init_block_size^MAGIC;
			/* set footer of block */
			sf_block *init_block_footer = (sf_block *)((char *)init_block + init_block_size);
			init_block_footer->prev_footer = init_block->header;
			/* link block in list */
			int init_class_index = find_class_index_free_lists(init_block_size);
			debug("INDEX %d", init_class_index);
			insert_block_in_free_list(init_block, init_class_index);
			/* set the last block to be this initial block (at this point) */
			last_block = init_block;
		}
		/* find what class index from free-list the size belongs to */
		int class_index = find_class_index_free_lists(alloc_block_size);
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
				sf_block *new_page = (sf_block *)(extend_mem - header_size - footer_size);
				int prev_alloc = (last_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
				/* create new block */
				size_t new_page_size = PAGE_SZ;
				new_page->header = (new_page_size | prev_alloc)^MAGIC;
				sf_block *new_page_footer = (sf_block *)((char *)new_page + new_page_size);
				new_page_footer->prev_footer = new_page->header;
				/* add new_page to free lists */
				int new_page_class_index = find_class_index_free_lists(new_page_size);
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

void flush_quick_list(int class_index) {
	//sf_show_heap();
	debug("QUICK LIST LENGTH %d", sf_quick_lists[class_index].length);
	sf_block *temp = NULL;
	while(sf_quick_lists[class_index].length > 0) {
		/* get first (aka start of list) */
		sf_block *curr_block = sf_quick_lists[class_index].first;
		/* get next block in heap */
		int block_size = (curr_block->header^MAGIC) & BLOCK_SIZE_MASK;
		sf_block *next_block = (sf_block *)((char *)curr_block + block_size);
		sf_block *prev_block = NULL;
		int prev_alloc = (curr_block->header^MAGIC) & PREV_BLOCK_ALLOCATED;
		if(prev_alloc == 0) {
			if((void *)(&(curr_block->prev_footer)) > sf_mem_start()) {
				/* get previous block in heap */
				size_t prev_block_size = (curr_block->prev_footer^MAGIC) & BLOCK_SIZE_MASK;
				/* note: &block->prev_footer == block */
				prev_block = (sf_block *)((char *)curr_block - prev_block_size);
			}
		}
		/* delete block from quick list */
		if(curr_block != NULL) {
			/* if block's next is not empty, set head to that and delete node
			 * otherwise, set head to NULL
			 */
			if(curr_block->body.links.next != NULL) {
				temp = curr_block;
				sf_quick_lists[class_index].first = curr_block->body.links.next;
				temp->body.links.next = NULL;
			} else {
				sf_quick_lists[class_index].first = NULL;
			}
			/*update list length */
			sf_quick_lists[class_index].length--;
			debug("QUICK LIST LENGTH %d", sf_quick_lists[class_index].length);
			/* update block alloc bit to free */
			curr_block->header = (block_size | 0 | prev_alloc)^MAGIC;
			/* update footer */
			next_block->prev_footer = curr_block->header;
			/* update next block's header (pal bit) */
			if((void *)(&(next_block->header)) < sf_mem_end() - header_size) {
				int curr_alloc = (curr_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
				int next_block_alloc = (next_block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
				int next_block_size = (next_block->header^MAGIC) & BLOCK_SIZE_MASK;
				next_block->header = (next_block_size | next_block_alloc | curr_alloc)^MAGIC;
			}
			/* worthless to update footer, since if it is a free block it'll be coalesced anyways */
			debug("QUICK LIST PREV_ALLOC %d", prev_alloc);
			debug("QUICK LIST ALLOC %lu", ((curr_block->header^MAGIC) & THIS_BLOCK_ALLOCATED));
			/* add block to free list */
			int free_list_index = find_class_index_free_lists(block_size);
			insert_block_in_free_list(curr_block, free_list_index);
			/* try to coalesce with next first */
			if(next_block != NULL && (void *)next_block < sf_mem_end() - header_size) {
				debug("COALESCING WITH NEXT");
				coalesce(curr_block, next_block);
			}
			//debug("IS PREV NULL %d", prev_block==NULL);
			/* now try to coalesce with previous */
			if(prev_block != NULL && (void *)prev_block > sf_mem_start()) {
				debug("COALESCING WITH PREVIOUS");
				coalesce(prev_block, curr_block);
			}
		}
	}
}

void insert_block_in_quick_list(sf_block *block, int class_index) {
	debug("INSERTING BLOCK INTO QUICK LIST");
	debug("QUICK LIST LENGTH %d", sf_quick_lists[class_index].length);
	/* first, try to insert into designated list if capacity hasn't been reached */
	if(sf_quick_lists[class_index].length < QUICK_LIST_MAX) {
		debug("CAPACITY NOT REACHED. ADDING BLOCK TO QUICK LIST");
		sf_block *head = sf_quick_lists[class_index].first;
		/* create new block */
		sf_block *new_block = block;
		/* get head of list */
		new_block->body.links.next = head;
		/* set passed in block as new head */
		sf_quick_lists[class_index].first = new_block;
		/* update list's length */
		sf_quick_lists[class_index].length++;
	} else {
		debug("CAPACITY REACHED. FLUSHING LIST");
		/* clear full quick list */
		flush_quick_list(class_index);
		/* insert block into (now) flushed list so that there is only one block in list */
		sf_block *head = sf_quick_lists[class_index].first;
		sf_block *new_block = block;
		new_block->body.links.next = head;
		sf_quick_lists[class_index].first = new_block;
		sf_quick_lists[class_index].length++;
	}
	debug("QUICK LIST LENGTH %d", sf_quick_lists[class_index].length);
}

void sf_free(void *pp) {
	//sf_show_heap();
	debug("\n\nFREEING BLOCK");
	/* verify that the pointer being passed in belongs to an allocated block */
	/* if pointer is NULL, call abort to exit program */
	if(pp == NULL)
		abort();
	/* if pointer is NOT 16-byte aligned, call abort to exit program */
	if(((unsigned long)pp & 15) != 0)
		abort();
	/* get block
	 * note: need to do -16 because sf_malloc returns the block's payload, not actual block
	 */
	sf_block *block = pp - header_size - footer_size;
	/* get block size */
	size_t block_size = (block->header^MAGIC) & BLOCK_SIZE_MASK;
	debug("%lu", block_size);
	/* if block size is valid, if not call abort to exit program */
	if(block_size < 32)
		abort();
	/* if block size is NOT a multiple of 16, call abort to exit program */
	if((block_size % 16) != 0)
		abort();
	/* get block's footer */
	sf_block *block_footer = (sf_block *)((char *)block + block_size);
	block_footer->prev_footer = block->header;
	/* if the header of the block is before the start of first block of the heap OR
	 * the footer of the block is after the end of the last block in the heap,
	 * call abort to exit programs
	 */
	if((void *)(&(block->header)) < sf_mem_start() || (void *)(block_footer) > sf_mem_end())
		abort();
	/* get alloc and prev_alloc bits */
	int alloc = (block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	int prev_alloc = (block->header^MAGIC) & PREV_BLOCK_ALLOCATED;
	/* if the allocated bit in the header is 0, call abort to exit program
	 * (since we can't "free" an un-allocated block)
	 */
	//debug("ALLOC %d", alloc);
	if(alloc == 0)
		abort();
	/* if the prev_alloc field is zero but the alloc field in the prev block is not zero,
	 * call abort to exit program
	 * (as something must've gone wrong because these fields MUST ALWAYS match)
	 */
	//debug("PREV ALLOC %d", prev_alloc);
	if(prev_alloc == 0) {
		/* if the previous block is not allocated (i.e. it is free), it has a footer
		 * so we can read the footer (i.e, block's prev_footer) to check the prev_alloc bit
		 * note: &block->prev_footer == block
		 */
		 if((void *)(&(block->prev_footer)) > sf_mem_start()) {
		 	int prev_block_alloc = (block->prev_footer^MAGIC) & THIS_BLOCK_ALLOCATED;
			if(prev_block_alloc != 0)
				abort();
		 }
	}
	/* if all of the above conditions pass, proceed to free block */
	/* first, try to insert at the front of the quick list of the appropriate size */
	int quick_list_max_block = ((NUM_QUICK_LISTS-1)*16) + 32;
	if(block_size <= quick_list_max_block) {
		debug("FREE FROM QUICK LISTS");
		int quick_list_class_index = find_class_index_quick_lists(block_size);
		insert_block_in_quick_list(block, quick_list_class_index);
	} else {
		debug("FREE FROM FREE LISTS");
		/* update block alloc bit to free */
		//debug("%lu", block->header^MAGIC);
		block->header = (block_size | 0 | prev_alloc)^MAGIC;
		//debug("NEW HEADER %lu", block->header^MAGIC);
		/* update footer with un-alloc bit */
		block_footer->prev_footer = block->header;
		/* update next block's header (pal bit) */
		if((void *)(&(block_footer->header)) < sf_mem_end() - header_size) {
			int curr_alloc = (block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
			debug("CURR_ALLOC %d", curr_alloc);
			int next_block_alloc = (block_footer->header^MAGIC) & THIS_BLOCK_ALLOCATED;
			int next_block_size = (block_footer->header^MAGIC) & BLOCK_SIZE_MASK;
			block_footer->header = (next_block_size | next_block_alloc | curr_alloc)^MAGIC;
		}
		/* add block to free list */
		int free_list_index = find_class_index_free_lists(block_size);
		insert_block_in_free_list(block, free_list_index);
		/* get next block in heap */
		sf_block *next_block = (sf_block *)((char *)block + block_size);
		/* try to coalesce with next first */
		if(next_block != NULL && (void *)next_block < sf_mem_end() - header_size) {
			debug("COALESCING WITH NEXT");
			coalesce(block, next_block);
		}
		/* now try to coalesce with previous (if any) */
		if(prev_alloc == 0 && (void *)(&(block->prev_footer)) > sf_mem_start()) {
			/* get previous block in heap */
			size_t prev_block_size = (block->prev_footer^MAGIC) & BLOCK_SIZE_MASK;
			/* note: &block->prev_footer == block */
			sf_block *prev_block = (sf_block *)((char *)block - prev_block_size);
			/* try to coalesce */
			if(prev_block != NULL && (void *)prev_block > sf_mem_start()) {
				debug("COALESCING WITH PREVIOUS");
				coalesce(prev_block, block);
			}
		}
	}
	sf_show_heap();
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
	//sf_show_heap();
	debug("\nRE-ALLOCATING BLOCK");
	/* verify that the pointer being passed in belongs to an allocated block */
	/* if pointer is NULL, set sf_errno to EINVAL and return NULL */
	if(pp == NULL) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* if pointer is NOT 16-byte aligned, set sf_errno to EINVAL and return NULL */
	if(((unsigned long)pp & 15) != 0) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* get block
	 * note: need to do -16 because sf_malloc returns the block's payload, not actual block
	 */
	sf_block *block = pp - header_size - footer_size;
	/* get block size */
	size_t block_size = (block->header^MAGIC) & BLOCK_SIZE_MASK;
	/* if block size is valid, set sf_errno to EINVAL and return NULL */
	if(block_size < 32) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* if block size is NOT a multiple of 16, set sf_errno to EINVAL and return NULL */
	if((block_size % 16) != 0) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* get block's footer */
	sf_block *block_footer = (sf_block *)((char *)block + block_size);
	block_footer->prev_footer = block->header;
	/* if the header of the block is before the start of first block of the heap OR
	 * the footer of the block is after the end of the last block in the heap,
	 * set sf_errno to EINVAL and return NULL
	 */
	if((void *)(&(block->header)) < sf_mem_start() || (void *)(block_footer) > sf_mem_end()) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* get alloc and prev_alloc bits */
	int alloc = (block->header^MAGIC) & THIS_BLOCK_ALLOCATED;
	int prev_alloc = (block->header^MAGIC) & PREV_BLOCK_ALLOCATED;
	/* if the allocated bit in the header is 0, set sf_errno to EINVAL and return NULL
	 * (since we can't "re-allocate" an un-allocated block)
	 */
	if(alloc == 0) {
		sf_errno = EINVAL;
		return NULL;
	}
	/* if the prev_alloc field is zero but the alloc field in the prev block is not zero,
	 * set sf_errno to EINVAL and return NULL
	 * (as something must've gone wrong because these fields MUST ALWAYS match)
	 */
	if(prev_alloc == 0) {
		/* if the previous block is not allocated (i.e. it is free), it has a footer
		 * so we can read the footer (i.e, block's prev_footer) to check the prev_alloc bit
		 * note: &block->prev_footer == block
		 */
		 if((void *)(&(block->prev_footer)) > sf_mem_start()) {
		 	int prev_block_alloc = (block->prev_footer^MAGIC) & THIS_BLOCK_ALLOCATED;
			if(prev_block_alloc != 0) {
				sf_errno = EINVAL;
				return NULL;
			}
		 }
	}
	/* if pointer is valid but the size parameter is 0, free the block and return NULL */
	if(rsize == 0) {
		sf_free(pp);
		return NULL;
	}
	/* if all of the above conditions pass, proceed to re-alloc block */
	/* re-allocating to a larger size */
	if(block_size < rsize) {
		void *larger_block = sf_malloc(rsize);
		if(larger_block != NULL) {
			memcpy(larger_block, block->body.payload, (block_size - header_size));
		}
		sf_free(pp);
		return larger_block;
	} else if(block_size > rsize) {
		/* re-allocating to a smaller size */
		rsize = rsize + header_size;
		int remainder = rsize % 16;
		if(remainder != 0)
			rsize += 16 - remainder;
		block = attempt_split(block, rsize);
		debug("B SIZE %lu",  (block->header^MAGIC) & BLOCK_SIZE_MASK);
		return block->body.payload;
	} else {
		/* block size and rsize are equal */
		return pp;
	}
    return NULL;
}
