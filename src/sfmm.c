#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../include/sfmm.h"
#include "../include/myownheader.h"
#include <errno.h>

#define BLOCKSIZE 32 //bytes
#define LONGSIZE 16
#define HEADERSIZE 8
#define FOOTERSIZE 8



/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

 sf_free_header* freelist_head = NULL;
 sf_footer* footOfPage = NULL;

 void* lowerBound = 0;
 void* upperBound = 0;

 size_t space = 4096;

 void* startOfHeap;
 int emptyList = 0;
 int freeFlag = 0;
 //int coalescePage = 0;

 static size_t numFrees = 0;
 static size_t internal = 0;
 static size_t external = 0;
 static size_t numAllocations = 0;
 static size_t numCoalesce = 0;


 int sbrkCount = 0;


//padding only for allocated blocks

/**
* This is your implementation of malloc. It creates dynamic memory which
* is aligned and padded properly for the underlying system. This memory
* is uninitialized.
* @param size The number of bytes requested to be allocated.
* @return If successful, the pointer to a valid region of memory to use is
* returned, else the value NULL is returned and the ERRNO is set  accordingly.
* If size is set to zero, then the value NULL is returned.
*/


void *sf_malloc(size_t size){
	size_t adjustedSize = 32;
	size_t paddingSize = 0;
	


	if (size == 0)
		return NULL;

	if (size > (4096 * 4))
		errno = ENOMEM;

	if (freelist_head == NULL){
		//freelist_head = sf_sbrk(0);*
		startOfHeap = sf_sbrk(0);
		sf_sbrk(1);
		sbrkCount++;

		freelist_head = startOfHeap;
		freelist_head -> next = NULL;
		freelist_head -> prev = NULL;

		freelist_head -> header.block_size = 4096 >> 4;
		freelist_head -> header.alloc = 0;

		lowerBound = startOfHeap;
		upperBound = lowerBound + 4096;

		footOfPage = upperBound - 8;
		footOfPage -> alloc = 0;
		footOfPage -> block_size = 4096 >> 4;

	}

	

	void* top = freelist_head; //current top of heap


	if ((long)top % 16 == 0){
		top += 8;
		freelist_head = top;
	}

	//if size is less than 16 bytes, can have up to 15 bytes of padding
	//if size is greater than 16 bytes, need to round it to nearest multiple
	//of 16


	if (size > LONGSIZE && size % 16 != 0){
		adjustedSize = (LONGSIZE * ((size + (LONGSIZE) + (LONGSIZE-1)) / LONGSIZE));
		paddingSize = adjustedSize - 16 - size;
	}else if (size > LONGSIZE && size % 16 == 0){
		adjustedSize = size + 16;
		paddingSize = 0;
	}else{
		paddingSize = LONGSIZE - size;
	}

	/*
	if (adjustedSize > space){
		sf_free_header *headOfNewPage = sf_sbrk(0);
		sf_footer *footOfNewPage = NULL;
		extendAndInitiatePage(headOfNewPage, footOfNewPage);
		sf_malloc(size);
	}
	*/

	
	//find block that fits the adjusted size
	sf_free_header* freeBlock = findFreeBlock(adjustedSize);
	//only 1 free block
	
	if (freeBlock != NULL){
		if (freeBlock -> next == NULL && freeBlock -> prev == NULL){
			sf_footer *foot = NULL;
			//allocate the block
			//if there is a splinter
			size_t sizeOfFreeBlock = freeBlock -> header.block_size;
			if (checkForSplinter(freeBlock, adjustedSize) == 1){

				foot = (void*)freeBlock + (sizeOfFreeBlock << 4) - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);
				freelist_head = NULL;
			}else{
				foot = (void*)freeBlock + adjustedSize - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);

				sf_free_header* newFreeHead = (void*)foot + 8;
				newFreeHead -> header.alloc = 0;
				newFreeHead -> header.padding_size = 0;
				newFreeHead -> header.block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				makeHeadOfList(newFreeHead);
				sf_footer *footOfSplitBlock = (void*)newFreeHead + (newFreeHead -> header.block_size << 4) - 8;
				footOfSplitBlock -> block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				footOfSplitBlock -> alloc = 0;  

				freelist_head = newFreeHead;
				freelist_head -> next = NULL;
				freelist_head -> prev = NULL;
			}
			/*
			freelist_head = (void*)foot + 8;
			freelist_head -> header.block_size = space >> 4;
			*/

			numAllocations++;
			internal += 8 + 8 + paddingSize;
			return (void*)freeBlock + 8;

		}else if (freeBlock -> next != NULL && freeBlock -> prev == NULL){
			sf_footer *foot = NULL;
			size_t sizeOfFreeBlock = freeBlock -> header.block_size;
			//freeBlock is freelist_head
			//make next the new freelist_head
			//removeBlock(freeBlock);
	

			if (checkForSplinter(freeBlock, adjustedSize) == 1){
				foot = (void*)freeBlock + (sizeOfFreeBlock << 4) - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);
			}else{
				foot = (void*)freeBlock + adjustedSize - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);

				//make new block
				sf_free_header* newFreeHead = (void*)foot + 8;
				newFreeHead -> header.alloc = 0;
				newFreeHead -> header.padding_size = 0;
				newFreeHead -> header.block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				removeBlock(freeBlock);
				makeHeadOfList(newFreeHead);
				sf_footer *footOfSplitBlock = (void*)newFreeHead + (newFreeHead -> header.block_size << 4) - 8;
				footOfSplitBlock -> block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				footOfSplitBlock -> alloc = 0;  

			}
			numAllocations++;
			internal += 8 + 8 + paddingSize;

			
			//sf_varprint(lowerBound);
			return (void*)freeBlock + 8;



		}else if (freeBlock -> next != NULL && freeBlock -> prev != NULL){
			sf_footer *foot = NULL;
			size_t sizeOfFreeBlock = freeBlock -> header.block_size;
		//middle of list
			removeBlock(freeBlock);

			if (checkForSplinter(freeBlock, adjustedSize) == 1){
				foot = (void*)freeBlock + (sizeOfFreeBlock << 4) - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);
			}else{
				foot = (void*)freeBlock + adjustedSize - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);

			//make new block
				sf_free_header* newFreeHead = (void*)foot + 8;
				newFreeHead -> header.alloc = 0;
				newFreeHead -> header.padding_size = 0;
				newFreeHead -> header.block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				makeHeadOfList(newFreeHead);
				sf_footer *footOfSplitBlock = (void*)newFreeHead + (newFreeHead -> header.block_size << 4) - 8;
				footOfSplitBlock -> block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				footOfSplitBlock -> alloc = 0;  

			}
			numAllocations++;
			internal += 8 + 8 + paddingSize;
			return (void*)freeBlock + 8;


		}else if (freeBlock -> next == NULL && freeBlock -> prev != NULL){
			sf_footer *foot = NULL;
			size_t sizeOfFreeBlock = freeBlock -> header.block_size;
		//tail of the list
			removeBlock(freeBlock);
			if (checkForSplinter(freeBlock, adjustedSize) == 1){
				foot = (void*)freeBlock + (sizeOfFreeBlock << 4) - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);

			}else{
			//allocate and split block
				foot = (void*)freeBlock + adjustedSize - 8;
				allocateBlock(freeBlock, foot, adjustedSize, paddingSize);

			//since block was split, make new block head of list
				sf_free_header* newFreeHead = (void*)foot + 8;
				newFreeHead -> header.alloc = 0;
				newFreeHead -> header.padding_size = 0;
				newFreeHead -> header.block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				makeHeadOfList(newFreeHead);
				sf_footer *footOfSplitBlock = (void*)newFreeHead + (newFreeHead -> header.block_size << 4) - 8;
				footOfSplitBlock -> block_size = ((sizeOfFreeBlock << 4) - adjustedSize) >> 4;
				footOfSplitBlock -> alloc = 0;  

			}
			numAllocations++;
			internal += 8 + 8 + paddingSize;

			return (void*)freeBlock + 8;

		}

	}else if (freeBlock == NULL){
		sf_free_header *headOfNewPage = (void*)footOfPage + 8;
		addPage();
		initializeNewPage(headOfNewPage);
		coalescePage((void*)headOfNewPage + 8);
		sf_malloc(size);
	}
	

	

	return NULL;


}

void coalescePage(void* ptr){

	sf_free_header *headOfNewPage = ptr - 8;

	sf_footer *footerOfPrev = (void*)headOfNewPage - 8; //should be footer of previous page
	

	size_t combinedSize = 0;
	size_t sizeOfNewPage = 0;
	size_t sizeOfPrev = 0;

	sizeOfPrev = footerOfPrev -> block_size;
	sizeOfNewPage = headOfNewPage -> header.block_size;

	sf_free_header *headOfPrevious = ((void*)footerOfPrev + 8)- (sizeOfPrev << 4);
	//sf_free_header* headOfPrev = headOfPrevious;

	removeBlock(headOfPrevious);
	removeBlock(headOfNewPage);


	combinedSize = (sizeOfNewPage << 4) + (sizeOfPrev << 4);


	if (footerOfPrev -> alloc == 0){

		headOfPrevious -> header.block_size = combinedSize >> 4;
		headOfPrevious -> header.alloc = 0;
		headOfPrevious -> header.padding_size = 0;

		sf_footer *newFooter = footOfPage;
		newFooter -> block_size = combinedSize >> 4;
		newFooter -> alloc = 0;

		makeHeadOfList(headOfPrevious);

		


	}else{
		makeHeadOfList(headOfNewPage);
	}


}

int checkForSplinter(sf_free_header* headOfBlock, size_t adjustedSize){

	size_t sizeOfBlock = headOfBlock -> header.block_size;
	if ((sizeOfBlock << 4) - adjustedSize < 32){
		return 1;
	}else{
		return 0;
	}
}

sf_free_header* findFreeBlock(size_t sizeOfBlock){
	sf_free_header* cursor = freelist_head;

	if(freelist_head != NULL){
		while (cursor != NULL){
			if (cursor -> header.block_size << 4 >= sizeOfBlock){
				return cursor;
			}else{
				cursor = cursor -> next;
			}
		}
	}else{
		return NULL;
	}
	return NULL;
	
}

void makeHeadOfList (sf_free_header* block){
	block -> next = freelist_head;
	block -> prev = NULL;

	freelist_head -> prev = block;
	freelist_head = block;
}

void allocateBlock (sf_free_header* freeBlock, sf_footer* foot, size_t adjustedSize, size_t paddingSize){
	sf_header *head = (void*)freeBlock;
	head -> alloc = 1;
	head -> block_size = adjustedSize >> 4;
	head -> padding_size = paddingSize;

	foot -> alloc = 1;
	foot -> block_size = adjustedSize >> 4;

	space -= (head -> block_size << 4);


}


void addPage(){
	sf_sbrk(1);
	sbrkCount++;
	space += 4096;

	upperBound = sf_sbrk(0);
}

void initializeNewPage(sf_free_header *headOfNewPage){
	
	headOfNewPage -> header.block_size = 4096 >> 4;
	headOfNewPage -> header.alloc = 0;
	headOfNewPage -> header.padding_size = 0;

	//removeBlock(freelist_head);
	makeHeadOfList(headOfNewPage);

	sf_footer* footOfNewPage = ((void*)headOfNewPage + 4096) - 8;
	footOfNewPage -> block_size = 4096 >> 4;
	footOfNewPage -> alloc = 0;

	footOfPage = footOfNewPage;

}


void sf_free(void *ptr){

	size_t sizeOfBlock = 0;
	freeFlag = 1;

	//freelist head points to the most recently freed block in memory. most recently freed thing is the top of the list

	if (ptr == NULL)
		return;

	//sf_free_header *freeHead = freelist_head; 
	sf_free_header *newFreeHeader = ptr - 8;


    //shift left 4 to get rid of alloc field
	sizeOfBlock = newFreeHeader -> header.block_size << 4;

	sf_footer *prevBlockFooter = ptr - 16;
	sf_footer *currentBlockFooter = (void*)ptr + sizeOfBlock - 16;
	sf_header *nextBlockHeader = (void*)currentBlockFooter + 8;

	//basic free, no coalescing. case 1 prev allocated, next allocated

	if(prevBlockFooter -> alloc == 1 && nextBlockHeader -> alloc == 1){

    	//set new free header values
    	//newfreeheader becomes freelisthead

		newFreeHeader -> header.alloc = 0;
		newFreeHeader -> header.block_size = sizeOfBlock >> 4;
		newFreeHeader -> header.padding_size = 0;


    	//set new free footer values
		sf_footer *newFreeFooter = (void*)newFreeHeader + sizeOfBlock - 8;
		newFreeFooter -> alloc = 0;
		newFreeFooter -> block_size = sizeOfBlock >> 4;

		space += (newFreeHeader -> header.block_size << 4);


	    //adjust pointers
		makeHeadOfList(newFreeHeader);

		numFrees++;



    	//end basic free
	}else{
		coalesce(ptr);
	}

}

sf_free_header *getTailOfList (){
	sf_free_header *cursor = freelist_head;

	while(cursor -> next != NULL){
		cursor = cursor -> next;
	}

	return cursor;

}


void coalesce(void* ptr){
	size_t combinedSize = 0;

	//freelist head points to the most recently freed block in memory. most recently freed thing is the top of the list

	if (ptr == NULL)
		return;

	//current block header to be freed
	sf_free_header *currentBlockHead = ptr - 8;


    //get size of current block
	size_t sizeOfBlock = currentBlockHead -> header.block_size << 4;

	sf_footer *prevBlockFooter = ptr - 16;
	sf_footer *currentBlockFooter = ptr + sizeOfBlock - 16;
	void *nextBlockHeader = (void*)currentBlockFooter + 8;


	sf_free_header *nextFreeBlock = (sf_free_header*)nextBlockHeader;
	sf_header *nextAllocBlock = (sf_header*)nextBlockHeader;

	/*
	case 2:
		prev allocated, next free
		coalese current block with next free block
		current block header -> block size = block size of current block + block size of next block
		current block header -> alloc = 0

		current block footer = next block footer
	*/
		if ((void*)prevBlockFooter >= lowerBound){

			if (prevBlockFooter -> alloc == 1 && nextFreeBlock -> header.alloc == 0){
				sf_footer* footerOfNextBlock = NULL;
				sf_free_header *newFreeHeader = NULL;
				numCoalesce++;

				size_t sizeOfNextBlock = nextFreeBlock -> header.block_size << 4;
				
				
				combinedSize = (sizeOfNextBlock >> 4) + (sizeOfBlock >> 4);
				footerOfNextBlock = (void*)nextFreeBlock + sizeOfNextBlock - 8;

					//header of coalesed block
				newFreeHeader = (void*)currentBlockHead;

					//set free header values: block size = size of current block + size of next block
				newFreeHeader -> header.block_size = combinedSize;
				newFreeHeader -> header.alloc = 0;
				newFreeHeader -> header.padding_size = 0;

				sf_footer* newFooter = footerOfNextBlock;
				newFooter -> block_size = combinedSize;
				newFooter -> alloc = 0;


				


				removeBlock(nextFreeBlock);
				numCoalesce++;


				if (emptyList == 1){
					freelist_head = newFreeHeader;
					freelist_head -> next = NULL;
					freelist_head -> prev = NULL;

					space += sizeOfBlock;


				}else{
					space += sizeOfNextBlock >> 4;
					makeHeadOfList(newFreeHeader);

				}

		}	/*
			case 3 prev free, next alloc
				coalese current block with previous free block
			*/
				else if (prevBlockFooter -> alloc == 0 && nextAllocBlock -> alloc == 1){

					numCoalesce++;
					size_t sizeOfPrevBlock  = prevBlockFooter -> block_size << 4;

					size_t combinedSize = (sizeOfPrevBlock >> 4) + (sizeOfBlock >> 4);


					sf_free_header *headerOfPrevBlock = (void*)currentBlockHead - sizeOfPrevBlock;
					sf_free_header *newFreeHeader = (void*)headerOfPrevBlock;


					removeBlock(currentBlockHead);

					newFreeHeader -> header.block_size = combinedSize;

					newFreeHeader -> header.alloc = 0;
					newFreeHeader -> header.padding_size = 0;


					sf_footer *newFreeFooter = currentBlockFooter;
					newFreeFooter -> block_size = combinedSize;
					newFreeFooter -> alloc = 0;

					space += newFreeHeader -> header.block_size << 4;


					makeHeadOfList(newFreeHeader);

					numCoalesce++;


		}	/*
			case 4, prev and next are free
			*/
			else if (prevBlockFooter -> alloc == 0 && nextFreeBlock -> header.alloc == 0){

				numCoalesce++;
				size_t sizeOfPrevBlock = prevBlockFooter -> block_size << 4;
				size_t sizeOfNextBlock = nextFreeBlock -> header.block_size << 4;

				size_t combinedSize = (sizeOfPrevBlock >> 4) + (sizeOfNextBlock >> 4) + (sizeOfBlock >> 4);

				sf_free_header *headerOfPrevBlock = (void*)currentBlockHead - sizeOfPrevBlock;
				sf_free_header *headerOfNextBlock = (void*)currentBlockHead + sizeOfBlock;

				//get the footer of next block
				sf_footer *footerOfNextBlock = (void*)nextFreeBlock + sizeOfNextBlock - 8;

				//free_header of coalesed block = head of current block - size of previous block
				sf_free_header *newFreeHeader = (void*)currentBlockHead - sizeOfPrevBlock;

				newFreeHeader -> header.block_size = combinedSize;
				newFreeHeader -> header.alloc = 0;
				newFreeHeader -> header.padding_size = 0;


				space += newFreeHeader -> header.block_size << 4;

				removeBlock(headerOfPrevBlock);
				removeBlock(headerOfNextBlock);

				freelist_head -> prev = newFreeHeader;
				newFreeHeader -> next = freelist_head;
				freelist_head = newFreeHeader;
				freelist_head -> prev = NULL;


				sf_footer *newFreeFooter = footerOfNextBlock;
				newFreeFooter -> block_size = combinedSize;
				newFreeFooter -> alloc = 0;

				numCoalesce++;


			}
			

		}else{
	   //if previous block is not above lower bound

		//check if next block is allocated or not
			if (nextFreeBlock -> header.alloc == 0){

				size_t sizeOfNextBlock = nextFreeBlock -> header.block_size << 4;
			//get size of new block
				size_t combinedSize = (sizeOfNextBlock >> 4) + (sizeOfBlock >> 4);

			//current block header = header of new block
				sf_free_header *newFreeHeader = (void*)currentBlockHead;
			//set values of header
				newFreeHeader -> header.block_size = combinedSize;
				newFreeHeader -> header.alloc = 0;
				newFreeHeader -> header.padding_size = 0;

			//footerOfNextBlock = footer of the new block
				sf_footer *newFreeFooter = (void*)nextFreeBlock + sizeOfNextBlock - 8;
				newFreeFooter -> block_size = combinedSize;
				newFreeFooter -> alloc = 0;


				space += newFreeHeader -> header.block_size << 4;

				removeBlock(nextFreeBlock);
				freelist_head -> prev = newFreeHeader;
				newFreeHeader -> next = freelist_head;
				freelist_head = newFreeHeader;
				freelist_head -> prev = NULL;
				numCoalesce++;

			}
		//if next block is allocated, just free the current block.
			else if (nextAllocBlock -> alloc == 1){

				sf_free_header *newFreeHeader = (void*)currentBlockHead;

				newFreeHeader -> header.block_size = currentBlockHead -> header.block_size;
				newFreeHeader -> header.alloc = 0;
				newFreeHeader -> header.padding_size = 0;

				sf_footer *newFreeFooter = currentBlockFooter;
				newFreeFooter -> block_size = newFreeHeader -> header.block_size;
				newFreeFooter -> alloc = 0;

				makeHeadOfList(newFreeHeader);
				numFrees++;
			}
		}


	}



	void removeBlock(sf_free_header *block){
	//in the middle of the list
		if (block -> next != NULL && block -> prev != NULL){
			block -> prev -> next = block -> next;
			block -> next -> prev = block -> prev;
		}
	//head of the list
		else if (block -> prev == NULL && block -> next != NULL){
			block -> next -> prev = NULL;
			freelist_head = block -> next;
		}
	//tail of the list
		else if (block -> next == NULL && block -> prev != NULL){
			block -> prev -> next = NULL;

		}
	//nothing in the free list
		else if (block -> next == NULL && block -> prev == NULL){
			emptyList = 1;
		}

	}



	void *sf_realloc(void *ptr, size_t size){
		size_t sizeOfBlock = 0;
		size_t paddingSize = 0;
		size_t adjustedSize = 32;
		size_t sizeOfNextBlock = 0;
		//size_t sizeOfNewBlock = 0;
		if (size == 0){
			return NULL;
		}

		//calculate size of ptr
		sf_header *headOfBlock = ptr - 8;
		paddingSize = headOfBlock -> padding_size;
		sizeOfBlock = headOfBlock -> block_size;

		sf_footer* footerOfBlock = (void*)headOfBlock + (sizeOfBlock << 4) - 8;

		//get header of next block to check for coalescing
		void* headerOfNextBlock = (void*)footerOfBlock + 8;
		sf_free_header *freeNextBlock = (sf_free_header*)headerOfNextBlock;
		sf_header *allocNextBlock = (sf_header*)headerOfNextBlock;



		//get size of block and padding for requested size
		if (size > LONGSIZE && size % 16 != 0){
			adjustedSize = (LONGSIZE * ((size + (LONGSIZE) + (LONGSIZE-1)) / LONGSIZE));
			paddingSize = adjustedSize - 16 - size;
		}else if (size > LONGSIZE && size % 16 == 0){
			adjustedSize = size + 16;
			paddingSize = 0;
		}else{
			paddingSize = LONGSIZE - size;
		}

		//if block sizes are the same, just update the padding
		if (adjustedSize == sizeOfBlock << 4){
			headOfBlock -> padding_size = paddingSize;

			internal += 8 + 8 + paddingSize;
			return (void*)headOfBlock + 8;

		//else if the adjustedSize is less than the size of the block, check for splinters, then either coalesce or dont. 
		}else if (adjustedSize < sizeOfBlock << 4){
			//check for splinter
			if ((sizeOfBlock << 4) - adjustedSize < 32){
				//add extra bytes of padding
				headOfBlock -> padding_size = paddingSize;
				headOfBlock -> block_size = sizeOfBlock;

				footerOfBlock = (void*)headOfBlock + (sizeOfBlock << 4) - 8;
				footerOfBlock -> block_size = sizeOfBlock;
				footerOfBlock -> alloc = 1;

				internal += 8 + 8 + paddingSize;
				return (void*)headOfBlock + 8;
			}
			else{
				//if no splinter, split the block
				headOfBlock -> block_size = adjustedSize >> 4;
				headOfBlock -> padding_size = paddingSize;
				headOfBlock -> alloc = 1;

				sf_footer *footerOfNewBlock = (void*)headOfBlock + adjustedSize - 8;
				footerOfNewBlock -> alloc = 1;
				footerOfNewBlock -> block_size = adjustedSize >> 4;

				internal += 8 + 8 + paddingSize;

				//make new free block with remaining bytes
				sf_free_header *headOfNewBlock = (void*)footerOfNewBlock + 8;
				headOfNewBlock -> header.block_size = ((sizeOfBlock << 4) - adjustedSize) >> 4;
				headOfNewBlock -> header.alloc = 0;
				headOfNewBlock -> header.padding_size = 0;

				//sizeOfNewBlock = headOfNewBlock -> header.block_size;
				sf_footer *footOfSplitBlock = (void*)headOfNewBlock + (headOfNewBlock -> header.block_size << 4) - 8;
				footOfSplitBlock -> block_size = ((sizeOfBlock << 4) - adjustedSize) >> 4;
				footOfSplitBlock -> alloc = 0; 

				//make new block head of list
				makeHeadOfList(headOfNewBlock);


				//now check if you need to coalesce
				if (freeNextBlock -> header.alloc == 0){
					coalesce((void*)headOfNewBlock + 8);
				}else{
					makeHeadOfList(headOfNewBlock);
				}

				//sf_snapshot(true);

				return (void*)headOfBlock + 8;


			}
		}else if (adjustedSize > sizeOfBlock << 4){ //need to expand the block
			//if next block is allocated, then malloc to search for a block that fits
			if (allocNextBlock -> alloc == 1){
				void* destination = sf_malloc(adjustedSize);
				memcpy(destination, ptr, adjustedSize);
				sf_free(ptr);
				return destination;
			}
			else if(freeNextBlock -> header.alloc == 0){
				
				sizeOfNextBlock = freeNextBlock -> header.block_size;
				sf_footer *footerOfNextBlock = (void*)freeNextBlock + (sizeOfNextBlock << 4) - 8;

				//if there isnt enough space
				if (adjustedSize > ((sizeOfBlock << 4) + (sizeOfNextBlock <<4))){
					void* destination = sf_malloc(size);
					memcpy(destination, ptr, size);
					sf_free(ptr);
					//sf_snapshot(true);
					return destination;
				}

				//if there is a splinter, take the whole block
				if ((((sizeOfBlock << 4) + (sizeOfNextBlock << 4)) - adjustedSize) < 32){
					removeBlock(freeNextBlock);
					headOfBlock -> block_size = ((sizeOfBlock << 4) + (sizeOfNextBlock << 4)) >> 4;
					headOfBlock -> alloc = 1;
					headOfBlock -> padding_size = paddingSize;

					footerOfNextBlock -> block_size = ((sizeOfBlock << 4) + (sizeOfNextBlock << 4)) >> 4;
					footerOfNextBlock -> alloc = 1;

					internal += 8 + 8 + paddingSize;
					return (void*)headOfBlock + 8;
				}else{
					//if no splinter, split the block
					//printf("adjusted size: %lu\n", adjustedSize);
					//printf("adjusted size >> 4: %lu\n", adjustedSize >> 4);
					headOfBlock -> block_size = adjustedSize >> 4;
					headOfBlock -> alloc = 1;
					headOfBlock -> padding_size = paddingSize;

					footerOfBlock = ((void*)headOfBlock) + adjustedSize - 8;
					footerOfBlock -> block_size = adjustedSize >> 4;
					footerOfBlock -> alloc = 1;

					internal += 8 + 8 + paddingSize;

					//header of new free block
					sizeOfNextBlock = sizeOfNextBlock << 4;

					//printf("sizeOfNextBlock: %lu\n", sizeOfNextBlock);

					sf_free_header *newFreeHeader = ((void*)footerOfBlock) + 8;
					newFreeHeader -> header.block_size = (sizeOfNextBlock - (adjustedSize - size)) >> 4;
					newFreeHeader -> header.alloc = 0;
					newFreeHeader -> header.padding_size = 0;


					footerOfNextBlock = ((void*)newFreeHeader) + (newFreeHeader -> header.block_size) - 8;
					footerOfNextBlock -> block_size = ((sizeOfNextBlock) - (adjustedSize - size)) >> 4;
					footerOfNextBlock -> alloc = 0;
					//removeBlock(newFreeHeader);


					makeHeadOfList(newFreeHeader);

					


					//make new free block head of list
					

					return (void*)headOfBlock + 8;


				}

			}
		}


		return NULL;
	}

	/**
 *  This function will copy the the correct values to the fields
 *  in the memory info struct.
 *  @param meminfo A pointer to the memory info struct passed
 *  to the function, upon return it will containt the calculated
 *  for current fragmentation
 *  @return If successful return 0, if failure return -1
 typedef struct {
    size_t internal;
    size_t external;
    size_t allocations;
    size_t frees;
    size_t coalesce;
} info;
 */
int sf_info(info* meminfo){
	sf_free_header *cursor = freelist_head;
	while(cursor != NULL){
		external += cursor -> header.block_size << 4;
		cursor = cursor -> next;
	}

	meminfo -> internal = internal;
	meminfo -> external = external;
	meminfo -> allocations = numAllocations;
	meminfo -> frees = numFrees;
	meminfo -> coalesce = numCoalesce;
	if (meminfo != NULL){
		return 0;
	}else {
		return 1;
	}
}
