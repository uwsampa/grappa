#ifndef _BM_UID
#  define _BM_UID
#  ifndef UID_FILE_COPIES
#    define UID_FILE_COPIES (1)
#  endif
#  ifndef WMF_DIR_CNT
#    define WMF_DIR_CNT     (1)
#  endif
#  define UID_NOT_USED  (-563281)


#  ifdef HAS_SHMEM
#    define UQID_INT()       uqid_init()
#    define UQID(A)          uqid(A)
#    define UQID_FINALIZE()  uqid_finalize()
#    define UQID_BARRIER()   uqid_barrier()
#    define UQID_EXIT(A)     uqid_exit(A)
     void uqid_init(void);
     void uqid(int*);
     void uqid_finalize(void);
     void uqid_barrier(void);
     void uqid_exit(int);
#  elif defined HAS_MPI
#    define UQID_INT()       uqid_init()
#    define UQID(A)          uqid(A)
#    define UQID_FINALIZE()  uqid_finalize()
#    define UQID_BARRIER()   uqid_barrier()
#    define UQID_EXIT(A)     uqid_exit(A)
     void uqid_init(void);
     void uqid(int*);
     void uqid_finalize(void);
     void uqid_barrier(void);
     void uqid_exit(int);
#  else
     void uqid_init(void);
     void uqid(int*);
     void uqid_finalize(void);
     void uqid_barrier(void);
     void uqid_exit(int);

#    define UQID_INT()
#    define UQID(A)          ((*(A)) = -1)
#    define UQID_FINALIZE()
#    define UQID_BARRIER()   uqid_barrier()
#    define UQID_EXIT()      uqid_exit()

#  endif

#endif
