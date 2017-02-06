#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    int *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == (void*)x-8);
    
    sf_free_header *headofx = ((void*)x)-8;
    sf_footer *footofx = (((void*)x) - 8 + (headofx->header.block_size << 4)) - 8;

    sf_blockprint((((void*)x)-8));
    cr_assert(headofx->header.alloc == 0);
    cr_assert(headofx->header.block_size<<4 == 32);
    cr_assert(headofx->header.padding_size == 0);

    cr_assert(footofx->alloc == 0);
    cr_assert(footofx->block_size<<4 == 32);
}

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

Test(sf_memsuite, Realloc_shrink_splinter, .init = sf_mem_init, .fini = sf_mem_fini){
    int *x = sf_malloc(sizeof(int) * 16);
    sf_header *headofx = ((void*)x)-8;
    cr_assert(headofx -> block_size << 4 == 80);

    sf_realloc(x, (sizeof(int) * 10));
    cr_assert(headofx -> block_size << 4 == 80);


}


Test(sf_memsuite, Realloc_shrink_coalesce, .init = sf_mem_init, .fini = sf_mem_fini){

    int *x = sf_malloc(sizeof(int) * 16);


    sf_header *headofx = ((void*)x)-8;
    sf_footer *footofx = (((void*)x) - 8 + (headofx->block_size << 4)) - 8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 16);
    cr_assert(freelist_head -> header.block_size << 4 == 3936);

    sf_free(y);
    cr_assert(freelist_head == (void*)y - 8);
    cr_assert(freelist_head -> header.block_size << 4 == 4016);


    sf_realloc(x, (sizeof(int) * 6));
    cr_assert(headofx -> block_size << 4 == 48);

    sf_free_header *headOfNewBlock = (void*)footofx + 8;
    //sf_varprint(headOfNewBlock);
    cr_assert(headOfNewBlock -> header.alloc == 0);
    cr_assert(headOfNewBlock -> header.block_size << 4 == 4016);
    cr_assert(headOfNewBlock -> header.padding_size == 0);
    //sf_snapshot(true);


}

Test(sf_memsuite, Realloc_shrink_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini){

    int *x = sf_malloc(sizeof(int) * 16);


    sf_header *headofx = ((void*)x)-8;
    //sf_footer *footofx = (((void*)x) - 8 + (headofx->block_size << 4)) - 8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 16);
    sf_header *headofy = ((void*)y)-8;
    cr_assert(headofy -> block_size << 4 == 80);

    
    sf_realloc(x, (sizeof(int) * 6));
    sf_footer *newfootofx = (((void*)x) - 8 + (headofx->block_size << 4)) - 8;
    cr_assert(headofx -> block_size << 4 == 48);

    sf_free_header *headOfNewBlock = (void*)newfootofx + 8;
    //sf_varprint(headOfNewBlock);
    cr_assert(headOfNewBlock -> header.alloc == 0);
    cr_assert(headOfNewBlock -> header.block_size << 4 == 32);
    cr_assert(headOfNewBlock -> header.padding_size == 0);
    //sf_snapshot(true);


}

Test(sf_memsuite, basic_free, .init = sf_mem_init, .fini = sf_mem_fini){
    int *x = sf_malloc(sizeof(int) * 16);
    sf_header *headofx = ((void*)x)-8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 6);
    sf_header *headofy = ((void*)y)-8;
    cr_assert(headofy -> block_size << 4 == 48);

    int *z = sf_malloc(sizeof(int) * 10);
    sf_header *headofz = ((void*)z)-8;
    cr_assert(headofz -> block_size << 4 == 64);

    sf_free(y);
    sf_free_header *freeheadofy = (void*)headofy;
    cr_assert(freeheadofy -> header.block_size << 4 == 48);
    cr_assert(freeheadofy -> header.alloc == 0);
    cr_assert(freeheadofy -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofy);

}

Test(sf_memsuite, coalesce_case_2, .init = sf_mem_init, .fini = sf_mem_fini){
    int *x = sf_malloc(sizeof(int) * 16);
    sf_header *headofx = ((void*)x)-8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 6);
    sf_header *headofy = ((void*)y)-8;
    cr_assert(headofy -> block_size << 4 == 48);

    int *z = sf_malloc(sizeof(int) * 10);
    sf_header *headofz = ((void*)z)-8;
    cr_assert(headofz -> block_size << 4 == 64);

    int *a = sf_malloc(sizeof(int));
    sf_header *headofa = ((void*)a)-8;
    cr_assert(headofa -> block_size << 4 == 32);


    sf_free(z);
    sf_free_header *freeheadofz = (void*)headofz;
    cr_assert(freeheadofz -> header.block_size << 4 == 64);
    cr_assert(freeheadofz -> header.alloc == 0);
    cr_assert(freeheadofz -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofz);


    sf_free(y);
    sf_free_header *freeheadofy = (void*)headofy;
    cr_assert(freeheadofy -> header.block_size << 4 == 112);
    cr_assert(freeheadofy -> header.alloc == 0);
    cr_assert(freeheadofy -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofy);

}

Test(sf_memsuite, coalesce_case_3, .init = sf_mem_init, .fini = sf_mem_fini){
    int *x = sf_malloc(sizeof(int) * 16);
    sf_header *headofx = ((void*)x)-8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 6);
    sf_header *headofy = ((void*)y)-8;
    cr_assert(headofy -> block_size << 4 == 48);

    int *z = sf_malloc(sizeof(int) * 10);
    sf_header *headofz = ((void*)z)-8;
    cr_assert(headofz -> block_size << 4 == 64);

    int *a = sf_malloc(sizeof(int));
    sf_header *headofa = ((void*)a)-8;
    cr_assert(headofa -> block_size << 4 == 32);


    sf_free(x);
    sf_free_header *freeheadofx = (void*)headofx;
    cr_assert(freeheadofx -> header.block_size << 4 == 80);
    cr_assert(freeheadofx -> header.alloc == 0);
    cr_assert(freeheadofx -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofx);


    sf_free(y);
    cr_assert(freeheadofx -> header.block_size << 4 == 128);
    cr_assert(freeheadofx -> header.alloc == 0);
    cr_assert(freeheadofx -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofx);

}

Test(sf_memsuite, coalesce_case_4, .init = sf_mem_init, .fini = sf_mem_fini){
    int *x = sf_malloc(sizeof(int) * 16);
    sf_header *headofx = ((void*)x)-8;
    cr_assert(headofx -> block_size << 4 == 80);

    int *y = sf_malloc(sizeof(int) * 6);
    sf_header *headofy = ((void*)y)-8;
    cr_assert(headofy -> block_size << 4 == 48);

    int *z = sf_malloc(sizeof(int) * 10);
    sf_header *headofz = ((void*)z)-8;
    cr_assert(headofz -> block_size << 4 == 64);

    int *a = sf_malloc(sizeof(int));
    sf_header *headofa = ((void*)a)-8;
    cr_assert(headofa -> block_size << 4 == 32);


    sf_free(x);
    sf_free_header *freeheadofx = (void*)headofx;
    cr_assert(freeheadofx -> header.block_size << 4 == 80);
    cr_assert(freeheadofx -> header.alloc == 0);
    cr_assert(freeheadofx -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofx);


    sf_free(z);
    sf_free_header *freeheadofz = (void*)headofz;
    cr_assert(freeheadofz -> header.block_size << 4 == 64);
    cr_assert(freeheadofz -> header.alloc == 0);
    cr_assert(freeheadofz -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofz);

    sf_free(y);
    sf_free_header *freeheadofy = (void*)freeheadofx;
    cr_assert(freeheadofy -> header.block_size << 4 == 192);
    cr_assert(freeheadofy -> header.alloc == 0);
    cr_assert(freeheadofy -> header.padding_size == 0);

    cr_assert(freelist_head == freeheadofy);

}
