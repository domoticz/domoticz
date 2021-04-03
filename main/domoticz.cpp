/*
 Domoticz, Open Source Home Automation System

 Copyright (C) 2012,2020 Rob Peters (GizMoCuz)

 Domoticz is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published
 by the Free Software Foundation, either version 3 of the License,
 or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty
 of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Domoticz. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "mainworker.h"
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include "CmdLine.h"
#include "Logger.h"
#include "Helper.h"
#include "WebServerHelper.h"
#include "SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "appversion.h"
#include "localtime_r.h"
#include "SignalHandler.h"

#if defined WIN32
	#include "../msbuild/WindowsHelper.h"
	#include <Shlobj.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
	#include <syslog.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <string.h> 
#endif

namespace
{
	constexpr const char *szHelp
	{
		"Usage: Domoticz -www port\n"
		"\t-version display version number\n"
		"\t-www port (for example -www 8080, or -www 0 to disable http)\n"
		"\t-wwwbind address (for example -wwwbind 0.0.0.0 or -wwwbind 192.168.0.20)\n"
#ifdef WWW_ENABLE_SSL
		"\t-sslwww port (for example -sslwww 443, or -sslwww 0 to disable https)\n"
		"\t-sslcert file_path (for example /opt/domoticz/server_cert.pem)\n"
		"\t-sslkey file_path (if different from certificate file)\n"
		"\t-sslpass passphrase (to access to server private key in certificate)\n"
		"\t-sslmethod method (supported methods: tlsv1, tlsv1_server, sslv23, sslv23_server, tlsv11, tlsv11_server, tlsv12, tlsv12_server)\n"
		"\t-ssloptions options (for SSL options, default is 'default_workarounds,no_sslv2,no_sslv3,no_tlsv1,no_tlsv1_1,single_dh_use')\n"
		"\t-ssldhparam file_path (for SSL DH parameters)\n"
#endif
#if defined WIN32
		"\t-wwwroot file_path (for example D:\\www)\n"
		"\t-dbase file_path (for example D:\\domoticz.db)\n"
		"\t-userdata file_path (for example D:\\domoticzdata)\n"
		"\t-approot file_path (for example D:\\domoticz)\n"
#else
		"\t-wwwroot file_path (for example /opt/domoticz/www)\n"
		"\t-dbase file_path (for example /opt/domoticz/domoticz.db)\n"
		"\t-userdata file_path (for example /opt/domoticz)\n"
		"\t-approot file_path (for example /opt/domoticz)\n"
#endif
		"\t-webroot additional web root, useful with proxy servers (for example domoticz)\n"
		"\t-startupdelay seconds (default=0)\n"
		"\t-nowwwpwd (in case you forgot the web server username/password)\n"
		"\t-nocache (do not return appcache, use only when developing the web pages)\n"
		"\t-wwwcompress mode (on = always compress [default], off = always decompress, static = no processing but try precompressed first)\n"
#if defined WIN32
		"\t-nobrowser (do not start web browser (Windows Only)\n"
#endif
		"\t-noupdates do not use the internal update functionality\n"
#if defined WIN32
		"\t-log file_path (for example D:\\domoticz.log)\n"
#else
		"\t-log file_path (for example /var/log/domoticz.log)\n"
#endif
		"\t-loglevel (combination of: normal,status,error,debug)\n"
		"\t-debuglevel (combination of: normal,hardware,received,webserver,eventsystem,python,thread_id)\n"
		"\t-notimestamps (do not prepend timestamps to logs; useful with syslog, etc.)\n"
		"\t-php_cgi_path (for example /usr/bin/php-cgi)\n"
#ifndef WIN32
		"\t-daemon (run as background daemon)\n"
		"\t-pidfile pid file location (for example /var/run/domoticz.pid)\n"
		"\t-syslog [user|daemon|local0 .. local7] (use syslog as log output, defaults to facility 'user')\n"
		"\t-f config_file (for example /etc/domoticz.conf)\n"
#endif
		""
	};

#ifndef WIN32
	constexpr std::array<std::pair<const char *, int>, 10> facilities{
		{
			{ "daemon", LOG_DAEMON }, //
			{ "user", LOG_USER },	  //
			{ "local0", LOG_LOCAL0 }, //
			{ "local1", LOG_LOCAL1 }, //
			{ "local2", LOG_LOCAL2 }, //
			{ "local3", LOG_LOCAL3 }, //
			{ "local4", LOG_LOCAL4 }, //
			{ "local5", LOG_LOCAL5 }, //
			{ "local6", LOG_LOCAL6 }, //
			{ "local7", LOG_LOCAL7 }, //
		}				  //
	};
	std::string logfacname = "user";
#endif
} // namespace
std::string szStartupFolder;
std::string szUserDataFolder;
std::string szWWWFolder;
std::string szWebRoot;
std::string dbasefile;

/*
#define VCGENCMDTEMPCOMMAND "vcgencmd measure_temp"
#define VCGENCMDARMSPEEDCOMMAND "vcgencmd measure_clock arm"
#define VCGENCMDV3DSPEEDCOMMAND "vcgencmd measure_clock v3d"
#define VCGENCMDCORESPEEDCOMMAND "vcgencmd measure_clock core"

bool bHasInternalTemperature=false;
std::string szInternalTemperatureCommand = "";

bool bHasInternalClockSpeeds=false;
std::string szInternalARMSpeedCommand = "";
std::string szInternalV3DSpeedCommand = "";
std::string szInternalCoreSpeedCommand = "";

bool bHasInternalVoltage=false;
std::string szInternalVoltageCommand = "";

bool bHasInternalCurrent=false;
std::string szInternalCurrentCommand = "";
*/

std::string szAppVersion="???";
int iAppRevision=0;
std::string szAppHash="???";
std::string szAppDate="???";
std::string szPyVersion="None";
int ActYear;
time_t m_StartTime = time(nullptr);
std::string szRandomUUID = "???";

MainWorker m_mainworker;
CLogger _log;
http::server::CWebServerHelper m_webservers;
CSQLHelper m_sql;
CNotificationHelper m_notifications;

std::string logfile;
bool g_bStopApplication = false;
bool g_bUseSyslog = false;
bool g_bRunAsDaemon = false;
bool g_bDontCacheWWW = false;
http::server::_eWebCompressionMode g_wwwCompressMode = http::server::WWW_USE_GZIP;
bool g_bUseUpdater = true;
http::server::server_settings webserver_settings;
#ifdef WWW_ENABLE_SSL
http::server::ssl_server_settings secure_webserver_settings;
#endif
bool bStartWebBrowser = true;
bool g_bUseWatchdog = true;

#define DAEMON_NAME "domoticz"
#define PID_FILE "/var/run/domoticz.pid" 

std::string daemonname = DAEMON_NAME;
std::string pidfile = PID_FILE;
int pidFilehandle = 0;

#ifndef WIN32
void daemonShutdown()
{
	if (pidFilehandle != 0) {
		close(pidFilehandle);
		pidFilehandle = 0;
	}
}

void daemonize(const char *rundir, const char *pidfile)
{
	int pid, sid, i;
	char str[10];
	struct sigaction newSigAction;
	sigset_t newSigSet;

	/* Set signal mask - signals we want to block */
	sigemptyset(&newSigSet);
	sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
	sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
	sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
	sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
	sigprocmask(SIG_BLOCK, &newSigSet, nullptr); /* Block the above specified signals */

	/* Set up a signal handler */
	newSigAction.sa_sigaction = signal_handler;
	sigemptyset(&newSigAction.sa_mask);
	newSigAction.sa_flags = SA_SIGINFO;

	/* Signals to handle */
	sigaction(SIGTERM, &newSigAction, nullptr); // catch term signal
	sigaction(SIGINT, &newSigAction, nullptr);  // catch interrupt signal
	sigaction(SIGSEGV, &newSigAction, nullptr); // catch segmentation fault signal
	sigaction(SIGABRT, &newSigAction, nullptr); // catch abnormal termination signal
	sigaction(SIGILL, &newSigAction, nullptr);  // catch invalid program image
	sigaction(SIGUSR1, &newSigAction, nullptr); // catch SIGUSR1 (used by watchdog)
	sigaction(SIGHUP, &newSigAction, nullptr); // catch HUP, for log rotation

	/* Fork*/
	pid = fork();

	if (pid < 0)
	{
		/* Could not fork */
		exit(EXIT_FAILURE);
	}
    
	if (pid > 0)
	{
		/* Child created ok, so exit parent process */
		exit(EXIT_SUCCESS);
	}

	/* Ensure only one copy */
	pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);

	if (pidFilehandle == -1)
	{
		/* Couldn't open lock file */
		syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Try to lock file */
	if (lockf(pidFilehandle, F_TLOCK, 0) == -1)
	{
		/* Couldn't get lock on lock file */
		syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Get and format PID */
	sprintf(str, "%d\n", getpid());

	/* write pid to lockfile */
	int twrite=write(pidFilehandle, str, strlen(str));
	if (twrite != int(strlen(str)))
	{
		syslog(LOG_INFO, "Could not write to lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}


	/* Child continues */

	umask(027); /* Set file permissions 750 */

	if (!logfile.empty())
		_log.SetOutputFile(logfile.c_str());

	/* Get a new process group */
	sid = setsid();

	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Route I/O connections */

	/* Open STDIN */
	i = open("/dev/null", O_RDWR);

	/* STDOUT */
	int dret = dup(i);
	if (dret == -1)
	{
		_log.Log(LOG_ERROR, "Could not set STDOUT descriptor !");
	}

	/* STDERR */
	dret = dup(i);
	if (dret == -1)
	{
		_log.Log(LOG_ERROR, "Could not set STDERR descriptor !");
	}

	int cdret = chdir(rundir); /* change running directory */
	if (cdret == -1)
	{
		_log.Log(LOG_ERROR, "Could not change running directory !");
	}
}
#endif

#if defined(_WIN32)
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		return GetModuleFileNameA(NULL, pathName, (DWORD)pathNameCapacity);
	}
#elif defined(__linux__) || defined(__CYGWIN32__) /* elif of: #if defined(_WIN32) */
	#include <unistd.h>
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		size_t pathNameSize = readlink("/proc/self/exe", pathName, pathNameCapacity - 1);
		pathName[pathNameSize] = '\0';
		return pathNameSize;
	}
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	#include <sys/sysctl.h>
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		int mib[4];
#ifdef __FreeBSD__
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PATHNAME;
		mib[3] = -1;
#else // __NetBSD__
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC_ARGS;
		mib[2] = getpid();
		mib[3] = KERN_PROC_PATHNAME;
#endif
		size_t cb = pathNameCapacity-1;
		sysctl(mib, 4, pathName, &cb, NULL, 0);
		return cb;
	}
#elif defined(__OpenBSD__)
#include <sys/sysctl.h>
static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
{
        int mib[4];
        char **argv;
        size_t len = 0;
        const char *comm;

        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC_ARGS;
        mib[2] = getpid();
        mib[3] = KERN_PROC_ARGV;
        pathName[0] = '\0';
        if (sysctl(mib, 4, NULL, &len, NULL, 0) < 0)
        {
                return 0;
        }

        if (!(argv = (char**)malloc(len)))
        {
                return 0;
        }

        if (sysctl(mib, 4, argv, &len, NULL, 0) < 0)
        {
                len = 0;
                goto finally;
        }

	        len = 0;
        comm = argv[0];

        if (*comm == '/' || *comm == '.')
        {
                // In OpenBSD PATH_MAX is 1024
                char * fullPath = (char*) malloc(PATH_MAX);
                if(!fullPath)
                {
                        goto finally;
                }

                if (realpath(comm, fullPath))
                {
                        if(pathNameCapacity > strnlen(fullPath, PATH_MAX))
                        {
                                strlcpy(pathName, fullPath, pathNameCapacity);
                                len = strnlen(pathName, pathNameCapacity);
                                free(fullPath);
                        }
                }
        }
        else
        {
                char *sp;
                char *xpath = strdup(getenv("PATH"));
                char *path = strtok_r(xpath, ":", &sp);
                struct stat st;

		if (!xpath)
                {
                        goto finally;
                }

                while (path)
                {
                        snprintf(pathName, pathNameCapacity, "%s/%s", path, comm);
                        if (!stat(pathName, &st) && (st.st_mode & S_IXUSR))
                        {
                                break;
                        }
                        pathName[0] = '\0';
                        path = strtok_r(NULL, ":", &sp);
                }
                free(xpath);
        }

  finally:
        free(argv);
        return len;
}
#elif defined(__APPLE__) /* elif of: #elif defined(__linux__) */
	#include <mach-o/dyld.h>
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		uint32_t pathNameSize = 0;

		_NSGetExecutablePath(NULL, &pathNameSize);

		if (pathNameSize > pathNameCapacity)
			pathNameSize = pathNameCapacity;

		if (!_NSGetExecutablePath(pathName, &pathNameSize))
		{
			char real[PATH_MAX];

			if (realpath(pathName, real) != NULL)
			{
				pathNameSize = strlen(real);
				strncpy(pathName, real, pathNameSize);
			}

			return pathNameSize;
		}

		return 0;
	}
#else /* else of: #elif defined(__APPLE__) */
	#error provide your own getExecutablePathName implementation
#endif /* end of: #if defined(_WIN32) */

void GetAppVersion()
{
	szAppVersion = VERSION_STRING;
	iAppRevision = APPVERSION;
	szAppHash = APPHASH;
	char szTmp[200];
	struct tm ltime;
	time_t btime = APPDATE;
	localtime_r(&btime, &ltime);
	ActYear = ltime.tm_year + 1900;
	strftime(szTmp, 200, "%Y-%m-%d %H:%M:%S", &ltime);
	szAppDate = szTmp;
}

bool GetConfigBool(std::string szValue)
{
	stdlower(szValue);
	return (szValue == "yes");
}

bool ParseConfigFile(const std::string &szConfigFile)
{
	std::ifstream infile;
	std::string sLine;
	infile.open(szConfigFile.c_str());
	if (!infile.is_open())
	{
		_log.Log(LOG_ERROR, "Could not open Configuration file '%s'",szConfigFile.c_str());
		return false;
	}
	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		sLine = stdstring_trim(sLine);
		if (sLine.empty())
			continue;
		if (sLine.find('#') == 0)
			continue; //Skip lines starting with # (Comments)

		size_t pos = sLine.find('=');
		if (pos == std::string::npos)
			continue; //invalid config line, should always contain xx=yy
		std::string szFlag = sLine.substr(0, pos);
		sLine = sLine.substr(pos + 1);

		szFlag = stdstring_trim(szFlag);
		sLine = stdstring_trim(sLine);

		if (szFlag == "http_port") {
			webserver_settings.listening_port = sLine;
		}
#ifdef WWW_ENABLE_SSL
		else if (szFlag == "ssl_port") {
			secure_webserver_settings.listening_port = sLine;
		}
		else if (szFlag == "ssl_cert") {
			secure_webserver_settings.cert_file_path = sLine;
		}
		else if (szFlag == "ssl_key") {
			secure_webserver_settings.private_key_file_path = sLine;
		}
		else if (szFlag == "ssl_dhparam") {
			secure_webserver_settings.tmp_dh_file_path = sLine;
		}
		else if (szFlag == "ssl_pass") {
			secure_webserver_settings.private_key_pass_phrase = sLine;
		}
		else if (szFlag == "ssl_method") {
			secure_webserver_settings.ssl_method = sLine;
		}
		else if (szFlag == "ssl_options") {
			secure_webserver_settings.ssl_options = sLine;
		}
#endif
		else if (szFlag == "http_root") {
			szWWWFolder = sLine;
		}
		else if (szFlag == "web_root") {
			szWebRoot = sLine;
		}
		else if (szFlag == "www_compress_mode") {
			if (sLine == "on")
				g_wwwCompressMode = http::server::WWW_USE_GZIP;
			else if (sLine == "off")
				g_wwwCompressMode = http::server::WWW_FORCE_NO_GZIP_SUPPORT;
			else if (sLine == "static")
				g_wwwCompressMode = http::server::WWW_USE_STATIC_GZ_FILES;
			else {
				_log.Log(LOG_ERROR, "Invalid www_compress_mode value in Configuration file '%s'", szConfigFile.c_str());
				return false;
			}
		}
		else if (szFlag == "cache") {
			g_bDontCacheWWW = !GetConfigBool(sLine);
		}
		else if (szFlag == "reset_password") {
			m_mainworker.m_bIgnoreUsernamePassword = GetConfigBool(sLine);
		}
		else if (szFlag == "log_file") {
			logfile = sLine;
		}
		else if (szFlag == "loglevel") {
			_log.SetLogFlags(sLine);
		}
		else if (szFlag == "debuglevel") {
			_log.SetDebugFlags(sLine);
		}
		else if (szFlag == "notimestamps") {
			_log.EnableLogTimestamps(!GetConfigBool(sLine));
		}
#ifndef WIN32
		else if (szFlag == "syslog") {
			g_bUseSyslog = true;
			logfacname = sLine;
		}
#endif
		else if (szFlag == "dbase_file") {
			dbasefile = sLine;
		}
		else if (szFlag == "startup_delay") {
			int DelaySeconds = atoi(sLine.c_str());
			_log.Log(LOG_STATUS, "Startup delay... waiting %d seconds...", DelaySeconds);
			sleep_seconds(DelaySeconds);
		}
		else if (szFlag == "updates") {
			g_bUseUpdater = GetConfigBool(sLine);
		}
		else if (szFlag == "php_cgi_path") {
			webserver_settings.php_cgi_path = sLine;
#ifdef WWW_ENABLE_SSL
			secure_webserver_settings.php_cgi_path = sLine;
#endif
		}
		else if (szFlag == "app_path") {
			szStartupFolder = sLine;
			FixFolderEnding(szStartupFolder);
		}
		else if (szFlag == "userdata_path") {
			szUserDataFolder = sLine;
			FixFolderEnding(szUserDataFolder);
		}
		else if (szFlag == "daemon_name") {
			daemonname = sLine;
		}
		else if (szFlag == "daemon") {
			g_bRunAsDaemon = GetConfigBool(sLine);
		}
		else if (szFlag == "pidfile") {
			pidfile = sLine;
		}
		else if (szFlag == "launch_browser") {
			bStartWebBrowser = GetConfigBool(sLine);
		}
	}
	infile.close();

	return true;
}

void DisplayAppVersion()
{
	_log.Log(LOG_STATUS, "Domoticz V%s (c)2012-%d GizMoCuz", szAppVersion.c_str(), ActYear);
	_log.Log(LOG_STATUS, "Build Hash: %s, Date: %s", szAppHash.c_str(), szAppDate.c_str());
}

time_t m_LastHeartbeat = 0;

#if defined WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPSTR lpCmdLine,_In_ int nShowCmd)
#else
int main(int argc, char**argv)
#endif
{
#if defined WIN32
#ifndef _DEBUG
	CreateMutexA(0, FALSE, "Local\\Domoticz"); 
	if(GetLastError() == ERROR_ALREADY_EXISTS) { 
		MessageBox(HWND_DESKTOP,"Another instance of Domoticz is already running!","Domoticz",MB_OK);
		return 1; 
	}
#endif //_DEBUG
	RedirectIOToConsole();
#endif //WIN32

	CCmdLine cmdLine;

	// parse argc,argv 
#if defined WIN32
	cmdLine.SplitLine(__argc, __argv);
#else
	cmdLine.SplitLine(argc, argv);
	//ignore pipe errors
	signal(SIGPIPE, SIG_IGN);
#endif
	bool bUseConfigFile = cmdLine.HasSwitch("-f");
	if (bUseConfigFile) {
		if (cmdLine.GetArgumentCount("-f") != 1)
		{
			_log.Log(LOG_ERROR, "Please specify the configuration file.");
			return 1;
		}
		std::string szConfigFile = cmdLine.GetSafeArgument("-f", 0, "");
		if (!ParseConfigFile(szConfigFile))
			return 1;
	}
	else {
		if (cmdLine.HasSwitch("-loglevel"))
		{
			std::string szLevel = cmdLine.GetSafeArgument("-loglevel", 0, "");
			_log.SetLogFlags(szLevel);
		}
		if (cmdLine.HasSwitch("-debuglevel"))
		{
			std::string szLevel = cmdLine.GetSafeArgument("-debuglevel", 0, "");
			_log.SetDebugFlags(szLevel);
		}
		if (cmdLine.HasSwitch("-notimestamps"))
		{
			_log.EnableLogTimestamps(false);
		}
		if (cmdLine.HasSwitch("-log"))
		{
			if (cmdLine.GetArgumentCount("-log") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify an output log file");
				return 1;
			}
			logfile = cmdLine.GetSafeArgument("-log", 0, "domoticz.log");
		}
		if (cmdLine.HasSwitch("-approot"))
		{
			if (cmdLine.GetArgumentCount("-approot") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a APP root path");
				return 1;
			}
			std::string szroot = cmdLine.GetSafeArgument("-approot", 0, "");
			if (!szroot.empty())
			{
				szStartupFolder = szroot;
				FixFolderEnding(szStartupFolder);
			}
		}
	}

	if (!logfile.empty())
		_log.SetOutputFile(logfile.c_str());

	if (szStartupFolder.empty())
	{
#if !defined WIN32
		char szStartupPath[255];
		getExecutablePathName((char*)&szStartupPath,255);
		szStartupFolder=szStartupPath;
		if (szStartupFolder.find_last_of('/')!=std::string::npos)
			szStartupFolder=szStartupFolder.substr(0,szStartupFolder.find_last_of('/')+1);
#else
#ifndef _DEBUG
		char szStartupPath[255];
		char * p;
		GetModuleFileName(NULL, szStartupPath, sizeof(szStartupPath));
		p = szStartupPath + strlen(szStartupPath);

		while (p >= szStartupPath && *p != '\\')
			p--;

		if (++p >= szStartupPath)
			*p = 0;
		szStartupFolder=szStartupPath;
		size_t start_pos = szStartupFolder.find("\\Release\\");
		if(start_pos != std::string::npos) {
			szStartupFolder.replace(start_pos, 9, "\\domoticz\\");
			_log.Log(LOG_STATUS,"%s",szStartupFolder.c_str());
		}
#endif
#endif
	}

	/* call srand once for the entire app */
	std::srand((unsigned int)std::time(nullptr));
	szRandomUUID = GenerateUUID();    

	GetAppVersion();
	DisplayAppVersion();

	if (!szStartupFolder.empty())
		_log.Log(LOG_STATUS, "Startup Path: %s", szStartupFolder.c_str());

	if (szWWWFolder.empty())
		szWWWFolder = szStartupFolder + "www";
	if (szUserDataFolder.empty())
		szUserDataFolder = szStartupFolder;

	if (!bUseConfigFile) {
		if ((cmdLine.HasSwitch("-h")) || (cmdLine.HasSwitch("--help")) || (cmdLine.HasSwitch("/?")))
		{
			_log.Log(LOG_NORM, "%s", szHelp);
			return 0;
		}
		if ((cmdLine.HasSwitch("-version")) || (cmdLine.HasSwitch("--version")))
		{
			//Application version is already displayed
			//DisplayAppVersion();
			return 0;
		}
		if (cmdLine.HasSwitch("-userdata"))
		{
			if (cmdLine.GetArgumentCount("-userdata") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a path for user data to be stored");
				return 1;
			}
			std::string szroot = cmdLine.GetSafeArgument("-userdata", 0, "");
			if (!szroot.empty())
			{
				szUserDataFolder = szroot;
				FixFolderEnding(szUserDataFolder);
			}
		}
		if (cmdLine.HasSwitch("-startupdelay"))
		{
			if (cmdLine.GetArgumentCount("-startupdelay") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a startupdelay");
				return 1;
			}
			int DelaySeconds = atoi(cmdLine.GetSafeArgument("-startupdelay", 0, "").c_str());
			_log.Log(LOG_STATUS, "Startup delay... waiting %d seconds...", DelaySeconds);
			sleep_seconds(DelaySeconds);
		}
		if (cmdLine.HasSwitch("-wwwbind"))
		{
			if (cmdLine.GetArgumentCount("-wwwbind") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify an address");
				return 1;
			}
			webserver_settings.listening_address = cmdLine.GetSafeArgument("-wwwbind", 0, "0.0.0.0");
		}
		if (cmdLine.HasSwitch("-www"))
		{
			if (cmdLine.GetArgumentCount("-www") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a port");
				return 1;
			}
			std::string wwwport = cmdLine.GetSafeArgument("-www", 0, "");
			int iPort = (int)atoi(wwwport.c_str());
			if ((iPort < 0) || (iPort > 32767))
			{
				_log.Log(LOG_ERROR, "Please specify a valid www port");
				return 1;
			}
			webserver_settings.listening_port = wwwport;
		}
		if (cmdLine.HasSwitch("-php_cgi_path"))
		{
			if (cmdLine.GetArgumentCount("-php_cgi_path") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify the path to the php-cgi command");
				return 1;
			}
			webserver_settings.php_cgi_path = cmdLine.GetSafeArgument("-php_cgi_path", 0, "");
		}
		if (cmdLine.HasSwitch("-wwwroot"))
		{
			if (cmdLine.GetArgumentCount("-wwwroot") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a WWW root path");
				return 1;
			}
			std::string szroot = cmdLine.GetSafeArgument("-wwwroot", 0, "");
			if (!szroot.empty())
				szWWWFolder = szroot;
		}
	}
	webserver_settings.www_root = szWWWFolder;
	m_mainworker.SetWebserverSettings(webserver_settings);
#ifdef WWW_ENABLE_SSL
	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-sslwww"))
		{
			if (cmdLine.GetArgumentCount("-sslwww") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a port");
				return 1;
			}
			std::string wwwport = cmdLine.GetSafeArgument("-sslwww", 0, "");
			int iPort = (int)atoi(wwwport.c_str());
			if ((iPort < 0) || (iPort > 32767))
			{
				_log.Log(LOG_ERROR, "Please specify a valid sslwww port");
				return 1;
			}
			secure_webserver_settings.listening_port = wwwport;
		}
		if (!webserver_settings.listening_address.empty()) {
			// Secure listening address has to be equal
			secure_webserver_settings.listening_address = webserver_settings.listening_address;
		}
		if (cmdLine.HasSwitch("-sslcert"))
		{
			if (cmdLine.GetArgumentCount("-sslcert") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a file path for your server certificate file");
				return 1;
			}
			secure_webserver_settings.cert_file_path = cmdLine.GetSafeArgument("-sslcert", 0, "");
			secure_webserver_settings.private_key_file_path = secure_webserver_settings.cert_file_path;
		}
		if (cmdLine.HasSwitch("-sslkey"))
		{
			if (cmdLine.GetArgumentCount("-sslkey") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a file path for your server SSL key file");
				return 1;
			}
			secure_webserver_settings.private_key_file_path = cmdLine.GetSafeArgument("-sslkey", 0, "");
		}
		if (cmdLine.HasSwitch("-sslpass"))
		{
			if (cmdLine.GetArgumentCount("-sslpass") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a passphrase to access to your server private key in certificate file");
				return 1;
			}
			secure_webserver_settings.private_key_pass_phrase = cmdLine.GetSafeArgument("-sslpass", 0, "");
		}
		if (cmdLine.HasSwitch("-sslmethod"))
		{
			if (cmdLine.GetArgumentCount("-sslmethod") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a SSL method");
				return 1;
			}
			secure_webserver_settings.ssl_method = cmdLine.GetSafeArgument("-sslmethod", 0, "");
		}
		if (cmdLine.HasSwitch("-ssloptions"))
		{
			if (cmdLine.GetArgumentCount("-ssloptions") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify SSL options");
				return 1;
			}
			secure_webserver_settings.ssl_options = cmdLine.GetSafeArgument("-ssloptions", 0, "");
		}
		if (cmdLine.HasSwitch("-ssldhparam"))
		{
			if (cmdLine.GetArgumentCount("-ssldhparam") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a file path for the SSL DH parameters file");
				return 1;
			}
			secure_webserver_settings.tmp_dh_file_path = cmdLine.GetSafeArgument("-ssldhparam", 0, "");
		}
		if (cmdLine.HasSwitch("-php_cgi_path"))
		{
			if (cmdLine.GetArgumentCount("-php_cgi_path") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify the path to the php-cgi command");
				return 1;
			}
			secure_webserver_settings.php_cgi_path = cmdLine.GetSafeArgument("-php_cgi_path", 0, "");
		}
	}
	secure_webserver_settings.www_root = szWWWFolder;
	m_mainworker.SetSecureWebserverSettings(secure_webserver_settings);
#endif
	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-nowwwpwd"))
		{
			m_mainworker.m_bIgnoreUsernamePassword = true;
		}
		if (cmdLine.HasSwitch("-nocache"))
		{
			g_bDontCacheWWW = true;
		}
		if (cmdLine.HasSwitch("-wwwcompress"))
		{
			if (cmdLine.GetArgumentCount("-wwwcompress") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a compress mode");
				return 1;
			}
			std::string szmode = cmdLine.GetSafeArgument("-wwwcompress", 0, "on");
			if (szmode == "off")
				g_wwwCompressMode = http::server::WWW_FORCE_NO_GZIP_SUPPORT;
			else if (szmode == "static")
				g_wwwCompressMode = http::server::WWW_USE_STATIC_GZ_FILES;
		}
	}
	if (dbasefile.empty()) {
		dbasefile = szUserDataFolder + "domoticz.db";
#ifdef WIN32
#ifndef _DEBUG
		if (!IsUserAnAdmin())
		{
			char szPath[MAX_PATH];
			HRESULT hr = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath);
			if (SUCCEEDED(hr))
			{
				std::string sPath = szPath;
				sPath += "\\Domoticz";

				DWORD dwAttr = GetFileAttributes(sPath.c_str());
				BOOL bDirExists = (dwAttr != 0xffffffff && (dwAttr & FILE_ATTRIBUTE_DIRECTORY));
				if (!bDirExists)
				{
					BOOL bRet = CreateDirectory(sPath.c_str(), NULL);
					if (bRet == FALSE) {
						MessageBox(0, "Error creating Domoticz directory in program data folder (%ProgramData%)!!", "Error:", MB_OK);
					}
				}
				sPath += "\\domoticz.db";
				dbasefile = sPath;
			}
		}
#endif
#endif
	}
	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-dbase"))
		{
			if (cmdLine.GetArgumentCount("-dbase") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a Database Name");
				return 1;
			}
			dbasefile = cmdLine.GetSafeArgument("-dbase", 0, "domoticz.db");
		}
	}
	m_sql.SetDatabaseName(dbasefile);

	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-webroot"))
		{
			if (cmdLine.GetArgumentCount("-webroot") != 1)
			{
				_log.Log(LOG_ERROR, "Please specify a web root path");
				return 1;
			}
			std::string szroot = cmdLine.GetSafeArgument("-webroot", 0, "");
			if (!szroot.empty())
				szWebRoot = szroot;
		}
		if (cmdLine.HasSwitch("-noupdates"))
		{
			g_bUseUpdater = false;
		}
	}

#if defined WIN32
	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-nobrowser"))
		{
			bStartWebBrowser = false;
		}
	}
	//Init WinSock
	WSADATA data;
	WORD version;

	version = (MAKEWORD(2, 2));
	int ret = WSAStartup(version, &data);
	if (ret != 0)
	{
		ret = WSAGetLastError();

		if (ret == WSANOTINITIALISED)
		{
			_log.Log(LOG_ERROR, "Error: Winsock could not be initialized!");
		}
	}
	CoInitializeEx(0, COINIT_MULTITHREADED);
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
#endif

#ifndef WIN32
	if (!bUseConfigFile) {
		if (cmdLine.HasSwitch("-daemon"))
		{
			g_bRunAsDaemon = true;
		}
		if (cmdLine.HasSwitch("-daemonname"))
		{
			daemonname = cmdLine.GetSafeArgument("-daemonname", 0, DAEMON_NAME);
		}

		if (cmdLine.HasSwitch("-pidfile"))
		{
			pidfile = cmdLine.GetSafeArgument("-pidfile", 0, PID_FILE);
		}

		if (cmdLine.HasSwitch("-syslog"))
		{
			g_bUseSyslog = true;
			logfacname = cmdLine.GetSafeArgument("-syslog", 0, "");
			if (logfacname.length() == 0)
			{
				logfacname = "user";
			}
		}
	}

	if ((g_bRunAsDaemon)||(g_bUseSyslog))
	{
		int logfacility = 0;

		for (const auto &facility : facilities)
		{
			if (strcmp(facility.first, logfacname.c_str()) == 0)
			{
				logfacility = facility.second;
				break;
			}
		}
		if ( logfacility == 0 ) 
		{
			_log.Log(LOG_ERROR, "%s is an unknown syslog facility", logfacname.c_str());
			return 1;
		}

		// _log.Log(LOG_STATUS, "syslog to %s (%x)", logfacname.c_str(), logfacility);
		setlogmask(LOG_UPTO(LOG_INFO));
		openlog(daemonname.c_str(), LOG_CONS | LOG_PERROR, logfacility);

		syslog(LOG_INFO, "Domoticz is starting up....");
	}

	if (g_bRunAsDaemon)
	{
		/* Deamonize */
		daemonize(szStartupFolder.c_str(), pidfile.c_str());
	}
	if ((g_bRunAsDaemon) || (g_bUseSyslog))
	{
		syslog(LOG_INFO, "Domoticz running...");
	}
#endif

	if (!g_bRunAsDaemon)
	{
#ifndef WIN32
		struct sigaction newSigAction;

		/* Set up a signal handler */
		newSigAction.sa_sigaction = signal_handler;
		sigemptyset(&newSigAction.sa_mask);
		newSigAction.sa_flags = SA_SIGINFO;

		/* Signals to handle */
		sigaction(SIGTERM, &newSigAction, nullptr); // catch term signal
		sigaction(SIGINT, &newSigAction, nullptr);  // catch interrupt signal
		sigaction(SIGSEGV, &newSigAction, nullptr); // catch segmentation fault signal
		sigaction(SIGABRT, &newSigAction, nullptr); // catch abnormal termination signal
		sigaction(SIGILL, &newSigAction, nullptr);  // catch invalid program image
		sigaction(SIGFPE, &newSigAction, nullptr);  // catch floating point error
		sigaction(SIGUSR1, &newSigAction, nullptr); // catch SIGUSR1 (used by watchdog)
#else
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
#endif
	}

	// start Watchdog thread after daemonization
	m_LastHeartbeat = mytime(nullptr);
	std::thread thread_watchdog(Do_Watchdog_Work);
	SetThreadName(thread_watchdog.native_handle(), "Watchdog");

	if (!m_mainworker.Start())
	{
		return 1;
	}
	m_StartTime = time(nullptr);

	/* now, lets get into an infinite loop of doing nothing. */
#if defined WIN32
#ifndef _DEBUG
	RedirectIOToConsole();	//hide console
#endif
	InitWindowsHelper(hInstance, hPrevInstance, nShowCmd, m_mainworker.GetWebserverAddress(), atoi(m_mainworker.GetWebserverPort().c_str()), bStartWebBrowser);
	MSG Msg;
	while (!g_bStopApplication)
	{
		if (PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&Msg, NULL, 0, 0) > 0)
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
		else
			sleep_milliseconds(100);
		m_LastHeartbeat = mytime(NULL);
	}
	TrayMessage(NIM_DELETE, NULL);
#else
	while ( !g_bStopApplication )
	{
		sleep_seconds(1);
		m_LastHeartbeat = mytime(nullptr);
	}
#endif
	_log.Log(LOG_STATUS, "Closing application!...");
	fflush(stdout);
	_log.Log(LOG_STATUS, "Stopping worker...");
	try
	{
		m_mainworker.Stop();
	}
	catch (...)
	{

	}
#ifndef WIN32
	if (g_bRunAsDaemon)
	{
		syslog(LOG_INFO, "Domoticz stopped...");
		daemonShutdown();

		// Delete PID file
		remove(pidfile.c_str());
	}
#else
	// Release WinSock
	WSACleanup();
	CoUninitialize();
#endif
	g_stop_watchdog = true;
	thread_watchdog.join();
	return 0;
}

