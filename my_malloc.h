#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__

/* we need this *ONLY* for the size_t typedef */
#include <stdio.h>

/* our metadata structure for use with the buddy system.
 * you *MUST NOT* change this definition unless specified
 * in an official assignment update by the TAs on the newsgroup
 * '.announce'!
 */
typedef struct metadata
{
  short in_use;
  short size;
  struct metadata* next;
  struct metadata* prev;
} metadata_t;
/* This is your error enum. You must return one of these in the
 * case that a call to your functions results in an error. The four
 * different types of errors for this homework are explained below.
 * If ANY function has a case where one of the erorrs described could
 * occur, it must return the appropriate enum CASTED AS THE PROPER 
 * RETURN TYPE. In the case where a single request is too large and
 * the request results in an out of memory error as well, the 
 * SINGLE_REQUEST_TOO_LARGE should take precedence. If any of the four
 * functions complete successfully, the error code should be set to
 * NO_ERROR. */
enum my_malloc_err {
	NO_ERROR,
	OUT_OF_MEMORY,
	SINGLE_REQUEST_TOO_LARGE,
	DOUBLE_FREE_DETECTED
};
enum my_malloc_err ERRNO;

/* this function should allocate a block that is big enough to hold the
 * specified size, and that is all. if there is not a block that is able
 * to satisfy the request, then you should attempt to grab more heap 
 * space with a call to my_sbrk. if this succeeds, then you should continue
 * as normal. if it fails, then you should return NULL.
 */
void* my_malloc(size_t);

/* this function should free the block of memory, recursively merging
 * buddies up the freelist until they can be merged no more.
 */
void my_free(void *);

/* this function returns a pointer to an array of num quantity of elements 
 * each of size size*/
void* my_calloc( size_t num, size_t size);

/* this copies memory starting at the source, src and copying num_bytes
 * of bytes into the memory pointed to by dest. The source and destination
 * can indeed overlap, so the copy should be done as if the data were put into
 * a temporary buffer first and then copied. */
void* my_memmove(void* dest, const void* src, size_t num_bytes);

/* this function will emulate the system call sbrk(2). if you do not
 * have enough free heap space to satisfy a memory request, then you
 * must call this function to allocate memory to your allocator. sound
 * odd? hopefully reading the NFO file will help you understand this.
 */
void* my_sbrk(int);

/*	takes in a data size and returns the closest power of 2 higher
*	than that data between 2^4 and 2^11. ex: 254 would return 256, 
*	17 would return 32.
*/
size_t getRoundedBlockSize(size_t size);

/*	return the appropriate index of the data based on the size
*	0 = 16 1 = 32 ... 7 = 2048
*/
int getSizeIndex(size_t size);

/*
*   Manages the free list when a block is removed
*	similarly to a linked list. returns 1 if the
*	removal was successful, otherwise 0.
*/
int mitigateRemoveBlock(metadata_t* block, int index);

/*
*	handles adjusting metadata_t address locations
*	if size is negative it moves backwards in address	
*/
metadata_t* bypassFunkyPointerArith(metadata_t* address, int size);

/*
*	splits an index in the free list
*	for use in the buddy system
*/
void splitBlocks(metadata_t* block, int index);

/*
*	find the buddy associated with the give address
*	as outlined in the hw overview. based around sharing
*	the same number in bit position 3.
*/
metadata_t* getBuddy(metadata_t* startAddress);

/*
*	returns the log base two of a number
*	logb2(4) = 2, logb2(8) = 3
*/
int logb2(int number);

/*
*	swaps the order of buddies if necessary
*	if the input "smaller buddy" is actually bigger
*	based around bit positon 2 as outlined in the hw overview
*/
void assignBuddyOrder(metadata_t* smaller, metadata_t* bigger);

/*
*	turns two smallers blocks of data into one bigger one during
*	the process of freeing
*/
void merge(metadata_t* smaller, metadata_t* bigger, int index);

#endif /* __MY_MALLOC_H__ */
