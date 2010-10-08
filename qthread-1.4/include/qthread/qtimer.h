#ifndef QTHREAD_TIMER
#define QTHREAD_TIMER

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtimer_s *qtimer_t;

void qtimer_start(qtimer_t);
void qtimer_stop(qtimer_t);
double qtimer_secs(qtimer_t);

qtimer_t qtimer_create(void);
void qtimer_destroy(qtimer_t);

#ifdef __cplusplus
}
#endif

#endif
