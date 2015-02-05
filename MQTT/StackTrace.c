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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/
#include "StackTrace.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32) || defined(WIN64)
#define snprintf _snprintf
#endif

/*BE
def STACKENTRY
{
	n32 ptr STRING open "name"
	n32 dec "line"
}

defList(STACKENTRY)
BE*/

#define MAX_STACK_DEPTH 50
#define MAX_FUNCTION_NAME_LENGTH 30
#define MAX_THREADS 255

typedef struct
{
	thread_id_type threadid;
	char name[MAX_FUNCTION_NAME_LENGTH];
	int line;
} stackEntry;

typedef struct
{
	thread_id_type id;
	int maxdepth;
	int current_depth;
	stackEntry callstack[MAX_STACK_DEPTH];
} threadEntry;

static int thread_count = 0;
static threadEntry threads[MAX_THREADS];
static threadEntry *cur_thread = NULL;

#if defined(WIN32) || defined(WIN64)
mutex_type stack_mutex;
#else
static pthread_mutex_t stack_mutex_store = PTHREAD_MUTEX_INITIALIZER;
static mutex_type stack_mutex = &stack_mutex_store;
#endif


int setStack(int create)
{
	int i = -1;
	thread_id_type curid = Thread_getid();

	cur_thread = NULL;
	for (i = 0; i < MAX_THREADS && i < thread_count; ++i)
	{
		if (threads[i].id == curid)
		{
			cur_thread = &threads[i];
			break;
		}
	}

	if (cur_thread == NULL && create && thread_count < MAX_THREADS)
	{
		cur_thread = &threads[thread_count];
		cur_thread->id = curid;
		cur_thread->maxdepth = 0;
		cur_thread->current_depth = 0;
		++thread_count;
	}
	return cur_thread != NULL; /* good == 1 */
}

void StackTrace_entry(const char* name, int line, int trace_level)
{
	Thread_lock_mutex(stack_mutex);
	if (!setStack(1))
		goto exit;
	if (trace_level != -1)
		Log_stackTrace(trace_level, 9, (int)cur_thread->id, cur_thread->current_depth, name, line, NULL);
	strncpy(cur_thread->callstack[cur_thread->current_depth].name, name, sizeof(cur_thread->callstack[0].name)-1);
	cur_thread->callstack[(cur_thread->current_depth)++].line = line;
	if (cur_thread->current_depth > cur_thread->maxdepth)
		cur_thread->maxdepth = cur_thread->current_depth;
	if (cur_thread->current_depth >= MAX_STACK_DEPTH)
		Log(LOG_FATAL, -1, "Max stack depth exceeded");
exit:
	Thread_unlock_mutex(stack_mutex);
}


void StackTrace_exit(const char* name, int line, void* rc, int trace_level)
{
	Thread_lock_mutex(stack_mutex);
	if (!setStack(0))
		goto exit;
	if (--(cur_thread->current_depth) < 0)
		Log(LOG_FATAL, -1, "Minimum stack depth exceeded for thread %lu", cur_thread->id);
	if (strncmp(cur_thread->callstack[cur_thread->current_depth].name, name, sizeof(cur_thread->callstack[0].name)-1) != 0)
		Log(LOG_FATAL, -1, "Stack mismatch. Entry:%s Exit:%s\n", cur_thread->callstack[cur_thread->current_depth].name, name);
	if (trace_level != -1)
	{
		if (rc == NULL)
			Log_stackTrace(trace_level, 10, (int)cur_thread->id, cur_thread->current_depth, name, line, NULL);
		else
			Log_stackTrace(trace_level, 11, (int)cur_thread->id, cur_thread->current_depth, name, line, (int*)rc);
	}
exit:
	Thread_unlock_mutex(stack_mutex);
}


void StackTrace_printStack(FILE* dest)
{
	FILE* file = stdout;
	int t = 0;

	if (dest)
		file = dest;
	for (t = 0; t < thread_count; ++t)
	{
		threadEntry *cur_thread = &threads[t];

		if (cur_thread->id > 0)
		{
			int i = cur_thread->current_depth - 1;

			fprintf(file, "=========== Start of stack trace for thread %lu ==========\n", (unsigned long)cur_thread->id);
			if (i >= 0)
			{
				fprintf(file, "%s (%d)\n", cur_thread->callstack[i].name, cur_thread->callstack[i].line);
				while (--i >= 0)
					fprintf(file, "   at %s (%d)\n", cur_thread->callstack[i].name, cur_thread->callstack[i].line);
			}
			fprintf(file, "=========== End of stack trace for thread %lu ==========\n\n", (unsigned long)cur_thread->id);
		}
	}
	if (file != stdout && file != stderr && file != NULL)
		fclose(file);
}


char* StackTrace_get(thread_id_type threadid)
{
	int bufsize = 256;
	char* buf = NULL;
	int t = 0;

	if ((buf = malloc(bufsize)) == NULL)
		goto exit;
	buf[0] = '\0';
	for (t = 0; t < thread_count; ++t)
	{
		threadEntry *cur_thread = &threads[t];

		if (cur_thread->id == threadid)
		{
			int i = cur_thread->current_depth - 1;
			int curpos = 0;

			if (i >= 0)
			{
				curpos += snprintf(&buf[curpos], bufsize - curpos -1,
						"%s (%d)\n", cur_thread->callstack[i].name, cur_thread->callstack[i].line);
				while (--i >= 0)
					curpos += snprintf(&buf[curpos], bufsize - curpos -1,
							"   at %s (%d)\n", cur_thread->callstack[i].name, cur_thread->callstack[i].line);
				if (buf[--curpos] == '\n')
					buf[curpos] = '\0';
			}
			break;
		}
	}
exit:
	return buf;
}

