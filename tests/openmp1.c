#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#  include <omp.h>
#endif
#include "../pthread.h"

enum {
  Size = 10000
};

static const int  ShouldSum = (Size-1)*Size/2;

static short Verbose = 1;

static short ThreadOK[3] = {0,0,0}; // Main, Thread1, Thread2

// Thread
static void *_thread(void* Id) {
  int i;
  int x[Size];
  int Sum;

#ifdef _OPENMP
#  pragma omp parallel for
#endif
  for ( i = 0; i < Size; i++ ) {
#ifdef _OPENMP
    if (Verbose && i%1000==0) {
      int tid = omp_get_thread_num();
#  pragma omp critical
      printf("thread %d : tid %d handles %d\n",(int)(size_t)Id,tid,i);
    }
#endif

    x[i] = i;
  }

  Sum=0;
  for ( i = 0; i < Size; i++ ) {
    Sum += x[i];
  }
  if (Verbose) {
#ifdef _OPENMP
#  pragma omp critical
#endif
    printf("Id %d : %s : %d(should be %d)\n",(int)(size_t)Id, __FUNCTION__, Sum,ShouldSum);
  }
  if (Sum == ShouldSum) ThreadOK[(int)(size_t)Id] = 1;
  return NULL;
}

// MainThread
static void MainThread(void) {
  int i;

#ifdef _OPENMP
#  pragma omp parallel for
#endif
  for ( i = 0; i < 4; i++ ) {
#ifdef _OPENMP
      int tid = omp_get_thread_num();
#  pragma omp critical
      printf("Main : tid %d\n",tid);
      _thread((void *)(size_t)tid);
#endif
  }
  return;
}

// Comment in/out for checking the effect of multiple threads.
#define SPAWN_THREADS

// main
#ifndef MONOLITHIC_PTHREAD_TESTS
int
main(int argc, char *argv[])
#else
int
test_openmp1(int argc, char *argv[])
#endif
{
  short OK;

  if (argc>1) Verbose = 1;

#ifdef _OPENMP
  omp_set_nested(-1);
  printf("%s%s%s\n", "Nested parallel blocks are ", omp_get_nested()?" ":"NOT ", "supported.");
#endif

  MainThread();

#ifdef SPAWN_THREADS
  {
    pthread_t a_thr;
    pthread_t b_thr;
    int status;

	memset(&a_thr, 0, sizeof(a_thr)); /* [i_a] fix valid MSVC complaint about unitialized a_thr / b_thr */
	memset(&b_thr, 0, sizeof(b_thr)); /* [i_a] fix valid MSVC complaint about unitialized a_thr / b_thr */

    status = pthread_create(&a_thr, NULL, _thread, (void*) 1 );
    if ( status != 0 ) {
      printf("Failed to create thread 1\n");
      return (-1);
    }

    status = pthread_create(&b_thr, NULL, _thread, (void*) 2 );
    if ( status != 0 ) {
      printf("Failed to create thread 2\n");
      return (-1);
    }

    printf("%s:%d - %s - a_thr:%p - b_thr:%p\n",
           __FILE__,__LINE__,__FUNCTION__,a_thr.p,b_thr.p);

    status = pthread_join(a_thr, NULL);
    if ( status != 0 ) {
      printf("Failed to join thread 1\n");
      return (-1);
    }
    printf("Joined thread1\n");

    status = pthread_join(b_thr, NULL);
    if ( status != 0 ) {
      printf("Failed to join thread 2\n");
      return (-1);
    }
    printf("Joined thread2\n");
  }
#endif // SPAWN_THREADS

  OK = 0;
  // Check that we have OpenMP before declaring things OK formally.
#ifdef _OPENMP
    OK = 1;
    {
      short i;
      for (i=0;i<3;i++) OK &= ThreadOK[i];
    }
    if (OK) printf("OMP : All looks good\n");
    else printf("OMP : Error\n");
#else
    OK = 1;
    printf("OpenMP seems not enabled ...\n");
#endif

  return OK?0:1;
}

//g++ -fopenmp omp_test.c -o omp_test -lpthread

