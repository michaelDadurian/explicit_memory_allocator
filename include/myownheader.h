#include "../include/sfmm.h"
#include <string.h>

void coalesce(void* ptr);
void removeBlock(sf_free_header *block);
sf_free_header *getTailOfList ();
void extendAndInitiatePage(sf_free_header*, sf_footer*);
sf_free_header* findFreeBlock(size_t sizeOfBlock);
void makeHeadOfList (sf_free_header* block);
void allocateBlock (sf_free_header* freeBlock, sf_footer* foot, size_t adjustedSize, size_t paddingSize);
int checkForSplinter(sf_free_header* headOfBlock, size_t adjustedSize);
void addPage();
void initializeNewPage(sf_free_header*);
void coalescePage(void* ptr);