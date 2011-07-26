#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "base.h"
#include "fastlist.h"

#define LIST_BLOCK_SIZE 32

struct list_block {
    struct list_block *next;
    int elements_in_block;
    // elements;
    };

struct list {
    struct list *next;
    int element_size;
    struct list_block *first_list_block;
    struct list_block *last_list_block;
    struct list_block *iterator_block;
    int iterator_position_in_block;
    struct list_block   initial_block;
    };

static struct list *freelist_list = NULL;
static struct list_block *freelist_block = NULL;

int list_allocation_size(int element_size) {
    return sizeof(struct list) + LIST_BLOCK_SIZE * element_size;
    }

void list_init(struct list *l, int element_size) {
    l->element_size = element_size;
    l->first_list_block = &l->initial_block;
    l->initial_block.elements_in_block = 0;
    l->initial_block.next = NULL;
    l->last_list_block = l->first_list_block;
}
        
struct list *list_create(int element_size) {
    struct list *l;
    
    if (!freelist_list)
        l = malloc(list_allocation_size(element_size));
    else {
        l = freelist_list;
        freelist_list = freelist_list->next;
        }
    list_init(l, element_size);
    return l;
}

void list_reinit(struct list *l) {
    struct list_block *lb,*lbn;
    
    lb = l->first_list_block->next;
    while(lb) {
        lbn = lb->next;
        
        lb->next = freelist_block;
        freelist_block = lb;
        lb = lbn;
        }
    l->first_list_block->elements_in_block = 0;
}

void list_destroy(struct list *l) {
    list_reinit(l);
    l->next = freelist_list;
    freelist_list = l;
    }

void list_append(struct list *l, void *element) {
    uint8 *bp;
    
    // potentially need to add a list block
    if (!l->last_list_block ||
        l->last_list_block->elements_in_block == LIST_BLOCK_SIZE) {
        struct list_block *lb;
        
        if (!l->last_list_block || !l->last_list_block->next) {
            if (!freelist_block)
                lb = malloc(sizeof(struct list_block) + LIST_BLOCK_SIZE * l->element_size);
            else {
                lb = freelist_block;
                freelist_block = lb->next;
                }
            lb->next = NULL;
            lb->elements_in_block = 0;
            } else {
            lb = l->last_list_block->next;
            lb->elements_in_block = 0;
            }
        
        if (l->last_list_block)
            l->last_list_block->next = lb;
        if (!l->first_list_block)
            l->first_list_block = lb;
        l->last_list_block = lb;
        }
    bp = (uint8 *)l->last_list_block;
    bp += sizeof(struct list_block);
    memcpy(&bp[l->last_list_block->elements_in_block * l->element_size],
            element,
            l->element_size);
    ++l->last_list_block->elements_in_block;
    if (l->last_list_block->elements_in_block == LIST_BLOCK_SIZE &&
        l->last_list_block->next) {
        l->last_list_block->next->elements_in_block = 0;
        l->last_list_block = l->last_list_block->next;
        }
    }

void list_iterator_init(struct list *l) {
    l->iterator_block = l->first_list_block;
    l->iterator_position_in_block = 0;
    }
    
void *list_next(struct list *l) {
    uint8 *bp;
    void *p;
            
    if (!l->iterator_block ||
        (l->iterator_block->elements_in_block == l->iterator_position_in_block))
        return NULL;
    
    bp = (uint8 *)l->iterator_block;
    bp += sizeof(struct list_block);
    p = (void *)&bp[l->iterator_position_in_block * l->element_size];
    ++l->iterator_position_in_block;
    if (l->iterator_position_in_block == LIST_BLOCK_SIZE) {
        l->iterator_block = l->iterator_block->next;
        l->iterator_position_in_block = 0;
        }
    return p;
    }

void list_clear(struct list *l) {
    if (l->last_list_block) {
        l->first_list_block->elements_in_block = 0;
        l->last_list_block = l->first_list_block;
        }
    }
    
    
        
