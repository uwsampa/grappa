#include <stdio.h>

#define prefetch4(addr) \
    __asm__ volatile(   "prefetcht0 %0\n"   \
                        "prefetcht0 %1\n"   \
                        "prefetcht0 %2\n"   \
                        "prefetcht0 %3\n" : :   \
                         "m" (*(((uint8 *)addr)+0)), \
                         "m" (*(((uint8 *)addr)+64)), \
                         "m" (*(((uint8 *)addr)+128)), \
                         "m" (*(((uint8 *)addr)+192)) );

#define prefetch(addr) \
    __asm__ volatile(   "prefetcht0 %0\n"   \
                         : : "m" (*(((uint8 *)addr)+0)) );

struct task;

struct scheduler {
    void    *scheduler_start;
    int     current_task;
    int     number_of_tasks;
    int     number_of_concurrent_tasks;
    struct  task    *tasks;
    };

enum task_state {
    ts_initial = 0,
    ts_live,
    ts_finished,
    };
    
#define no_task -1

struct task {
    void    *restart_point;
    void    *data;
    enum task_state state;
    };

struct my_task_data {
    int foo;
    };

#define __cat2(x,y) x##y
#define __cat(x,y) __cat2(x,y)

#define _yield(task_struct_ptr, name) \
    (task_struct_ptr)->restart_point = &&__cat(restart_point, __cat(name, __LINE__));  \
    goto *(current_scheduler->scheduler_start);    \
    __cat(restart_point, __cat(name, __LINE__)):

#define yield() _yield(current_task, __cat(__COUNTER__, L))

#define end_task() \
    current_task->state = ts_finished; \
    goto *(current_scheduler->scheduler_start);
 
#define task_scheduler(task_array_ptr, data_array_ptr, _number_of_tasks, _type)    \
    while (1) {   \
    struct scheduler __cat(scheduler, __LINE__);    \
    struct task *current_task = NULL;\
    struct scheduler *current_scheduler = & __cat(scheduler, __LINE__); \
    _type *current_data; \
    int __count = 0;    \
    \
    \
    __cat(scheduler, __LINE__).scheduler_start = \
        &&__cat(scheduler_start, __LINE__);    \
    __cat(scheduler, __LINE__).number_of_tasks = _number_of_tasks; \
    __cat(scheduler, __LINE__).tasks = task_array_ptr; \
    __cat(scheduler, __LINE__).current_task = no_task; \
    for (__count = 0; __count < _number_of_tasks; __count++) {\
        __cat(scheduler, __LINE__).tasks[__count].state = ts_initial; \
        task_array_ptr[__count].data = &data_array_ptr[__count]; \
        }\
    if (_number_of_tasks <= 0) \
        goto __cat(scheduler_end, __LINE__); \
    \
    goto __cat(scheduler_start, __LINE__); \
    \
    __cat(scheduler_end, __LINE__):\
    break; \
    \
    __cat(scheduler_start, __LINE__):   \
    if (__cat(scheduler, __LINE__).current_task == no_task)    \
        __cat(scheduler, __LINE__).current_task = 0;   \
    else { \
        ++__cat(scheduler, __LINE__).current_task; \
        if (__cat(scheduler, __LINE__).current_task >=  \
            __cat(scheduler, __LINE__).number_of_tasks) \
            __cat(scheduler, __LINE__).current_task = 0; \
        } \
    __count = 0; \
    while (__count < \
        __cat(scheduler, __LINE__).number_of_tasks) { \
        if (__cat(scheduler, __LINE__).tasks \
            [__cat(scheduler, __LINE__).current_task].state !=  \
            ts_finished)    \
                break;  \
        ++__cat(scheduler, __LINE__).current_task;  \
        if (__cat(scheduler, __LINE__).current_task >=  \
            __cat(scheduler, __LINE__).number_of_tasks) \
            __cat(scheduler, __LINE__).current_task = 0; \
        ++__count;\
        }   \
    if (__count == __cat(scheduler, __LINE__).number_of_tasks)  \
        goto __cat(scheduler_end, __LINE__); \
    current_task = &__cat(scheduler, __LINE__).tasks \
        [__cat(scheduler, __LINE__).current_task]; \
    current_data = (_type *) current_task->data; \
    if (current_task->state == ts_initial) { \
        current_task->state = ts_live; \
        } \
    else { \
        goto *current_task->restart_point;\
        }

// define some aliases
#define cd current_data
#define ct current_task
#define cs current_scheduler
         
#define end_scheduler() \
    end_task(); \
    }

#ifdef UNIT_TEST
//For example:

void test() {
    struct task ot[2];
    struct my_task_data od[2];
    struct task tasks[10];
    struct my_task_data data[10];
    int n = 10;

    printf("start test!\n");
    task_scheduler(ot, od, 2, struct my_task_data) {
        printf("start outer task\n");
        yield();
        task_scheduler(tasks, data, n, struct my_task_data) {
            // ephemeral variables can be declared here.
            // any task specific persistent data needs to
            // be stored in tasks[].data
            current_data->foo = current_scheduler->current_task;
            printf("Before foo! %d\n", current_data->foo);
            yield();
            printf("Foo! %d\n", current_data->foo);
        } end_scheduler();
    yield();
    printf("end outer task\n");
    } end_scheduler();
    printf("end test!\n");
}

int main() {
    test();
    return 0;
}

#endif 
