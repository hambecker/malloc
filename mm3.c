/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"


/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

//overhead of total data
#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))

//Macros for getting the header from a payload pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))

//Macros for working with raw pointer as the header: 
#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated

//Macro for getting the next block's payload
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))


#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

//Gets the footer from a header
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-OVERHEAD)

/* rounds down to the nearest multiple of mem_pagesize() */
 #define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))


typedef struct {
	size_t size;
	char allocated;
} block_header;

typedef struct unalloc_bp {
	struct unalloc_bp *prev;
	struct unalloc_bp *next;
} unalloc_bp;


typedef struct {
	size_t size;
	int filler;
} block_footer;

typedef struct page_locater {
	struct page_locater *next;
	struct page_locater *prev;
	size_t size;
	char filler;
} page_locater;



void *extend(size_t size);
void set_allocated(void *bp, size_t size);
void *coalesce(void *bp);
int ptr_is_mapped(void *p, size_t len);

void *current_avail = NULL;
int current_avail_size = 0;
void *first_bp = NULL;
page_locater *first_page = NULL;
unalloc_bp *first_unalloc = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{


	//gets a page size for a new block of memory
	current_avail_size = PAGE_ALIGN(sizeof(block_header));

	//allocates an initial block of memory 
	current_avail = mem_map(current_avail_size);
	if (current_avail == NULL)
		return -1;

	//sets page linker for first node
	first_page = ((page_locater*)(current_avail));
	first_page->next = NULL;
	first_page->size = current_avail_size;
	first_page->prev = NULL;

	//sets header for for first node
	first_bp = current_avail + sizeof(block_header) + sizeof(page_locater);
	GET_SIZE(HDRP(first_bp)) = OVERHEAD;
	GET_SIZE(FTRP(first_bp)) = OVERHEAD;
	GET_ALLOC(HDRP(first_bp)) = 1;


	//set head size for first node
	GET_SIZE(HDRP(NEXT_BLKP(first_bp))) = current_avail_size  - OVERHEAD  - sizeof(block_header) - sizeof(page_locater);
	GET_ALLOC(HDRP(NEXT_BLKP(first_bp))) = 0;
	GET_SIZE(FTRP(NEXT_BLKP(first_bp))) = current_avail_size - OVERHEAD  - sizeof(block_header) - sizeof(page_locater);

	//set last node to 0 size and 1 allocated
	GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(first_bp)))) = 0;
	GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(first_bp)))) = 1;

	first_bp =  first_bp + OVERHEAD;

	first_unalloc = first_bp;
	first_unalloc->next = NULL;
	first_unalloc->prev = NULL;

	return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
	int newsize = ALIGN(size + OVERHEAD);
	void *bp = first_bp;
	struct page_locater *page = first_page;
	struct unalloc_bp *unalloc_traverser  = first_unalloc;
	

	while(unalloc_traverser!=NULL) {
		bp = (void*)unalloc_traverser;
		size_t memory_size = GET_SIZE(HDRP(bp)); 
		char allocated = GET_ALLOC(HDRP(bp));
		if (!allocated  && (memory_size >= newsize)) {


			set_allocated(bp, newsize);


			// if unalloc is in the middle of two in the list remove it from the list
			if(unalloc_traverser->prev != NULL && unalloc_traverser->next!= NULL){
				unalloc_traverser->prev->next = unalloc_traverser->next->prev;
			}

			//if unalloc is at the head of the list and there is no next make first unalloc NULL
			else if(unalloc_traverser->prev == NULL && unalloc_traverser->next== NULL){
				first_unalloc = NULL;
			}
			//must be head of the explicit list
			else if(unalloc_traverser->prev == NULL){
				first_unalloc = first_unalloc->next;
				first_unalloc->prev = NULL;

			//must be tail of the explicit list	
			} else {
				unalloc_traverser->prev->next = NULL;
			}

			return bp;
		}
		unalloc_traverser = unalloc_traverser->next;
	}



	// if(first_page->next==NULL)
	// {	
	// 	while (GET_SIZE(HDRP(bp)) != 0) {

			// size_t memory_size = GET_SIZE(HDRP(bp)); 
			// char allocated = GET_ALLOC(HDRP(bp));
			// if (!allocated  && (memory_size >= newsize)) {
		 // 		set_allocated(bp, newsize);
			// 	return bp;
			// }
		 // 	bp = NEXT_BLKP(bp);
	// 	}
	// } else {
	// 	while(page->next!=NULL){

	// 		while (GET_SIZE(HDRP(bp)) != 0) {
	// 			if (!GET_ALLOC(HDRP(bp))&& (GET_SIZE(HDRP(bp)) >= newsize)) {
	// 	 		set_allocated(bp, newsize);
	// 			return bp;
	// 		}
	// 	 		bp = NEXT_BLKP(bp);
	// 		}
	// 		page = page->next;
	// 		bp = ((char*)page)+sizeof(page_locater)+OVERHEAD +sizeof(block_header);
	// 	}	
	// }

	bp = extend(newsize);
	return bp;
}

/*
*  extend - creates a new page
*/   

void *extend(size_t size)
{

	//gets a page size for a new block of memory
	current_avail_size = PAGE_ALIGN(size + OVERHEAD + sizeof(block_header) +sizeof(page_locater));

	//allocates an initial block of memory 
	void *new_avail = mem_map(current_avail_size);

	//grab the first page
	struct page_locater *temp = first_page;

	// go through the pages untill you find a next page that is null
	while(temp->next!=NULL)
		temp = temp->next;


	//sets page linker for first node
	struct page_locater *new_page = ((page_locater*)new_avail);
	new_page->next = NULL;
	new_page->size = current_avail_size;
	temp->next = new_page;
	new_page->prev = temp;


	//sets header for for first node
	void *beg_bp = new_avail + sizeof(block_header) + sizeof(page_locater);
	GET_SIZE(HDRP(beg_bp)) = OVERHEAD;
	GET_SIZE(FTRP(beg_bp)) = OVERHEAD;
	GET_ALLOC(HDRP(beg_bp)) = 1;

	//set head size for first node
	GET_SIZE(HDRP(NEXT_BLKP(beg_bp))) = current_avail_size - OVERHEAD  - sizeof(block_header) - sizeof(page_locater);
	GET_ALLOC(HDRP(NEXT_BLKP(beg_bp))) = 0;
	GET_SIZE(FTRP(NEXT_BLKP(beg_bp))) = current_avail_size - OVERHEAD  - sizeof(block_header) - sizeof(page_locater);

	//set last node to 0 size and 1 allocated
	GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(beg_bp)))) = 0;
	GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(beg_bp)))) = 1;

	beg_bp =  beg_bp + OVERHEAD;
	set_allocated(beg_bp, size);
	return beg_bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	GET_ALLOC(HDRP(ptr)) = 0;
	//size_t size = GET_SIZE(HDRP(ptr));
	coalesce(ptr);
	
	void *p = ptr;

	if(GET_SIZE(HDRP(PREV_BLKP(p)))== OVERHEAD && GET_SIZE(HDRP(NEXT_BLKP(p)))== 0)
	{
		p = p - sizeof(block_header) - sizeof(page_locater) - OVERHEAD;

		struct page_locater *deletion_page = ((page_locater*)p);
		if(deletion_page->prev!=NULL && deletion_page->next!=NULL)
		{
			if(deletion_page->prev != NULL && deletion_page->next !=  NULL)
			{
				deletion_page->prev->next = deletion_page->next;
				deletion_page->next->prev = deletion_page->prev;
			}
			//HEAD of pages case where there is a next
			else 	if(deletion_page->prev == NULL && deletion_page->next !=  NULL)
			{
				first_page = deletion_page->next;
				first_page->prev = NULL;
			}
			//TAIL of pages case
			else if(deletion_page->prev !=NULL && deletion_page->next == NULL)
			{
				deletion_page->prev->next = NULL;
			}
			mem_unmap(p, deletion_page->size);
		}
	}
}
/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{

	void *bp = first_bp;
	page_locater *page = first_page;
	if(page->next==NULL)
	{		


		if(!ptr_is_mapped(HDRP(bp), sizeof(block_header))){
			return 0;
		}

		while (GET_SIZE(HDRP(bp)) != 0) 
		{
			if(!ptr_is_mapped(bp, GET_SIZE(HDRP(bp)))){
				return 0;
			}


			// Head and tail check
			size_t head_memory_size = GET_SIZE(HDRP(bp)); 
			size_t footer_memory_size = GET_SIZE(FTRP(bp)); 
			if (head_memory_size != footer_memory_size) 
			{
				return 0;
			}

			//coalesce check
			if(GET_ALLOC(HDRP(bp))==0){
				if((GET_ALLOC(HDRP(PREV_BLKP(bp))) == 0) || (GET_ALLOC(HDRP(NEXT_BLKP(bp))) ==0))
				{
					return 0;
				}
			}

		 	bp = NEXT_BLKP(bp);
		}
	} else {
		while(page!=NULL){
			bp = ((char*)page)+sizeof(page_locater)+OVERHEAD +sizeof(block_header);
			if(!ptr_is_mapped(HDRP(bp), sizeof(block_header))){
				return 0;
			}
			while (GET_SIZE(HDRP(bp)) != 0) {
				if(!ptr_is_mapped(bp, GET_SIZE(HDRP(bp)))){
					return 0;
				}

				//Head and tail check
				size_t head_memory_size = GET_SIZE(HDRP(bp)); 
				size_t footer_memory_size = GET_SIZE(FTRP(bp));
				if (head_memory_size != footer_memory_size) {
					return 0;
				}


				//coalesce check
				if(GET_ALLOC(HDRP(bp))==0){
					if((GET_ALLOC(HDRP(PREV_BLKP(bp))) == 0) || (GET_ALLOC(HDRP(NEXT_BLKP(bp))) ==0))
					{
						return 0; 
					}
				}
		 		bp = NEXT_BLKP(bp);
			}
			page = page->next;
		}
	}
	return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{

	void *bp;
	page_locater *page = first_page;
	while(page != NULL)
	{

		bp = ((char*)page)+sizeof(page_locater)+OVERHEAD +sizeof(block_header);
		//points to first payload
		while(GET_SIZE(HDRP(bp))!= 0)
		{
			if(bp == p)
			{
				if(GET_SIZE(HDRP(bp))>=0)
				{
					if(ptr_is_mapped(p, GET_SIZE(HDRP(bp))))
					{
						if(GET_SIZE(HDRP(p))== GET_SIZE(FTRP(p)))
						{
							if(GET_ALLOC(HDRP(p)) == 1)
							{
								return 1;
							}
					}	}
				}	

			}
			bp = NEXT_BLKP(bp);
		}
		page = page->next;
	}	
	return 0;
}


void set_allocated(void *bp, size_t size) {
	size_t extra_size = GET_SIZE(HDRP(bp)) - size;

	if (extra_size > ALIGN(1 + OVERHEAD)) {
		GET_SIZE(HDRP(bp)) = size;
		GET_SIZE(FTRP(bp)) = size;
		GET_SIZE(HDRP(NEXT_BLKP(bp))) = extra_size;
		GET_SIZE(FTRP(NEXT_BLKP(bp))) = extra_size;
		GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0;

		//travers to add to the end of the explicit list
		struct unalloc_bp *traverse = first_unalloc;
		while(traverse->next!=NULL){
			traverse = traverse->next;
		}
		traverse->next = (unalloc_bp*)NEXT_BLKP(bp);
	}
	GET_ALLOC(HDRP(bp)) = 1;


}

void *coalesce(void *bp){
	size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	struct unalloc_bp *traverse = first_unalloc;

	if (prev_alloc && !next_alloc){
		
		//coalsece together
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		GET_SIZE(HDRP(bp)) = size;
		GET_SIZE(FTRP(bp)) = size;

		//update the implicit free list

		//find the next and delete it

	}

	// previous block un allocated and nect allocated, with the coalecsing it will not need to be added to the list
	else if (!prev_alloc && next_alloc) {
		size+= GET_SIZE(HDRP(PREV_BLKP(bp)));
		GET_SIZE(FTRP(bp)) = size;
		GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
		bp = PREV_BLKP(bp);
	}

	// coalsecing needed for prev and next need to remove the next from the explicit free list
	else if (!prev_alloc && !next_alloc) {
		size+= (GET_SIZE(HDRP(PREV_BLKP(bp))) +GET_SIZE(HDRP(NEXT_BLKP(bp))));
		GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
		GET_SIZE(FTRP(NEXT_BLKP(bp))) = size;

		//find the next_bp and remove it from the list
		struct unalloc_bp *next_deletion = (unalloc_bp*)NEXT_BLKP(bp);
		//if it has a next and previous get rid of it
		if(next_deletion->prev != NULL && next_deletion->next !=  NULL)
		{
			next_deletion->prev->next = next_deletion->next;
			next_deletion->next->prev = next_deletion->prev;
		}
		//HEAD of pages case where there is a next
		else 	if(next_deletion->prev == NULL && next_deletion->next !=  NULL)
		{
			first_unalloc = next_deletion->next;
			first_unalloc->prev = NULL;
		}
		//TAIL of pages case
		else if(next_deletion->prev !=NULL && next_deletion->next == NULL)
		{
			next_deletion->prev->next = NULL;
		}



		bp = PREV_BLKP(bp);
	}

	//no coalecing needed but need to add to the end of the explicit
	else{

		//if nothing is on the list of explicits make the first item
		if(traverse==NULL){
			first_unalloc = bp;
			first_unalloc->next = NULL;
			first_unalloc->prev = NULL;
		} else{
			//update the implicit free list
			//travers to add to the end of the explicit list
			while(traverse!=NULL){
				traverse = traverse->next;
			}
			traverse->next = (unalloc_bp*)bp;
		}
	}

	return bp;
}

int ptr_is_mapped(void *p, size_t len) {
    void *s = ADDRESS_PAGE_START(p);
    return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}
