/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2010 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
// features.h does not exist on FreeBSD
#ifndef TARGET_BSD
// features initializes the system's state, including the state of __USE_GNU
#include <features.h>
#endif

// If __USE_GNU is defined, we don't need to do anything.
// If we defined it ourselves, we need to undefine it later.
#ifndef __USE_GNU
    #define __USE_GNU
    #define APP_UNDEF_USE_GNU
#endif

#include <ucontext.h>

// If we defined __USE_GNU ourselves, we need to undefine it here.
#ifdef APP_UNDEF_USE_GNU
    #undef __USE_GNU
    #undef APP_UNDEF_USE_GNU
#endif

#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_attr_t thread_attr;
int thread_alive = 0;

void setup_signal_stack(void);
void print_signal_stack(void);
void install_signal_handler(void);
void signal_handler(int, siginfo_t *, void *);
void generate_segv(int val);
void lock();
void unlock();
void *thread_gen_segv(void *);

void setup_signal_stack() {
  int ret_val;
  stack_t ss;
  stack_t oss;

  ss.ss_sp = malloc(SIGSTKSZ);
  assert(ss.ss_sp && "malloc failure");
  
  ss.ss_size = SIGSTKSZ;
  ss.ss_flags = 0;

  printf("ESP of alternate stack: 0x%x, top: 0x%x, size: %d\n",
         ss.ss_sp, (unsigned char *)ss.ss_sp + ss.ss_size, ss.ss_size);

  ret_val = sigaltstack(&ss, &oss);
  if(ret_val) {
    perror("ERROR, sigaltstack failed");
    exit(1);
  }

  printf("ESP of original stack: 0x%x, top: 0x%x, size: %d, disabled = %s\n",
         oss.ss_sp, (unsigned char *)oss.ss_sp + oss.ss_size, oss.ss_size,
         (oss.ss_flags & SS_DISABLE) ? "true" : "false");
}

void print_signal_stack() {
  int ret_val;
  stack_t oss;

  ret_val = sigaltstack(NULL, &oss);
  if(ret_val) {
    perror("ERROR, sigaltstack failed");
    exit(1);
  }

  printf("ESP of original stack: 0x%x, top: 0x%x, size: %d, disabled = %s\n",
         oss.ss_sp, (unsigned char *)oss.ss_sp + oss.ss_size, oss.ss_size,
         (oss.ss_flags & SS_DISABLE) ? "true" : "false");
}

void install_signal_handler() {
  int ret_val;
  struct sigaction s_sigaction;
  struct sigaction *p_sigaction = &s_sigaction;

  /* Register the signal hander using the siginfo interface*/
  p_sigaction->sa_sigaction = signal_handler;
  p_sigaction->sa_flags = SA_SIGINFO | SA_ONSTACK;

  /* Mask all other signals */
  ret_val = sigfillset(&p_sigaction->sa_mask);
  if(ret_val) {
    perror("ERROR, sigfillset failed");
    exit(1);
  }

  ret_val = sigaction(SIGUSR1, p_sigaction, NULL);
  if(ret_val) {
    perror("ERROR, sigaction failed");
    exit(1);
  }
}

void lock() {
  int ret_val;
  
  ret_val = pthread_mutex_lock(&mutex);
  if(ret_val) {
    perror("ERROR, pthread_mutex_lock failed");
  }

  fflush(stdout);
}

void unlock() {
  int ret_val;

  fflush(stdout);

  ret_val = pthread_mutex_unlock(&mutex);
  if(ret_val) {
    perror("ERROR, pthread_mutex_unlock failed");
  }
}

void signal_handler(int signum, siginfo_t *siginfo, void *_uctxt) {
  ucontext_t *uctxt = (ucontext_t *)_uctxt;
  ucontext_t signal_ctxt;

  printf("signal %d (captured EIP: 0x%x)\n", signum,
         uctxt->uc_mcontext.gregs[REG_EIP]);
}

void *thread_start(void *arg) {
  int ret_val;
  sigset_t sigmask;

  printf("sigsuspend...\n");

  lock();
  thread_alive = 1;
  unlock();

  ret_val = sigfillset(&sigmask);
  if(ret_val) {
    perror("ERROR, sigfillset failed");
    exit(1);
  }

  ret_val = sigdelset(&sigmask, SIGUSR1);
  if(ret_val) {
    perror("ERROR, sigdelset failed");
    exit(1);
  }

  ret_val = sigsuspend(&sigmask);
  assert(ret_val && (errno == EINTR));

  printf("unsuspended\n");

  printf("sigwaitinfo...\n");

  lock();
  thread_alive = 2;
  unlock();

  ret_val = sigfillset(&sigmask);
  if(ret_val) {
    perror("ERROR, sigfillset failed");
    exit(1);
  }

  ret_val = sigprocmask(SIG_BLOCK, &sigmask, NULL);
  if(ret_val) {
    perror("ERROR, sigprocmask failed");
    exit(1);
  }

  ret_val = sigemptyset(&sigmask);
  if(ret_val) {
    perror("ERROR, sigemptyset failed");
    exit(1);
  }

  ret_val = sigaddset(&sigmask, SIGUSR1);
  if(ret_val) {
    perror("ERROR, sigaddset failed");
    exit(1);
  }

  ret_val = sigwaitinfo(&sigmask, NULL);
  assert(ret_val == SIGUSR1);

  printf("unsuspended\n");
  return 0;
}

int main(int argc, char **argv) {
  pthread_t tid;
  int ret_val;
  void *thread_ret;
  int cont = 0;

  setup_signal_stack();
  install_signal_handler();

  ret_val = pthread_create(&tid, NULL, thread_start, NULL);
  if(ret_val) {
    perror("ERROR, pthread_create failed");
    exit(1);
  }

  printf("created thread 0x%lx\n", tid);

  while(!cont) {
    lock();
    if(thread_alive == 1) cont = 1;
    unlock();
  }

  ret_val = pthread_kill(tid, SIGUSR1);
  if(ret_val) {
    perror("ERROR: pthread_kill failed");
    exit(1);
  }

  cont = 0;
  while(!cont) {
    lock();
    if(thread_alive == 2) cont = 1;
    unlock();
  }

  ret_val = pthread_kill(tid, SIGUSR1);
  if(ret_val) {
    perror("ERROR: pthread_kill failed");
    exit(1);
  }

  ret_val = pthread_join(tid, NULL);
  if(ret_val) {
    perror("ERROR: pthread_join failed");
    exit(1);
  }
}
