typedef struct checkidstruct
{
  int *ID;
  struct checkidstruct *next;
} Checklisttype;

int *checkID(int* ptr);
int *deleteID( int *ptr);
int *addID( int *ptr);
