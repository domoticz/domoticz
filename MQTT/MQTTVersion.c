/*******************************************************************************
 * Copyright (c) 2012, 2014 IBM Corp.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <memory.h>
#include <ctype.h>
#include "MQTTAsync.h"

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#endif


/**
 *
 * @file
 * \brief MQTTVersion - display the version and build information strings for a library.
 *
 * With no arguments, we try to load and call the version string for the libraries we 
 * know about: mqttv3c, mqttv3cs, mqttv3a, mqttv3as.
 * With an argument:
 *   1) we try to load the named library, call getVersionInfo and display those values. 
 *   2) If that doesn't work, we look through the binary for eyecatchers, and display those.  
 *      This will work if the library is not executable in the current environment.
 *
 * */
 
 
 char* libraries[] = {"paho-mqtt3c", "paho-mqtt3cs", "paho-mqtt3a", "paho-mqtt3as"};
 char* eyecatchers[] = {"MQTTAsyncV3_Version", "MQTTAsyncV3_Timestamp", 
 					 "MQTTClientV3_Version", "MQTTClientV3_Timestamp"};
 

/**
 * Finds an eyecatcher in a binary file and returns the following value.
 * @param filename the name of the file
 * @param eyecatcher_input the eyecatcher string to look for
 * @return the value found - "" if not found 
 */
char* FindString(char* filename, char* eyecatcher_input)
{
	FILE* infile = NULL;
	static char value[100];
	char* eyecatcher = eyecatcher_input;
	
	memset(value, 0, 100);
	if ((infile = fopen(filename, "rb")) != NULL)
	{
		int buflen = strlen(eyecatcher);
		char* buffer = (char*) malloc(buflen);
		int count = 0;
		int c = fgetc(infile);

		while (feof(infile) == 0)
		{
			buffer[count++] = c;
			if (memcmp(eyecatcher, buffer, buflen) == 0)
			{
				char* ptr = value;
				c = fgetc(infile); /* skip space */
				c = fgetc(infile);
				while (isprint(c))
				{
					*ptr++ = c;
					c = fgetc(infile);
				}
				break;
			}
			if (count == buflen)
			{
				memmove(buffer, &buffer[1], buflen - 1);
				count--;
			}
			c = fgetc(infile);
		}
		free(buffer);
	}
	return value;
}


int printVersionInfo(MQTTAsync_nameValue* info)
{
	int rc = 0;
	
	while (info->name)
	{
		printf("%s: %s\n", info->name, info->value);
		info++;
		rc = 1;  /* at least one value printed */
	}
	return rc;
}

typedef MQTTAsync_nameValue* (*func_type)(void);

int loadandcall(char* libname)
{
	int rc = 0;
	MQTTAsync_nameValue* (*func_address)(void) = NULL;
#if defined(WIN32) || defined(WIN64)
	wchar_t wlibname[30];
	HMODULE APILibrary;

	mbstowcs(wlibname, libname, strlen(libname) + 1);
	if ((APILibrary = LoadLibrary(wlibname)) == NULL)
		printf("Error loading library %s, error code %d\n", libname, GetLastError());
	else
	{
		func_address = (func_type)GetProcAddress(APILibrary, "MQTTAsync_getVersionInfo");
		if (func_address == NULL) 
			func_address = (func_type)GetProcAddress(APILibrary, "MQTTClient_getVersionInfo");
		if (func_address)
			rc = printVersionInfo((*func_address)());
		FreeLibrary(APILibrary);
	}
#else
	void* APILibrary = dlopen(libname, RTLD_LAZY); /* Open the Library in question */
	char* ErrorOutput = dlerror(); 	               /* Check it opened properly */
	if (ErrorOutput != NULL)
		printf("Error loading library %s, error %s\n", libname, ErrorOutput);
	else
	{	
		*(void **) (&func_address) = dlsym(APILibrary, "MQTTAsync_getVersionInfo");
		if (func_address == NULL)
			func_address = dlsym(APILibrary, "MQTTClient_getVersionInfo");
		if (func_address)
			rc = printVersionInfo((*func_address)());
		dlclose(APILibrary);
	}
#endif
	return rc;
}
 
	
#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

void printEyecatchers(char* filename)
{
	int i = 0;
	
	for (i = 0; i < ARRAY_SIZE(eyecatchers); ++i)
	{
		char* value = FindString(filename, eyecatchers[i]);
		if (value[0]) 
			printf("%s: %s\n", eyecatchers[i], value);
	}
}


int main(int argc, char** argv)
{
	printf("MQTTVersion: print the version strings of an MQTT client library\n"); 
	printf("Copyright (c) 2013 IBM Corp.\n"); 
	
	if (argc == 1)
	{
		int i = 0;
		char namebuf[60];
		
		printf("Specify a particular library name if it is not in the current directory, or not executable on this platform\n");
		 
		for (i = 0; i < ARRAY_SIZE(libraries); ++i)
		{
#if defined(WIN32) || defined(WIN64)
			sprintf(namebuf, "%s.dll", libraries[i]);
#else
			sprintf(namebuf, "lib%s.so.1", libraries[i]);
#endif
			printf("--- Trying library %s ---\n", libraries[i]);
			if (!loadandcall(namebuf))
				printEyecatchers(namebuf);
		}
	}
	else
	{
		if (!loadandcall(argv[1]))
			printEyecatchers(argv[1]);
	}

	return 0;
}
