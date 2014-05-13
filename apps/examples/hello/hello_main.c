/****************************************************************************
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <errno.h>


/****************************************************************************
 * Definitions
 ****************************************************************************/
#define STACKSIZE 8192
#define PRIORITY         100
#define NARGS              4
/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char arg1[] = "Arg1";
static const char arg2[] = "Arg2";
static const char arg3[] = "Arg3";
static const char arg4[] = "Arg4";
#ifdef SDCC
/* I am not yet certain why SDCC does not like the following
* initializer.  It involves some issues with 2- vs 3-byte
* pointer types.
*/
static const char *g_argv[NARGS+1];
#else
static const char *g_argv[NARGS+1] = { arg1, arg2, arg3, arg4, NULL };
#endif

#  define CONFIG_EXAMPLES_OSTEST_RR_RANGE 10000
#  warning "CONFIG_EXAMPLES_OSTEST_RR_RANGE undefined, using default value = 10000"
#  define CONFIG_EXAMPLES_OSTEST_RR_RUNS 20
#  warning "CONFIG_EXAMPLES_OSTEST_RR_RUNS undefined, using default value = 10"

static const char *g_statenames[] =
{
  "INVALID ",
  "PENDING ",
  "READY   ", 
  "RUNNING ", 
  "INACTIVE", 
  "WAITSEM ", 
#ifndef CONFIG_DISABLE_MQUEUE
  "WAITSIG ", 
#endif
#ifndef CONFIG_DISABLE_MQUEUE
  "MQNEMPTY", 
  "MQNFULL "
#endif
};

static const char *g_ttypenames[4] =
{
  "TASK   ",
  "PTHREAD",
  "KTHREAD",
  "--?--  "
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static int user_main(int argc, char *argv[])
{
	printf("user_main: Exitting\n");
	long i=0;
	for(i=0;i<100000000;i++)
	{
	}
	return 0;
}

static void get_primes(int *count, int *last)
 {
   int number;
   int local_count = 0;
 
   *last = 0;    /* To make the compiler happy */
 
   for (number = 1; number < CONFIG_EXAMPLES_OSTEST_RR_RANGE; number++)
   {
     int div;
     bool is_prime = true;
 
     for (div = 2; div <= number / 2; div++)
       if (number % div == 0)
       {
         is_prime = false;
         break;
       }
 
     if (is_prime)
     {
       local_count++;
       *last = number;
 #if 0 /* We don't really care what the numbers are */
       printf(" Prime %d: %d\n", local_count, number);
 #endif
     }
   }
 
   *count = local_count;
}

void show_task(FAR struct tcb_s *tcb, FAR void *arg)
{
  int i;

  /* Show task/thread status */

  printf("%5d %3d %4s %7s%c%c %8s ",
         tcb->pid, tcb->sched_priority,
         tcb->flags & TCB_FLAG_ROUND_ROBIN ? "RR  " : "FIFO",
         g_ttypenames[(tcb->flags & TCB_FLAG_TTYPE_MASK) >> TCB_FLAG_TTYPE_SHIFT],
         tcb->flags & TCB_FLAG_NONCANCELABLE ? 'N' : ' ',
         tcb->flags & TCB_FLAG_CANCEL_PENDING ? 'P' : ' ',
         g_statenames[tcb->task_state]);

  /* Is this a task or a thread? */

#ifndef CONFIG_DISABLE_PTHREAD
  if ((tcb->flags & TCB_FLAG_TTYPE_MASK) == TCB_FLAG_TTYPE_PTHREAD)
    {
      FAR struct pthread_tcb_s *ptcb = (FAR struct pthread_tcb_s *)tcb;

      /* It is a pthread.  Show any name assigned to the pthread via prtcl() along
       * with the startup value.
       */

#if CONFIG_TASK_NAME_SIZE > 0
      printf("%s(%p)\n", tcb->name, ptcb->arg);
#else
      printf("pthread(%p)\n", ptcb->arg);
#endif
    }
  else
#endif
    {
      FAR struct task_tcb_s *ttcb = (FAR struct task_tcb_s *)tcb;

      /* Show task name and arguments */

      printf("%s(", ttcb->argv[0]);

      /* Special case 1st argument (no comma) */

      if (ttcb->argv[1])
        {
         printf("%p", ttcb->argv[1]);
        }

      /* Then any additional arguments */

#if CONFIG_MAX_TASK_ARGS > 2
      for (i = 2; i <= CONFIG_MAX_TASK_ARGS && ttcb->argv[i]; i++)
        {
          printf(", %p", ttcb->argv[i]);
         }
#endif
      printf(")\n");
    }
}

static FAR void *get_primes_thread(FAR void *parameter)
{
    int id = (int)parameter;
    int count;
    int last;
    int i;
    double tra = 12;
    printf("get_primes_thread id=%d started, looking for primes < %d, doing %d run(s)\n",
          id, CONFIG_EXAMPLES_OSTEST_RR_RANGE, CONFIG_EXAMPLES_OSTEST_RR_RUNS);
	printf("here is %f.\n", tra);
	for (i = 0; i < CONFIG_EXAMPLES_OSTEST_RR_RUNS; i++)
    {
        get_primes(&count, &last);
		printf("%d %d\n", id, i);
    }
    sched_foreach(show_task, NULL);
//printf("%u\n",((FAR struct tcb_s*)g_readytorun.head)->pid);
    printf("get_primes_thread id=%d finished, found %d primes, last one was %d\n",
          id, count, last);
 
    pthread_exit(NULL);
	return NULL; /* To keep some compilers happy */
}


/****************************************************************************
 * hello_main
 *********************
*******************************************************/

int hello_main(int argc, char *argv[])
{
  	printf("Hello, World!!\n");
/*
	int result;
	#ifndef CONFIG_CUSTOM_STACK
         result = task_create("ostest1", PRIORITY, STACKSIZE, user_main,
                         (FAR char * const *)g_argv);
         #else
         result = task_create("ostest1", PRIORITY, user_main,
                         (FAR char * const *)g_argv);
         #endif
         if (result == ERROR)
         {
                 printf("ostest1_main: ERROR Failed to start user_main\n");
         }
         else
         {
                 printf("ostest1_main: Started user_main at PID=%d\n", result);
         }
	printf("ostest_main1: Exitting\n");

	#ifndef CONFIG_CUSTOM_STACK
   	result = task_create("ostest2", PRIORITY, STACKSIZE, user_main,
                        (FAR char * const *)g_argv);
 	#else
   	result = task_create("ostest2", PRIORITY, user_main,
                        (FAR char * const *)g_argv);
 	#endif
   	if (result == ERROR)
     	{
       		printf("ostest2_main: ERROR Failed to start user_main\n");
     	}
   	else
     	{
       		printf("ostest2_main: Started user_main at PID=%d\n", result);
	}
	printf("ostest2_main: Exitting\n");
*/

pthread_t get_primes1_thread;
pthread_t get_primes2_thread;
pthread_t get_primes3_thread;

   struct sched_param sparam;
   pthread_attr_t attr;
   pthread_addr_t result;
   int status;
   status = pthread_attr_init(&attr);
   if (status != OK)
     {
       printf("rr_test: ERROR: pthread_attr_init failed, status=%d\n",  status);
     }
   sparam.sched_priority = sched_get_priority_min(SCHED_FIFO);
   status = pthread_attr_setschedparam(&attr, &sparam);
   if (status != OK)
     {
       printf("rr_test: ERROR: pthread_attr_setschedparam failed, status=%d\n",  status);
     }
   else
     {
       printf("rr_test: Set thread priority to %d\n",  sparam.sched_priority);
     }
   status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
   if (status != OK)
     {
       printf("rr_test: ERROR: pthread_attr_setschedpolicy failed, status=%d\n",  status);
     }
   else
     {
       printf("rr_test: Set thread policy to SCHED_RR\n");
     }
   printf("rr_test: Starting first get_primes_thread\n");
status = pthread_create(&get_primes1_thread, &attr, get_primes_thread, (FAR void *)1);
   if (status != 0)
     {
       printf("         ERROR: Thread 1 creation failed: %d\n",  status);
     }
 
   printf("         First get_primes_thread: %d\n", (int)get_primes1_thread);
   printf("rr_test: Starting second get_primes_thread\n");
 
   status = pthread_create(&get_primes2_thread, &attr, get_primes_thread, (FAR void *)2);
   if (status != 0)
     {
       printf("         ERROR: Thread 2 creation failed: %d\n",  status);
     }
 
   printf("         Second get_primes_thread: %d\n", (int)get_primes2_thread);
   printf("rr_test: Waiting for threads to complete -- this should take awhile\n");
   printf("         If RR scheduling is working, they should start and complete at\n");
   printf("         about the same time\n");
 
   pthread_join(get_primes2_thread, &result);
   pthread_join(get_primes1_thread, &result);
   printf("rr_test: Done\n");
 
  	return 0;
}

