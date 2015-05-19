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
 *    Ian Craggs - async client updates
 *******************************************************************************/
/**
 * @file
 * \brief A file system based persistence implementation.
 *
 * A directory is specified when the MQTT client is created. When the persistence is then
 * opened (see ::Persistence_open), a sub-directory is made beneath the base for this
 * particular client ID and connection key. This allows one persistence base directory to
 * be shared by multiple clients.
 *
 */

#if !defined(NO_PERSISTENCE)

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if defined(WIN32) || defined(WIN64)
	#include <direct.h>
	/* Windows doesn't have strtok_r, so remap it to strtok */
	#define strtok_r( A, B, C ) strtok( A, B )
	int keysWin32(char *, char ***, int *);
	int clearWin32(char *);
	int containskeyWin32(char *, char *);
#else
	#include <sys/stat.h>
	#include <dirent.h>
	#include <unistd.h>
	int keysUnix(char *, char ***, int *);
	int clearUnix(char *);
	int containskeyUnix(char *, char *);
#endif

#include "MQTTClientPersistence.h"
#include "MQTTPersistenceDefault.h"
#include "StackTrace.h"
#include "Heap.h"


/** Create persistence directory for the client: context/clientID-serverURI.
 *  See ::Persistence_open
 */

int pstopen(void **handle, const char* clientID, const char* serverURI, void* context)
{
	int rc = 0;
	char *dataDir = context;
	char *clientDir;
	char *pToken = NULL;
	char *save_ptr = NULL;
	char *pCrtDirName = NULL;
	char *pTokDirName = NULL;
	char *perserverURI = NULL, *ptraux;

	FUNC_ENTRY;
	/* Note that serverURI=address:port, but ":" not allowed in Windows directories */
	perserverURI = malloc(strlen(serverURI) + 1);
	strcpy(perserverURI, serverURI);
	ptraux = strstr(perserverURI, ":");
	*ptraux = '-' ;

	/* consider '/'  +  '-'  +  '\0' */
	clientDir = malloc(strlen(dataDir) + strlen(clientID) + strlen(perserverURI) + 3);
	sprintf(clientDir, "%s/%s-%s", dataDir, clientID, perserverURI);


	/* create clientDir directory */

	/* pCrtDirName - holds the directory name we are currently trying to create.           */
	/*               This gets built up level by level until the full path name is created.*/
	/* pTokDirName - holds the directory name that gets used by strtok.         */
	pCrtDirName = (char*)malloc( strlen(clientDir) + 1 );
	pTokDirName = (char*)malloc( strlen(clientDir) + 1 );
	strcpy( pTokDirName, clientDir );

	pToken = strtok_r( pTokDirName, "\\/", &save_ptr );

	strcpy( pCrtDirName, pToken );
	rc = pstmkdir( pCrtDirName );
	pToken = strtok_r( NULL, "\\/", &save_ptr );
	while ( (pToken != NULL) && (rc == 0) )
	{
		/* Append the next directory level and try to create it */
		sprintf( pCrtDirName, "%s/%s", pCrtDirName, pToken );
		rc = pstmkdir( pCrtDirName );
		pToken = strtok_r( NULL, "\\/", &save_ptr );
	}

	*handle = clientDir;

	free(perserverURI);
	free(pTokDirName);
	free(pCrtDirName);

	FUNC_EXIT_RC(rc);
	return rc;
}

/** Function to create a directory.
 * Returns 0 on success or if the directory already exists.
 */
int pstmkdir( char *pPathname )
{
	int rc = 0;

	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	if ( _mkdir( pPathname ) != 0 )
	{
#else
	/* Create a directory with read, write and execute access for the owner and read access for the group */
	if ( mkdir( pPathname, S_IRWXU | S_IRGRP ) != 0 )
	{
#endif
		if ( errno != EEXIST )
			rc = MQTTCLIENT_PERSISTENCE_ERROR;
	}

	FUNC_EXIT_RC(rc);
	return rc;
}



/** Write wire message to the client persistence directory.
 *  See ::Persistence_put
 */
int pstput(void* handle, char* key, int bufcount, char* buffers[], int buflens[])
{
	int rc = 0;
	char *clientDir = handle;
	char *file;
	FILE *fp;
	int bytesWritten = 0;
	int bytesTotal = 0;
	int i;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	/* consider '/' + '\0' */
	file = malloc(strlen(clientDir) + strlen(key) + strlen(MESSAGE_FILENAME_EXTENSION) + 2 );
	sprintf(file, "%s/%s%s", clientDir, key, MESSAGE_FILENAME_EXTENSION);

	fp = fopen(file, "wb");
	if ( fp != NULL )
	{
		for(i=0; i<bufcount; i++)
		{
			bytesTotal += buflens[i];
			bytesWritten += fwrite( buffers[i], sizeof(char), buflens[i], fp );
		}
		fclose(fp);
		fp = NULL;
	} else
		rc = MQTTCLIENT_PERSISTENCE_ERROR;

	if ( bytesWritten != bytesTotal )
	{
		pstremove(handle, key);
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
	}

	free(file);

exit:
	FUNC_EXIT_RC(rc);
	return rc;
};


/** Retrieve a wire message from the client persistence directory.
 *  See ::Persistence_get
 */
int pstget(void* handle, char* key, char** buffer, int* buflen)
{
	int rc = 0;
	FILE *fp;
	char *clientDir = handle;
	char *file;
	char *buf;
	unsigned long fileLen = 0;
	unsigned long bytesRead = 0;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	/* consider '/' + '\0' */
	file = malloc(strlen(clientDir) + strlen(key) + strlen(MESSAGE_FILENAME_EXTENSION) + 2);
	sprintf(file, "%s/%s%s", clientDir, key, MESSAGE_FILENAME_EXTENSION);

	fp = fopen(file, "rb");
	if ( fp != NULL )
	{
		fseek(fp, 0, SEEK_END);
		fileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf=(char *)malloc(fileLen);
		bytesRead = fread(buf, sizeof(char), fileLen, fp);
		*buffer = buf;
		*buflen = bytesRead;
		if ( bytesRead != fileLen )
			rc = MQTTCLIENT_PERSISTENCE_ERROR;
		fclose(fp);
		fp = NULL;
	} else
		rc = MQTTCLIENT_PERSISTENCE_ERROR;

	free(file);
	/* the caller must free buf */

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



/** Delete a persisted message from the client persistence directory.
 *  See ::Persistence_remove
 */
int pstremove(void* handle, char* key)
{
	int rc = 0;
	char *clientDir = handle;
	char *file;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		return rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	/* consider '/' + '\0' */
	file = malloc(strlen(clientDir) + strlen(key) + strlen(MESSAGE_FILENAME_EXTENSION) + 2);
	sprintf(file, "%s/%s%s", clientDir, key, MESSAGE_FILENAME_EXTENSION);

#if defined(WIN32) || defined(WIN64)
	if ( _unlink(file) != 0 )
	{
#else
	if ( unlink(file) != 0 )
	{
#endif
		if ( errno != ENOENT )
			rc = MQTTCLIENT_PERSISTENCE_ERROR;
	}

	free(file);

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/** Delete client persistence directory (if empty).
 *  See ::Persistence_close
 */
int pstclose(void* handle)
{
	int rc = 0;
	char *clientDir = handle;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

#if defined(WIN32) || defined(WIN64)
	if ( _rmdir(clientDir) != 0 )
	{
#else
	if ( rmdir(clientDir) != 0 )
	{
#endif
		if ( errno != ENOENT && errno != ENOTEMPTY )
			rc = MQTTCLIENT_PERSISTENCE_ERROR;
	}

	free(clientDir);

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/** Returns whether if a wire message is persisted in the client persistence directory.
 * See ::Persistence_containskey
 */
int pstcontainskey(void *handle, char *key)
{
	int rc = 0;
	char *clientDir = handle;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

#if defined(WIN32) || defined(WIN64)
	rc = containskeyWin32(clientDir, key);
#else
	rc = containskeyUnix(clientDir, key);
#endif

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


#if defined(WIN32) || defined(WIN64)
int containskeyWin32(char *dirname, char *key)
{
	int notFound = MQTTCLIENT_PERSISTENCE_ERROR;
	int fFinished = 0;
	char *filekey, *ptraux;
	char dir[MAX_PATH+1];
	WIN32_FIND_DATAA FileData;
	HANDLE hDir;

	FUNC_ENTRY;
	sprintf(dir, "%s/*", dirname);

	hDir = FindFirstFileA(dir, &FileData);
	if (hDir != INVALID_HANDLE_VALUE)
	{
		while (!fFinished)
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
				filekey = malloc(strlen(FileData.cFileName) + 1);
				strcpy(filekey, FileData.cFileName);
				ptraux = strstr(filekey, MESSAGE_FILENAME_EXTENSION);
				if ( ptraux != NULL )
					*ptraux = '\0' ;
				if(strcmp(filekey, key) == 0)
				{
					notFound = 0;
					fFinished = 1;
				}
				free(filekey);
			}
			if (!FindNextFileA(hDir, &FileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					fFinished = 1;
			}
		}
		FindClose(hDir);
	}

	FUNC_EXIT_RC(notFound);
	return notFound;
}
#else
int containskeyUnix(char *dirname, char *key)
{
	int notFound = MQTTCLIENT_PERSISTENCE_ERROR;
	char *filekey, *ptraux;
	DIR *dp;
	struct dirent *dir_entry;
	struct stat stat_info;

	FUNC_ENTRY;
	if((dp = opendir(dirname)) != NULL)
	{
		while((dir_entry = readdir(dp)) != NULL && notFound)
		{
			char* filename = malloc(strlen(dirname) + strlen(dir_entry->d_name) + 2);
			sprintf(filename, "%s/%s", dirname, dir_entry->d_name);
			lstat(filename, &stat_info);
			free(filename);
			if(S_ISREG(stat_info.st_mode))
			{
				filekey = malloc(strlen(dir_entry->d_name) + 1);
				strcpy(filekey, dir_entry->d_name);
				ptraux = strstr(filekey, MESSAGE_FILENAME_EXTENSION);
				if ( ptraux != NULL )
					*ptraux = '\0' ;
				if(strcmp(filekey, key) == 0)
					notFound = 0;
				free(filekey);
			}
		}
		closedir(dp);
	}

	FUNC_EXIT_RC(notFound);
	return notFound;
}
#endif


/** Delete all the persisted message in the client persistence directory.
 * See ::Persistence_clear
 */
int pstclear(void *handle)
{
	int rc = 0;
	char *clientDir = handle;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

#if defined(WIN32) || defined(WIN64)
	rc = clearWin32(clientDir);
#else
	rc = clearUnix(clientDir);
#endif

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


#if defined(WIN32) || defined(WIN64)
int clearWin32(char *dirname)
{
	int rc = 0;
	int fFinished = 0;
	char *file;
	char dir[MAX_PATH+1];
	WIN32_FIND_DATAA FileData;
	HANDLE hDir;

	FUNC_ENTRY;
	sprintf(dir, "%s/*", dirname);

	hDir = FindFirstFileA(dir, &FileData);
	if (hDir != INVALID_HANDLE_VALUE)
	{
		while (!fFinished)
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
				file = malloc(strlen(dirname) + strlen(FileData.cFileName) + 2);
				sprintf(file, "%s/%s", dirname, FileData.cFileName);
				rc = remove(file);
				free(file);
				if ( rc != 0 )
				{
					rc = MQTTCLIENT_PERSISTENCE_ERROR;
					break;
				}
			}
			if (!FindNextFileA(hDir, &FileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					fFinished = 1;
			}
		}
		FindClose(hDir);
	} else
		rc = MQTTCLIENT_PERSISTENCE_ERROR;

	FUNC_EXIT_RC(rc);
	return rc;
}
#else
int clearUnix(char *dirname)
{
	int rc = 0;
	DIR *dp;
	struct dirent *dir_entry;
	struct stat stat_info;

	FUNC_ENTRY;
	if((dp = opendir(dirname)) != NULL)
	{
		while((dir_entry = readdir(dp)) != NULL && rc == 0)
		{
			lstat(dir_entry->d_name, &stat_info);
			if(S_ISREG(stat_info.st_mode))
			{
				if ( remove(dir_entry->d_name) != 0 )
					rc = MQTTCLIENT_PERSISTENCE_ERROR;
			}
		}
		closedir(dp);
	} else
		rc = MQTTCLIENT_PERSISTENCE_ERROR;

	FUNC_EXIT_RC(rc);
	return rc;
}
#endif


/** Returns the keys (file names w/o the extension) in the client persistence directory.
 *  See ::Persistence_keys
 */
int pstkeys(void *handle, char ***keys, int *nkeys)
{
	int rc = 0;
	char *clientDir = handle;

	FUNC_ENTRY;
	if (clientDir == NULL)
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

#if defined(WIN32) || defined(WIN64)
	rc = keysWin32(clientDir, keys, nkeys);
#else
	rc = keysUnix(clientDir, keys, nkeys);
#endif

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


#if defined(WIN32) || defined(WIN64)
int keysWin32(char *dirname, char ***keys, int *nkeys)
{
	int rc = 0;
	char **fkeys = NULL;
	int nfkeys = 0;
	char dir[MAX_PATH+1];
	WIN32_FIND_DATAA FileData;
	HANDLE hDir;
	int fFinished = 0;
	char *ptraux;
	int i;

	FUNC_ENTRY;
	sprintf(dir, "%s/*", dirname);

	/* get number of keys */
	hDir = FindFirstFileA(dir, &FileData);
	if (hDir != INVALID_HANDLE_VALUE)
	{
		while (!fFinished)
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
				nfkeys++;
			if (!FindNextFileA(hDir, &FileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					fFinished = 1;
			}
		}
		FindClose(hDir);
	} else
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	if (nfkeys != 0 )
		fkeys = (char **)malloc(nfkeys * sizeof(char *));

	/* copy the keys */
	hDir = FindFirstFileA(dir, &FileData);
	if (hDir != INVALID_HANDLE_VALUE)
	{
		fFinished = 0;
		i = 0;
		while (!fFinished)
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
				fkeys[i] = malloc(strlen(FileData.cFileName) + 1);
				strcpy(fkeys[i], FileData.cFileName);
				ptraux = strstr(fkeys[i], MESSAGE_FILENAME_EXTENSION);
				if ( ptraux != NULL )
					*ptraux = '\0' ;
				i++;
			}
			if (!FindNextFileA(hDir, &FileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					fFinished = 1;
			}
		}
		FindClose(hDir);
	} else
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	*nkeys = nfkeys;
	*keys = fkeys;
	/* the caller must free keys */

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
#else
int keysUnix(char *dirname, char ***keys, int *nkeys)
{
	int rc = 0;
	char **fkeys = NULL;
	int nfkeys = 0;
	char *ptraux;
	int i;
	DIR *dp;
	struct dirent *dir_entry;
	struct stat stat_info;

	FUNC_ENTRY;
	/* get number of keys */
	if((dp = opendir(dirname)) != NULL)
	{
		while((dir_entry = readdir(dp)) != NULL)
		{
			char* temp = malloc(strlen(dirname)+strlen(dir_entry->d_name)+2);

			sprintf(temp, "%s/%s", dirname, dir_entry->d_name);
			if (lstat(temp, &stat_info) == 0 && S_ISREG(stat_info.st_mode))
				nfkeys++;
			free(temp);
		}
		closedir(dp);
	} else
	{
		rc = MQTTCLIENT_PERSISTENCE_ERROR;
		goto exit;
	}

	if (nfkeys != 0)
	{
		fkeys = (char **)malloc(nfkeys * sizeof(char *));

		/* copy the keys */
		if((dp = opendir(dirname)) != NULL)
		{
			i = 0;
			while((dir_entry = readdir(dp)) != NULL)
			{
				char* temp = malloc(strlen(dirname)+strlen(dir_entry->d_name)+2);
	
				sprintf(temp, "%s/%s", dirname, dir_entry->d_name);
				if (lstat(temp, &stat_info) == 0 && S_ISREG(stat_info.st_mode))
				{
					fkeys[i] = malloc(strlen(dir_entry->d_name) + 1);
					strcpy(fkeys[i], dir_entry->d_name);
					ptraux = strstr(fkeys[i], MESSAGE_FILENAME_EXTENSION);
					if ( ptraux != NULL )
						*ptraux = '\0' ;
					i++;
				}
				free(temp);
			}
			closedir(dp);
		} else
		{
			rc = MQTTCLIENT_PERSISTENCE_ERROR;
			goto exit;
		}
	}

	*nkeys = nfkeys;
	*keys = fkeys;
	/* the caller must free keys */

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
#endif



#if defined(UNIT_TESTS)
int main (int argc, char *argv[])
{
#define MSTEM "m-"
#define NMSGS 10
#define NBUFS 4
#define NDEL 2
#define RC !rc ? "(Success)" : "(Failed) "

	int rc;
	char *handle;
	char *perdir = ".";
	char *clientID = "TheUTClient";
	char *serverURI = "127.0.0.1:1883";

	char *stem = MSTEM;
	int msgId, i;
	int nm[NDEL] = {5 , 8};  /* msgIds to get and remove */

	char *key;
	char **keys;
	int nkeys;
	char *buffer, *buff;
	int buflen;

	int nbufs = NBUFS;
	char *bufs[NBUFS] = {"m0", "mm1", "mmm2" , "mmmm3"};  /* message content */
	int buflens[NBUFS];
	for(i=0;i<nbufs;i++)
		buflens[i]=strlen(bufs[i]);

	/* open */
	//printf("Persistence directory : %s\n", perdir);
	rc = pstopen((void**)&handle, clientID, serverURI, perdir);
	printf("%s Persistence directory for client %s : %s\n", RC, clientID, handle);

	/* put */
	for(msgId=0;msgId<NMSGS;msgId++)
	{
		key = malloc(MESSAGE_FILENAME_LENGTH + 1);
		sprintf(key, "%s%d", stem, msgId);
		rc = pstput(handle, key, nbufs, bufs, buflens);
		printf("%s Adding message %s\n", RC, key);
		free(key);
	}

	/* keys ,ie, list keys added */
	rc = pstkeys(handle, &keys, &nkeys);
	printf("%s Found %d messages persisted in %s\n", RC, nkeys, handle);
	for(i=0;i<nkeys;i++)
		printf("%13s\n", keys[i]);

	if (keys !=NULL)
		free(keys);

	/* containskey */
	for(i=0;i<NDEL;i++)
	{
		key = malloc(MESSAGE_FILENAME_LENGTH + 1);
		sprintf(key, "%s%d", stem, nm[i]);
		rc = pstcontainskey(handle, key);
		printf("%s Message %s is persisted ?\n", RC, key);
		free(key);
	}

	/* get && remove*/
	for(i=0;i<NDEL;i++)
	{
		key = malloc(MESSAGE_FILENAME_LENGTH + 1);
		sprintf(key, "%s%d", stem, nm[i]);
		rc = pstget(handle, key, &buffer, &buflen);
		buff = malloc(buflen+1);
		memcpy(buff, buffer, buflen);
		buff[buflen] = '\0';
		printf("%s Retrieving message %s : %s\n", RC, key, buff);
		rc = pstremove(handle, key);
		printf("%s Removing message %s\n", RC, key);
		free(key);
		free(buff);
		free(buffer);
	}

	/* containskey */
	for(i=0;i<NDEL;i++)
	{
		key = malloc(MESSAGE_FILENAME_LENGTH + 1);
		sprintf(key, "%s%d", stem, nm[i]);
		rc = pstcontainskey(handle, key);
		printf("%s Message %s is persisted ?\n", RC, key);
		free(key);
	}

	/* keys ,ie, list keys added */
	rc = pstkeys(handle, &keys, &nkeys);
	printf("%s Found %d messages persisted in %s\n", RC, nkeys, handle);
	for(i=0;i<nkeys;i++)
		printf("%13s\n", keys[i]);

	if (keys != NULL)
		free(keys);


	/* close -> it will fail, since client persistence directory is not empty */
	rc = pstclose(&handle);
	printf("%s Closing client persistence directory for client %s\n", RC, clientID);

	/* clear */
	rc = pstclear(handle);
	printf("%s Deleting all persisted messages in %s\n", RC, handle);

	/* keys ,ie, list keys added */
	rc = pstkeys(handle, &keys, &nkeys);
	printf("%s Found %d messages persisted in %s\n", RC, nkeys, handle);
	for(i=0;i<nkeys;i++)
		printf("%13s\n", keys[i]);

	if ( keys != NULL )
		free(keys);

	/* close */
	rc = pstclose(&handle);
	printf("%s Closing client persistence directory for client %s\n", RC, clientID);
}
#endif


#endif /* NO_PERSISTENCE */
