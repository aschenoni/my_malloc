#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the 
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you will receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif


/* make sure this always points to the beginning of your current
 * heap space! if it does not, then the grader will not be able
 * to run correctly and you will receive 0 credit. remember that
 * only the _first_ call to my_malloc() returns the beginning of
 * the heap. sequential calls will return a pointer to the newly
 * added space!
 * Technically this should be declared static because we do not
 * want any program outside of this file to be able to access it
 * however, DO NOT CHANGE the way this variable is declared or
 * it will break the autograder.
 */
void* heap;

/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist[8];
metadata_t* offset = 0;
/**** SIZES FOR THE FREE LIST ****
 * freelist[0] -> 16
 * freelist[1] -> 32
 * freelist[2] -> 64
 * freelist[3] -> 128
 * freelist[4] -> 256
 * freelist[5] -> 512
 * freelist[6] -> 1024
 * freelist[7] -> 2048
 */


int firstCall = 1;

void* my_malloc(size_t size)
{
	//1.
	//Figure out what size block you need to satisfy user's request.
	//size field in metadata = user requested size + sizeof(metadata)
	size_t necessarySize = size + sizeof(metadata_t);
	if (necessarySize > SBRK_SIZE)
	{
		
		return NULL;
	}
	else if(size <= 0)
	{

		return NULL;
	}
	//round the size up to ensure alignment
	necessarySize = getRoundedBlockSize(necessarySize);

	//find where that size block is in the free list
	int sizeIndex = getSizeIndex(necessarySize);

	metadata_t* block = NULL;

	//check if there is a free block at the position
	if(freelist[sizeIndex] != NULL)
	{

		block = freelist[sizeIndex];
		mitigateRemoveBlock(block, sizeIndex);

		block->in_use = 1;
		metadata_t* startAddress = block;
		ERRNO = NO_ERROR;

		metadata_t* returnAddress = bypassFunkyPointerArith(startAddress, sizeof(metadata_t));
		return returnAddress;
	}

	int indexSplit = 0;
	for(int i = 8; i>sizeIndex;i--)
	{
		if(freelist[i] != NULL)
		{
			block = freelist[i];
			indexSplit = i;
		}
	}
	//allocate more heap space if necessary
	if (block == NULL)
	{
		freelist[7] = my_sbrk(SBRK_SIZE);
		if(firstCall == 1)
		{
			offset = freelist[7];
			heap = freelist[7];
			firstCall = 0;
		}
		if(freelist[7] == NULL)
		{
			ERRNO = OUT_OF_MEMORY;
			return NULL;
		}
		freelist[7]->size = SBRK_SIZE;
		freelist[7]->prev = NULL;
		freelist[7]->next = NULL;
		freelist[7]->in_use = 0;
	}
	else
	{
		splitBlocks(block,indexSplit);
	}
	block = my_malloc(size);

  return block;
}

void* my_calloc(size_t num, size_t size)
{
	char* returnAddress = my_malloc(num*size);
	if (!returnAddress)
	{
		ERRNO = SINGLE_REQUEST_TOO_LARGE;
		return NULL;
	}
	char* initializeZero = returnAddress;
	for(int i =0; i < num*size; i++)
	{
		*initializeZero = 0;
		initializeZero++;
	}
  	return (void*) returnAddress;
}

void my_free(void* ptr)
{
	if(ptr == NULL)
	{
		return;
	}
  //1.
	//calculate the proper address of the block to be freed
	metadata_t* freeAddress = bypassFunkyPointerArith( (metadata_t*) ptr, sizeof(metadata_t) *-1 );
	if(!freeAddress->in_use)
	{
		ERRNO = DOUBLE_FREE_DETECTED;
		return;
	}
	freeAddress->in_use = 0;
	//2. attempt to merge the block with its buddy if its buddy is free
	//cascade upwards until either you have just merged largest possible
	// block or an attempt to merge fails
	int index = getSizeIndex(freeAddress->size);
	metadata_t* smallerBuddy = freeAddress;
	metadata_t* buddy = getBuddy(freeAddress);
	while(!buddy->in_use && index < 7)
	{
		buddy = getBuddy(freeAddress);
		metadata_t* biggerBuddy = buddy;
		assignBuddyOrder(smallerBuddy, biggerBuddy);
		merge(smallerBuddy, biggerBuddy, index);
		freeAddress = smallerBuddy;
		index++;
	}
	ERRNO = NO_ERROR;
	freeAddress->next = freelist[index];
	if(freelist[index])
	{
		freelist[index]->prev = freeAddress;
	}
	freelist[index] = freeAddress;

}

metadata_t* getBuddy(metadata_t* startAddress) 
{
	//subtract heap offset from start address
	int address = (char*)startAddress - (char*)offset;
	metadata_t* buddy;
	if(address & startAddress->size) 
	{
		buddy = (metadata_t*) ((char*)startAddress - startAddress->size);//buddy is equal to result - size
	}
	else
	{
		buddy = (metadata_t*) ((char*)startAddress + startAddress->size);
	}
		// buddy is equal to result + size
	//re-add heap offset to buddy
	return buddy;
}

void assignBuddyOrder(metadata_t* smaller, metadata_t* bigger)
{
	int address = (char*)smaller - (char*)offset;
	if(address & smaller->size)
	{
		metadata_t* temp = smaller;
		smaller = bigger;
		bigger = temp;
	}
}

void merge(metadata_t* smaller, metadata_t* bigger, int index)
{	
	metadata_t* temp = bigger-> prev;
	if(bigger->next != NULL)
	{
		bigger->next->prev = temp;
	}
	if(temp != NULL)
	{
		temp-> next = bigger->next;
	}
	if(freelist[index] == bigger)
	{
		freelist[index] = bigger-> next;
	}
	bigger = NULL;

	smaller->size *= 2;
}

void* my_memmove(void* dest, const void* src, size_t num_bytes)
{
	char* destination = (char*) dest;
	char* source = (char*) src;

	if(src > dest)
	{
		for(int i = 0; i < num_bytes; i++)
		{
			destination[i] = source[i];
		}
	}
	else
	{
		for(int i = 0; i < num_bytes; i++)
		{
			destination[(num_bytes-1)-i] = source[(num_bytes-1)-i]; 
		}
	}

  return dest;
}

size_t getRoundedBlockSize(size_t size)
{
	size_t blockSize = 16;
	while(blockSize < size && blockSize < 2048)
	{
		blockSize *= 2;
	}
	return blockSize;
}

int getSizeIndex(size_t size)
{
	size_t startSize = 16;
	int index = 0;
	while(startSize != size)
	{
		startSize *= 2;
		index++;
	}
	return index;
}

int mitigateRemoveBlock(metadata_t* block, int index)
{
	metadata_t* temp = freelist[index];

	//check head
	if (temp == block)
	{
		freelist[index] = block->next;
		if(block->next != NULL)
		{
			block->next->prev = NULL;
		}
		block->next = NULL;
		block->prev = NULL;
		return 1;
	}

	while(temp != NULL)
	{
		if (temp == block) //find where it is in the list
		{
			if(temp->next == NULL)//if it is the tail
			{
				temp->prev->next = NULL;
				block -> next = NULL;
				block -> prev = NULL;
				return 1;
			}
			else
			{
				temp->next->prev = temp->prev;
				temp->prev->next = temp->next;
				block->next = NULL;
				block->prev = NULL;
				return 1;
			}
		}
		temp = temp-> next;
	}
	return 0;
}

metadata_t* bypassFunkyPointerArith(metadata_t* address, int size)
{
	char* tempAddress = (char*)address;
	int i = 0;
	if (size > 0)
	{
		while(i < size)
		{
			tempAddress++;
			i++;
		}
	}
	if (size < 0)
		while(i > size)
		{
			tempAddress--;
			i--;
		}

	return (metadata_t*)tempAddress;
}

void splitBlocks(metadata_t* block, int index)
{
    int size = block->size;
    char* split_address = (char*) bypassFunkyPointerArith(block, size/2);

    metadata_t* block1 = block;
    metadata_t* block2 = (metadata_t*)(split_address);
    freelist[index] = block->next;
    if(block->next != NULL)
        block->next->prev = NULL;

    freelist[index-1] = block1;
    //link block1 and block2
    block1->next = block2;
    block1->prev = NULL;
    block1->in_use = 0;
    block1->size = size/2;
    block2->next = NULL;
    block2->prev = block1;
    block2->in_use = 0;  
    block2->size = size/2;
}

int logb2(int number)
{
	int value = 0;
	while(number > 1)
	{
		number = number/2;
		value++;
	}
	return value;
}

