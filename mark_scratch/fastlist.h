struct list;

// Returns the size of memory that should be allocated
// when creating a new list for elements of size "element_size"
int list_allocation_size(int element_size);

// Takes a blank chunk of memory of list_allocation_size and
// initiates it into a list
void list_init(struct list *l, int element_size);

// Re-initiates an already inited list.  This also gives back
// extension blocks of memory to the free pool. 
void list_reinit(struct list *l);

// This allocates a new list and inits it.
struct list *list_create(int element_size);

// This destroys a list, returing extension blocks to the free
// pool and placing the list itself on a free pool.
void list_destroy(struct list *);

// This clears all elements on the list but does not give back
// back any extension blocks to the free pool.
void list_clear(struct list *);

// Add an item to list
void list_append(struct list *, void *element);

// Initialize the iterator for the list to the head of the list
void list_iterator_init(struct list *);

// Return a pointer to the current item pointed to by the
// iterator and advance the iterator to the next element.
void *list_next(struct list *);
