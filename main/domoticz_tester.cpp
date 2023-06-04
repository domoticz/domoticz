#include "stdafx.h"
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include "CmdLine.h"
#include "Helper.h"
#include "appversion.h"
#include "localtime_r.h"

#ifndef WIN32
	#include <sys/stat.h>
	#include <unistd.h>
	#include <syslog.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <string.h>
	#include <stdarg.h>
#endif

constexpr const char *szHelp
{
	"Usage: Domoticz_tester\n"
	"\t-module <module> (which module to test, for example -module helper)\n"
	"\t-function <function> (for example -function stdstring_ltrim)\n"
	"\t-input <input> (for example -input \"3,text\")\n"
	"\t-verbose More verbose output\n"
	"\t-binary Output the result in binary (Hexadecimal)\n"
	"\t-measure Show measurements like execution speed\n"
	"\t-version display version number\n"
	"\t-h (or --help or /?) display this help information\n"
	"\n"
	"Available modules:\n"
	"\thelper\n"
	"\tbaroforecastcalculator\n"
	""
};

std::string szAppVersion="???";
int iAppRevision=0;
std::string szAppHash="???";
std::string szAppDate="???";
std::string szPyVersion="None";
int ActYear;
time_t m_StartTime = time(nullptr);

std::string szTestModule = "";
std::string szTestFunction = "";
std::string szTestInput = "";
std::string szTestOutput = "";

bool bSuccess = false;
bool bVerbose = false;
bool bQuiet = false;
bool bBinary = false;
bool bMeasure = false;

#define MAX_LOG_LINE_BUFFER 100
#define MAX_LOG_LINE_LENGTH (2048 * 3)

#define INPUTSEPERATOR "|#|"

/* **********
Some supporting functions
********** */

void Log(const std::string &sLogline, ...)
{
	Log("%s", sLogline.c_str());
}

void Log(const char *logline, ...)
{
	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	std::stringstream sstr;
	if (bVerbose)
	{
		sstr << TimeToString(nullptr, TF_DateTimeMs) << "  ";
		sstr << "Log: ";
	}
	sstr << cbuffer;
	std::cout << sstr.str() << std::endl;
}

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

void DisplayAppVersion()
{
	if(!bQuiet)
	{
		Log("Domoticz_tester V%s (c)2022-%d KidDigital", szAppVersion.c_str(), ActYear);
		Log("Build Hash: %s, Date: %s", szAppHash.c_str(), szAppDate.c_str());
		Log("");
	}
}

/* **********
Helper.cpp
********** */
bool helper_tester(const std::string szFunction, std::string &szInput, std::string &szOutput)
{
	bool bSuccess = false;

	std::vector<std::string> svInputs;
	StringSplit(szInput, INPUTSEPERATOR, svInputs);

	// stdstring_ltrim
	if (szFunction == "stdstring_ltrim")
	{
		szOutput = stdstring_ltrim(szInput);
		bSuccess = true;
	}
	// stdstring_rtrim
	else if (szFunction == "stdstring_rtrim")
	{
		szOutput = stdstring_rtrim(szInput);
		bSuccess = true;
	}
	// stdstring_trim
	else if (szFunction == "stdstring_trim")
	{
		szOutput = stdstring_trim(szInput);
		bSuccess = true;
	}
	// stdstring_ltrimws
	else if (szFunction == "stdstring_ltrimws")
	{
		szOutput = stdstring_ltrimws(szInput);
		bSuccess = true;
	}
	// stdstring_rtrimws
	else if (szFunction == "stdstring_rtrimws")
	{
		szOutput = stdstring_rtrimws(szInput);
		bSuccess = true;
	}
	// stdstring_trimws
	else if (szFunction == "stdstring_trimws")
	{
		szOutput = stdstring_trimws(szInput);
		bSuccess = true;
	}
	// GenerateMD5Hash
	else if (szFunction == "GenerateMD5Hash")
	{
		szOutput = GenerateMD5Hash(szInput);
		bSuccess = true;
	}
	// Crc32
	else if (szFunction == "Crc32")
	{
		unsigned int uiTestOutput = Crc32(0, (const unsigned char*)szInput.c_str(), szInput.length());
		szOutput = std_format("%d", uiTestOutput);
		bSuccess = true;
	}
	// round_digits
	else if (szFunction == "round_digits")
	{
		if (svInputs.size() == 2)
		{
			double dTestOutput = round_digits(std::stod(svInputs[0]), std::stoi(svInputs[1]));
			szOutput = std_format("%f", dTestOutput);
			bSuccess = true;
		}
	}
	// CalculateDewPoint
	else if (szFunction == "CalculateDewPoint")
	{
		if (svInputs.size() == 2)
		{
			double dTestOutput = CalculateDewPoint(std::stod(svInputs[0]), std::stoi(svInputs[1]));
			szOutput = std_format("%.2f", dTestOutput);
			bSuccess = true;
		}
	}
	// base32_decode
	else if (szFunction == "base32_decode")
	{
		bSuccess = base32_decode(szInput, szOutput);
	}
	// base32_encode
	else if (szFunction == "base32_encode")
	{
		bSuccess = base32_encode(szInput, szOutput);
	}
	else
	{
		szOutput = "NOT FOUND!";
	}
	return bSuccess;
}

/* **********
Main function
********** */
int main(int argc, char**argv)
{
	CCmdLine cmdLine;

	// parse argc,argv 
	cmdLine.SplitLine(argc, argv);
	//ignore pipe errors
	signal(SIGPIPE, SIG_IGN);

	if (cmdLine.HasSwitch("-verbose"))
	{
		bVerbose = true;
	}
	if (cmdLine.HasSwitch("-quiet"))
	{
		bQuiet = true;
	}
	if (cmdLine.HasSwitch("-binary"))
	{
		bBinary = true;
	}
	if (cmdLine.HasSwitch("-measure"))
	{
		bMeasure = true;
	}

	GetAppVersion();
	DisplayAppVersion();

	if ((cmdLine.HasSwitch("-h")) || (cmdLine.HasSwitch("--help")) || (cmdLine.HasSwitch("/?")))
	{
		Log("%s", szHelp);
		return 0;
	}
	if ((cmdLine.HasSwitch("-version")) || (cmdLine.HasSwitch("--version")))
	{
		return 0;
	}

	if (cmdLine.HasSwitch("-module"))
	{
		if (cmdLine.GetArgumentCount("-module") != 1)
		{
			Log("Please specify a module to test!");
			return 1;
		}
		std::string szroot = cmdLine.GetSafeArgument("-module", 0, "");
		if (!szroot.empty())
			szTestModule = szroot;
	}

	if (cmdLine.HasSwitch("-function"))
	{
		if (cmdLine.GetArgumentCount("-function") != 1)
		{
			Log("Please specify a function to test within module %s", szTestModule.c_str());
			return 1;
		}
		std::string szroot = cmdLine.GetSafeArgument("-function", 0, "");
		if (!szroot.empty())
			szTestFunction = szroot;
	}

	if (cmdLine.HasSwitch("-input"))
	{
		int iArgCnt = cmdLine.GetArgumentCount("-input");
		if (iArgCnt < 1)
		{
			Log("Please specify the input parameters for function %s", szTestFunction.c_str());
			return 1;
		}
		for(int i=0;i < iArgCnt; i++)
		{
			std::string szroot = cmdLine.GetSafeArgument("-input", i, "");
			if (!szroot.empty())
			{
				if (!szTestInput.empty())
					szTestInput = szTestInput + INPUTSEPERATOR;
				szTestInput = szTestInput + szroot;
			}
		}
	}

	if (szTestModule.empty() || szTestFunction.empty() || szTestInput.empty())
	{
		Log("Please specify a (valid) module and function and input to run this tester!");
		return 1;
	}

	/* call srand once for the entire app */
	std::srand((unsigned int)std::time(nullptr));

	if (!bQuiet)
		Log("Domoticztester testing function %s of Module %s with input: .%s.", szTestFunction.c_str(), szTestModule.c_str(), szTestInput.c_str());

	if (szTestModule == "helper")
	{
		try
		{
			bSuccess = helper_tester(szTestFunction, szTestInput, szTestOutput);
		}
		catch(const std::exception& e)
		{
			Log("Executing : %s (%s) | Crashed! (%s)", szTestFunction.c_str(), szTestModule.c_str(), e.what());
			return 1;
		}
	}
	else if (false)
	{
		/* code */
	}
	else
	{
		Log("No module %s found!", szTestModule.c_str());
		return 1;
	}

	std::stringstream sstr;
	sstr << "Executing : " << szTestFunction << " (" << szTestModule << ") | ";
	if (bSuccess)
		sstr << "Result : ." << szTestOutput << ".";
	else
	{
		sstr << "Failed!";
		if (!szTestOutput.empty())
			sstr << " (" << szTestOutput << ")";
	}
	if (bBinary)
	{
		std::string sHex = ToHexString((uint8_t*)szTestOutput.c_str(),szTestOutput.size());
		sstr << " (b'" << sHex << "')";
	}

	Log(sstr.str().c_str());

	fflush(stdout);
	return !bSuccess;
}
