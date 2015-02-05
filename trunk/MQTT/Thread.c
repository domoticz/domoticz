/*******************************************************************************
 * Copyright (c) 2009, 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation
 *    Ian Craggs, Allan Stockdill-Mander - async client updates
 *    Ian Craggs - bug #415042 - start Linux thread as disconnected
 *    Ian Craggs - fix for bug #420851
 *******************************************************************************/
/**
 * @file
 * \brief Threading related functions
 *
 * Used to create platform independent threading functions
 */


#include "Thread.h"
#if defined(THREAD_UNIT_TESTS)
#define NOSTACKTRACE
#endif
#include "StackTrace.h"

#undef malloc
#undef realloc
#undef free

#if !defined(WIN32) && !defined(WIN64)
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#endif
#include <memory.h>
#include <stdlib.h>

/**
 * Start a new thread
 * @param fn the function to run, must be of the correct signature
 * @param parameter pointer to the function parameter, can be NULL
 * @return the new thread
 */
thread_type Thread_start(thread_fn fn, void* parameter)
{
#if defined(WIN32) || defined(WIN64)
	thread_type thread = NULL;
#else
	thread_type thread = 0;
	pthread_attr_t attr;
#endif

	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	thread = CreateThread(NULL, 0, fn, parameter, 0, NULL);
#else
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attr, fn, parameter) != 0)
		thread = 0;
	pthread_attr_destroy(&attr);
#endif
	FUNC_EXIT;
	return thread;
}


/**
 * Create a new mutex
 * @return the new mutex
 */
mutex_type Thread_create_mutex()
{
	mutex_type mutex = NULL;
	int rc = 0;

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		mutex = CreateMutex(NULL, 0, NULL);
	#else
		mutex = malloc(sizeof(pthread_mutex_t));
		rc = pthread_mutex_init(mutex, NULL);
	#endif
	FUNC_EXIT_RC(rc);
	return mutex;
}


/**
 * Lock a mutex which has already been created, block until ready
 * @param mutex the mutex
 * @return completion code, 0 is success
 */
int Thread_lock_mutex(mutex_type mutex)
{
	int rc = -1;

	/* don't add entry/exit trace points as the stack log uses mutexes - recursion beckons */
	#if defined(WIN32) || defined(WIN64)
		/* WaitForSingleObject returns WAIT_OBJECT_0 (0), on success */
		rc = WaitForSingleObject(mutex, INFINITE);
	#else
		rc = pthread_mutex_lock(mutex);
	#endif

	return rc;
}


/**
 * Unlock a mutex which has already been locked
 * @param mutex the mutex
 * @return completion code, 0 is success
 */
int Thread_unlock_mutex(mutex_type mutex)
{
	int rc = -1;

	/* don't add entry/exit trace points as the stack log uses mutexes - recursion beckons */
	#if defined(WIN32) || defined(WIN64)
		/* if ReleaseMutex fails, the return value is 0 */
		if (ReleaseMutex(mutex) == 0)
			rc = GetLastError();
		else
			rc = 0;
	#else
		rc = pthread_mutex_unlock(mutex);
	#endif

	return rc;
}


/**
 * Destroy a mutex which has already been created
 * @param mutex the mutex
 */
void Thread_destroy_mutex(mutex_type mutex)
{
	int rc = 0;

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		rc = CloseHandle(mutex);
	#else
		rc = pthread_mutex_destroy(mutex);
		free(mutex);
	#endif
	FUNC_EXIT_RC(rc);
}


/**
 * Get the thread id of the thread from which this function is called
 * @return thread id, type varying according to OS
 */
thread_id_type Thread_getid()
{
	#if defined(WIN32) || defined(WIN64)
		return GetCurrentThreadId();
	#else
		return pthread_self();
	#endif
}


#if defined(USE_NAMED_SEMAPHORES)
#define MAX_NAMED_SEMAPHORES 10

static int named_semaphore_count = 0;

static struct 
{
	sem_type sem;
	char name[NAME_MAX-4];
} named_semaphores[MAX_NAMED_SEMAPHORES];
 
#endif


/**
 * Create a new semaphore
 * @return the new condition variable
 */
sem_type Thread_create_sem()
{
	sem_type sem = NULL;
	int rc = 0;

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		sem = CreateEvent(
		        NULL,               // default security attributes
		        FALSE,              // manual-reset event?
		        FALSE,              // initial state is nonsignaled
		        NULL                // object name
		        );
    #elif defined(USE_NAMED_SEMAPHORES)
    	if (named_semaphore_count == 0)
    		memset(named_semaphores, '\0', sizeof(named_semaphores));
    	char* name = &(strrchr(tempnam("/", "MQTT"), '/'))[1]; /* skip first slash of name */
    	if ((sem = sem_open(name, O_CREAT, S_IRWXU, 0)) == SEM_FAILED)
    		rc = -1;
    	else
    	{
    		int i;
    		
    		named_semaphore_count++;
    		for (i = 0; i < MAX_NAMED_SEMAPHORES; ++i)
    		{
    			if (named_semaphores[i].name[0] == '\0')
    			{ 
    				named_semaphores[i].sem = sem;
    				strcpy(named_semaphores[i].name, name);	
    				break;
    			}
    		}
    	}
	#else
		sem = malloc(sizeof(sem_t));
		rc = sem_init(sem, 0, 0);
	#endif
	FUNC_EXIT_RC(rc);
	return sem;
}


/**
 * Wait for a semaphore to be posted, or timeout.
 * @param sem the semaphore
 * @param timeout the maximum time to wait, in milliseconds
 * @return completion code
 */
int Thread_wait_sem(sem_type sem, int timeout)
{
/* sem_timedwait is the obvious call to use, but seemed not to work on the Viper,
 * so I've used trywait in a loop instead. Ian Craggs 23/7/2010
 */
	int rc = -1;
#if !defined(WIN32) && !defined(WIN64)
#define USE_TRYWAIT
#if defined(USE_TRYWAIT)
	int i = 0;
	int interval = 10000; /* 10000 microseconds: 10 milliseconds */
	int count = (1000 * timeout) / interval; /* how many intervals in timeout period */
#else
	struct timespec ts;
#endif
#endif

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		rc = WaitForSingleObject(sem, timeout);
	#elif defined(USE_TRYWAIT)
		while (++i < count && (rc = sem_trywait(sem)) != 0)
		{
			if (rc == -1 && ((rc = errno) != EAGAIN))
			{
				rc = 0;
				break;
			}
			usleep(interval); /* microseconds - .1 of a second */
		}
	#else
		if (clock_gettime(CLOCK_REALTIME, &ts) != -1)
		{
			ts.tv_sec += timeout;
			rc = sem_timedwait(sem, &ts);
		}
	#endif

 	FUNC_EXIT_RC(rc);
 	return rc;
}


/**
 * Check to see if a semaphore has been posted, without waiting.
 * @param sem the semaphore
 * @return 0 (false) or 1 (true)
 */
int Thread_check_sem(sem_type sem)
{
#if defined(WIN32) || defined(WIN64)
	return WaitForSingleObject(sem, 0) == WAIT_OBJECT_0;
#else
	int semval = -1;
	sem_getvalue(sem, &semval);
	return semval > 0;
#endif
}


/**
 * Post a semaphore
 * @param sem the semaphore
 * @return completion code
 */
int Thread_post_sem(sem_type sem)
{
	int rc = 0;

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		if (SetEvent(sem) == 0)
			rc = GetLastError();
	#else
		if (sem_post(sem) == -1)
			rc = errno;
	#endif

 	FUNC_EXIT_RC(rc);
  return rc;
}


/**
 * Destroy a semaphore which has already been created
 * @param sem the semaphore
 */
int Thread_destroy_sem(sem_type sem)
{
	int rc = 0;

	FUNC_ENTRY;
	#if defined(WIN32) || defined(WIN64)
		rc = CloseHandle(sem);
    #elif defined(USE_NAMED_SEMAPHORES)
    	int i;
    	rc = sem_close(sem);
    	for (i = 0; i < MAX_NAMED_SEMAPHORES; ++i)
    	{
    		if (named_semaphores[i].sem == sem)
    		{ 
    			rc = sem_unlink(named_semaphores[i].name);
    			named_semaphores[i].name[0] = '\0';	
    			break;
    		}
    	}
    	named_semaphore_count--;
	#else
		rc = sem_destroy(sem);
		free(sem);
	#endif
	FUNC_EXIT_RC(rc);
	return rc;
}


#if !defined(WIN32) && !defined(WIN64)
/**
 * Create a new condition variable
 * @return the condition variable struct
 */
cond_type Thread_create_cond()
{
	cond_type condvar = NULL;
	int rc = 0;

	FUNC_ENTRY;
	condvar = malloc(sizeof(cond_type_struct));
	rc = pthread_cond_init(&condvar->cond, NULL);
	rc = pthread_mutex_init(&condvar->mutex, NULL);

	FUNC_EXIT_RC(rc);
	return condvar;
}

/**
 * Signal a condition variable
 * @return completion code
 */
int Thread_signal_cond(cond_type condvar)
{
	int rc = 0;

	pthread_mutex_lock(&condvar->mutex);
	rc = pthread_cond_signal(&condvar->cond);
	pthread_mutex_unlock(&condvar->mutex);

	return rc;
}

/**
 * Wait with a timeout (seconds) for condition variable
 * @return completion code
 */
int Thread_wait_cond(cond_type condvar, int timeout)
{
	FUNC_ENTRY;
	int rc = 0;
	struct timespec cond_timeout;
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);

	cond_timeout.tv_sec = cur_time.tv_sec + timeout;
	cond_timeout.tv_nsec = cur_time.tv_usec * 1000;

	pthread_mutex_lock(&condvar->mutex);
	rc = pthread_cond_timedwait(&condvar->cond, &condvar->mutex, &cond_timeout);
	pthread_mutex_unlock(&condvar->mutex);

	FUNC_EXIT_RC(rc);
	return rc;
}

/**
 * Destroy a condition variable
 * @return completion code
 */
int Thread_destroy_cond(cond_type condvar)
{
	int rc = 0;

	rc = pthread_mutex_destroy(&condvar->mutex);
	rc = pthread_cond_destroy(&condvar->cond);
	free(condvar);

	return rc;
}
#endif


#if defined(THREAD_UNIT_TESTS)

#include <stdio.h>

thread_return_type secondary(void* n)
{
	int rc = 0;

	/*
	cond_type cond = n;

	printf("Secondary thread about to wait\n");
	rc = Thread_wait_cond(cond);
	printf("Secondary thread returned from wait %d\n", rc);*/

	sem_type sem = n;

	printf("Secondary thread about to wait\n");
	rc = Thread_wait_sem(sem);
	printf("Secondary thread returned from wait %d\n", rc);

	printf("Secondary thread about to wait\n");
	rc = Thread_wait_sem(sem);
	printf("Secondary thread returned from wait %d\n", rc);
	printf("Secondary check sem %d\n", Thread_check_sem(sem));

	return 0;
}


int main(int argc, char *argv[])
{
	int rc = 0;

	sem_type sem = Thread_create_sem();

	printf("check sem %d\n", Thread_check_sem(sem));

	printf("post secondary\n");
	rc = Thread_post_sem(sem);
	printf("posted secondary %d\n", rc);

	printf("check sem %d\n", Thread_check_sem(sem));

	printf("Starting secondary thread\n");
	Thread_start(secondary, (void*)sem);

	sleep(3);
	printf("check sem %d\n", Thread_check_sem(sem));

	printf("post secondary\n");
	rc = Thread_post_sem(sem);
	printf("posted secondary %d\n", rc);

	sleep(3);

	printf("Main thread ending\n");
}

#endif
