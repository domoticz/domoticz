#include "stdafx.h"
#include "Helper.h"
#include "Logger.h"
#ifdef WIN32
#include "dirent_windows.h"
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
#if !defined(WIN32)
#include <sys/ptrace.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <math.h>
#include <algorithm>
#include "../main/localtime_r.h"
#include <sstream>
#include <openssl/md5.h>
#include <chrono>
#include <limits.h>
#include <cstring>

#if defined WIN32
#include "../msbuild/WindowsHelper.h"
#endif

#include "RFXtrx.h"
#include "../hardware/hardwaretypes.h"

// Includes for SystemUptime()
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <sys/sysinfo.h>
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <time.h>
#endif

#if defined(__FreeBSD__) 
// Check if OpenBSD or DragonFly need that at well?
#include <pthread_np.h>
#ifndef PTHREAD_MAX_MAMELEN_NP
#define PTHREAD_MAX_NAMELEN_NP 32 	// Arbitrary
#endif
#endif

void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results)
{
	results.clear();
	size_t cutAt;
	while( (cutAt = str.find(delim)) != std::string::npos )
	{
		results.push_back(str.substr(0,cutAt));
		str = str.substr(cutAt+ delim.size());
	}
	if (!str.empty())
	{
		results.push_back(str);
	}
}

uint64_t hexstrtoui64(const std::string &str)
{
	uint64_t ul;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> ul;
	return ul;
}

void stdreplace(
	std::string &inoutstring,
	const std::string& replaceWhat,
	const std::string& replaceWithWhat)
{
	int pos = 0;
	while (std::string::npos != (pos = inoutstring.find(replaceWhat, pos)))
	{
		inoutstring.replace(pos, replaceWhat.size(), replaceWithWhat);
		pos += replaceWithWhat.size();
	}
}

void stdupper(std::string &inoutstring)
{
	for (size_t i = 0; i < inoutstring.size(); ++i)
		inoutstring[i] = toupper(inoutstring[i]);
}

void stdlower(std::string &inoutstring)
{
	std::transform(inoutstring.begin(), inoutstring.end(), inoutstring.begin(), ::tolower);
}

void stdupper(std::wstring& inoutstring)
{
	for (size_t i = 0; i < inoutstring.size(); ++i)
		inoutstring[i] = towupper(inoutstring[i]);
}

void stdlower(std::wstring& inoutstring)
{
	std::transform(inoutstring.begin(), inoutstring.end(), inoutstring.begin(), ::towlower);
}

std::vector<std::string> GetSerialPorts(bool &bUseDirectPath)
{
	bUseDirectPath=false;

	std::vector<std::string> ret;
#if defined WIN32
	//windows

	std::vector<int> ports;
	std::vector<std::string> friendlyNames;
	char szPortName[40];

	EnumSerialFromWMI(ports, friendlyNames);

	bool bFoundPort = false;
	if (!ports.empty())
	{
		bFoundPort = true;
		for (const auto & itt : ports)
		{
			sprintf(szPortName, "COM%d", itt);
			ret.push_back(szPortName);
		}
	}

	if (bFoundPort)
		return ret;

	//Scan old fashion way (SLOW!)
	COMMCONFIG cc;
	DWORD dwSize = sizeof(COMMCONFIG);
	for (int ii = 0; ii < 256; ii++)
	{
		sprintf(szPortName, "COM%d", ii);
		if (GetDefaultCommConfig(szPortName, &cc, &dwSize))
		{
			bFoundPort = true;
			sprintf(szPortName, "COM%d", ii);

			//Check if we did not already have it
			bool bFound = false;
			for (const auto & itt : ret)
			{
				if (itt == szPortName)
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
				ret.push_back(szPortName); // add port
		}
	}
	// Method 2: CreateFile, slow
	// ---------
	if (!bFoundPort) {
		for (int ii = 0; ii < 256; ii++)
		{
			sprintf(szPortName, "\\\\.\\COM%d", ii);
			bool bSuccess = false;
			HANDLE hPort = ::CreateFile(szPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
			if (hPort == INVALID_HANDLE_VALUE) {
				DWORD dwError = GetLastError();
				//Check to see if the error was because some other app had the port open
				if (dwError == ERROR_ACCESS_DENIED)
					bSuccess = TRUE;
			}
			else {
				//The port was opened successfully
				bSuccess = TRUE;
				//Don't forget to close the port, since we are going to do nothing with it anyway
				CloseHandle(hPort);
			}
			if (bSuccess) {
				bFoundPort = true;
				sprintf(szPortName, "COM%d", ii);
				ret.push_back(szPortName); // add port
			}
			// --------------
		}
	}
	// Method 3: EnumSerialPortsWindows, often fails
	// ---------
	if (!bFoundPort) {
		std::vector<SerialPortInfo> serialports;
		EnumSerialPortsWindows(serialports);
		if (!serialports.empty())
		{
			for (const auto & itt : serialports)
			{
				ret.push_back(itt.szPortName); // add port
			}
		}
	}

#else
	//scan /dev for /dev/ttyUSB* or /dev/ttyS* or /dev/tty.usbserial* or /dev/ttyAMA* or /dev/ttySAC* or /dev/ttymxc*
	//also scan /dev/serial/by-id/* on Linux

	bool bHaveTtyAMAfree=false;
	std::string sLine = "";
	std::ifstream infile;

	infile.open("/boot/cmdline.txt");
	if (infile.is_open())
	{
		if (!infile.eof())
		{
			getline(infile, sLine);
			bHaveTtyAMAfree=(sLine.find("ttyAMA0")==std::string::npos);
		}
	}

	DIR *d=NULL;
	d=opendir("/dev");
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while ((de = readdir(d)))
		{
			// Only consider character devices and symbolic links
                        if ((de->d_type == DT_CHR) || (de->d_type == DT_LNK))
                        {
			std::string fname = de->d_name;
			if (fname.find("ttyUSB")!=std::string::npos)
			{
				ret.push_back("/dev/" + fname);
			}
			else if (fname.find("tty.usbserial")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
			else if (fname.find("ttyACM")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
			else if (fname.find("ttySAC") != std::string::npos)
			{
				bUseDirectPath = true;
				ret.push_back("/dev/" + fname);
			}
			else if (fname.find("ttymxc") != std::string::npos)
			{
				bUseDirectPath = true;
				ret.push_back("/dev/" + fname);
			}
#if defined (__FreeBSD__) || defined (__OpenBSD__) || defined (__NetBSD__)
			else if (fname.find("ttyU")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
			else if (fname.find("cuaU")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
#endif
#ifdef __APPLE__
			else if (fname.find("cu.")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
#endif
			if (bHaveTtyAMAfree)
			{
				if (fname.find("ttyAMA0")!=std::string::npos)
				{
					ret.push_back("/dev/" + fname);
					bUseDirectPath=true;
				}
				// By default, this is the "small UART" on Rasberry 3 boards
                                        if (fname.find("ttyS0")!=std::string::npos)
                                        {
                                                ret.push_back("/dev/" + fname);
                                                bUseDirectPath=true;
                                        }
                                        // serial0 and serial1 are new with Rasbian Jessie
                                        // Avoids confusion between Raspberry 2 and 3 boards
                                        // More info at http://spellfoundry.com/2016/05/29/configuring-gpio-serial-port-raspbian-jessie-including-pi-3/
                                        if (fname.find("serial")!=std::string::npos)
                                        {
                                                ret.push_back("/dev/" + fname);
                                                bUseDirectPath=true;
                                        }
				}
			}
		}
		closedir(d);
	}
	//also scan in /dev/usb
	d=opendir("/dev/usb");
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while ((de = readdir(d)))
		{
			std::string fname = de->d_name;
			if (fname.find("ttyUSB")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/usb/" + fname);
			}
		}
		closedir(d);
	}

#if defined(__linux__) || defined(__linux) || defined(linux)
	d=opendir("/dev/serial/by-id");
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while ((de = readdir(d)))
		{
			// Only consider symbolic links
                        if (de->d_type == DT_LNK)
                        {
				std::string fname = de->d_name;
				ret.push_back("/dev/serial/by-id/" + fname);
			}
		}
		closedir(d);
	}

#endif
#endif
	return ret;
}

bool file_exist (const char *filename)
{
	struct stat sbuffer;
	return (stat(filename, &sbuffer) == 0);
}

double CalculateAltitudeFromPressure(double pressure)
{
	double seaLevelPressure=101325.0;
	double altitude = 44330.0 * (1.0 - pow( (pressure / seaLevelPressure), 0.1903));
	return altitude;
}

/**************************************************************************/
/*!
Calculates the altitude (in meters) from the specified atmospheric
pressure (in hPa), sea-level pressure (in hPa), and temperature (in °C)
@param seaLevel Sea-level pressure in hPa
@param atmospheric Atmospheric pressure in hPa
@param temp Temperature in degrees Celsius
*/
/**************************************************************************/
float pressureToAltitude(float seaLevel, float atmospheric, float temp)
{
	/* Hyposometric formula: */
	/* */
	/* ((P0/P)^(1/5.257) - 1) * (T + 273.15) */
	/* h = ------------------------------------- */
	/* 0.0065 */
	/* */
	/* where: h = height (in meters) */
	/* P0 = sea-level pressure (in hPa) */
	/* P = atmospheric pressure (in hPa) */
	/* T = temperature (in °C) */
	return (((float)pow((seaLevel / atmospheric), 0.190223F) - 1.0F)
		* (temp + 273.15F)) / 0.0065F;
}

/**************************************************************************/
/*!
Calculates the sea-level pressure (in hPa) based on the current
altitude (in meters), atmospheric pressure (in hPa), and temperature
(in °C)
@param altitude altitude in meters
@param atmospheric Atmospheric pressure in hPa
@param temp Temperature in degrees Celsius
*/
/**************************************************************************/
float pressureSeaLevelFromAltitude(float altitude, float atmospheric, float temp)
{
	/* Sea-level pressure: */
	/* */
	/* 0.0065*h */
	/* P0 = P * (1 - ----------------- ) ^ -5.257 */
	/* T+0.0065*h+273.15 */
	/* */
	/* where: P0 = sea-level pressure (in hPa) */
	/* P = atmospheric pressure (in hPa) */
	/* h = altitude (in meters) */
	/* T = Temperature (in °C) */
	return atmospheric * (float)pow((1.0F - (0.0065F * altitude) /
		(temp + 0.0065F * altitude + 273.15F)), -5.257F);
}


std::string &stdstring_ltrim(std::string &s)
{
	while (!s.empty())
	{
		if (s[0] != ' ')
			return s;
		s = s.substr(1);
	}
	//	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string &stdstring_rtrim(std::string &s)
{
	while (!s.empty())
	{
		if (s[s.size() - 1] != ' ')
			return s;
		s = s.substr(0, s.size() - 1);
	}
	//s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &stdstring_trim(std::string &s)
{
	return stdstring_ltrim(stdstring_rtrim(s));
}

double CalculateDewPoint(double temp, int humidity)
{
	if (humidity==0)
		return temp;
	double dew_numer = 243.04*(log(double(humidity)/100.0)+((17.625*temp)/(temp+243.04)));
	double dew_denom = 17.625-log(double(humidity)/100.0)-((17.625*temp)/(temp+243.04));
	if (dew_numer==0)
		dew_numer=1;
	return dew_numer/dew_denom;
}

uint32_t IPToUInt(const std::string &ip)
{
	int a, b, c, d;
	uint32_t addr = 0;

	if (sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
		return 0;

	addr = a << 24;
	addr |= b << 16;
	addr |= c << 8;
	addr |= d;
	return addr;
}

bool isInt(const std::string &s)
{
	for(size_t i = 0; i < s.length(); i++){
		if(!isdigit(s[i]))
			return false;
	}
	return true;
}

void sleep_seconds(const long seconds)
{
	std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void sleep_milliseconds(const long milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int createdir(const char *szDirName, int secattr)
{
	int ret = 0;
#ifdef WIN32
	ret = _mkdir(szDirName);
#else
	ret = mkdir(szDirName, secattr);
#endif
	return ret;
}

int mkdir_deep(const char *szDirName, int secattr)
{
	char DirName[260];
	DirName[0] = 0;
	const char* p = szDirName;
	char* q = DirName;
	int ret = 0;
	while(*p)
	{
		if (('\\' == *p) || ('/' == *p))
		{
			if ((p > szDirName) && (':' != *(p-1)))
			{
				ret = createdir(DirName, secattr);
			}
		}
		*q++ = *p++;
		*q = '\0';
	}
	if (DirName[0])
	{
		ret = createdir(DirName, secattr);
	}
	return ret;
}

int RemoveDir(const std::string &dirnames, std::string &errorPath)
{
	std::vector<std::string> splitresults;
	StringSplit(dirnames, "|", splitresults);
	int returncode = 0;
	if (!splitresults.empty())
	{
#ifdef WIN32
		for (size_t i = 0; i < splitresults.size(); i++)
		{
			if (!file_exist(splitresults[i].c_str()))
				continue;
			size_t s_szLen = strlen(splitresults[i].c_str());
			if (s_szLen < MAX_PATH)
			{
				char deletePath[MAX_PATH + 1];
				strcpy_s(deletePath, splitresults[i].c_str());
				deletePath[s_szLen + 1] = '\0'; // SHFILEOPSTRUCT needs an additional null char

				SHFILEOPSTRUCT shfo = { NULL, FO_DELETE, deletePath, NULL, FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION, FALSE, NULL, NULL };
				if (returncode = SHFileOperation(&shfo))
				{
					errorPath = splitresults[i];
					break;
				}
			}
		}
#else
		for (size_t i = 0; i < splitresults.size(); i++)
		{
			if (!file_exist(splitresults[i].c_str()))
				continue;
			ExecuteCommandAndReturn("rm -rf \"" + splitresults[i] + "\"", returncode);
			if (returncode)
			{
				errorPath = splitresults[i];
				break;
			}
		}
#endif
	}
	return returncode;
}

double ConvertToCelsius(const double Fahrenheit)
{
	return (Fahrenheit-32.0) * 0.5556;
}

double ConvertToFahrenheit(const double Celsius)
{
	return (Celsius*1.8)+32.0;
}

double RoundDouble(const long double invalue, const short numberOfPrecisions)
{
	long long p = (long long) pow(10.0L, numberOfPrecisions);
	double ret= (long long)(invalue * p + 0.5L) / (double)p;
	return ret;
}

double ConvertTemperature(const double tValue, const unsigned char tSign)
{
	if (tSign=='C')
		return tValue;
	return RoundDouble(ConvertToFahrenheit(tValue),1);
}

std::vector<std::string> ExecuteCommandAndReturn(const std::string &szCommand, int &returncode)
{
	std::vector<std::string> ret;

	try
	{
		FILE *fp;

		/* Open the command for reading. */
#ifdef WIN32
		fp = _popen(szCommand.c_str(), "r");
#else
		fp = popen(szCommand.c_str(), "r");
#endif
		if (fp != NULL)
		{
			char path[1035];
			/* Read the output a line at a time - output it. */
			while (fgets(path, sizeof(path) - 1, fp) != NULL)
			{
				ret.push_back(path);
			}
			/* close */
#ifdef WIN32
			returncode = _pclose(fp);
#else
			returncode = pclose(fp);
#endif
		}
	}
	catch (...)
	{

	}
	return ret;
}

std::string TimeToString(const time_t *ltime, const _eTimeFormat format)
{
	struct tm timeinfo;
	struct timeval tv;
	std::stringstream sstr;
	if (ltime == NULL) // current time
	{
#ifdef CLOCK_REALTIME
		struct timespec ts;
		if (!clock_gettime(CLOCK_REALTIME, &ts))
		{
			tv.tv_sec = ts.tv_sec;
			tv.tv_usec = ts.tv_nsec / 1000;
		}
		else
#endif
			gettimeofday(&tv, NULL);
#ifdef WIN32
		time_t tv_sec = tv.tv_sec;
		localtime_r(&tv_sec, &timeinfo);
#else
		localtime_r(&tv.tv_sec, &timeinfo);
#endif
	}
	else
		localtime_r(ltime, &timeinfo);

	if (format > TF_Time)
	{
		//Date
		sstr << (timeinfo.tm_year + 1900) << "-"
		<< std::setw(2)	<< std::setfill('0') << (timeinfo.tm_mon + 1) << "-"
		<< std::setw(2) << std::setfill('0') << timeinfo.tm_mday;
	}

	if (format != TF_Date)
	{
		//Time
		if (format > TF_Time)
			sstr << " ";
		sstr
		<< std::setw(2) << std::setfill('0') << timeinfo.tm_hour << ":"
		<< std::setw(2) << std::setfill('0') << timeinfo.tm_min << ":"
		<< std::setw(2) << std::setfill('0') << timeinfo.tm_sec;
	}

	if (format > TF_DateTime && ltime == NULL)
		sstr << "." << std::setw(3) << std::setfill('0') << ((int)tv.tv_usec / 1000);

	return sstr.str();
}

std::string GenerateMD5Hash(const std::string &InputString, const std::string &Salt)
{
	std::string cstring = InputString + Salt;
	unsigned char digest[MD5_DIGEST_LENGTH + 1];
	digest[MD5_DIGEST_LENGTH] = 0;
	MD5((const unsigned char*)cstring.c_str(), cstring.size(), (unsigned char*)&digest);
	char mdString[(MD5_DIGEST_LENGTH * 2) + 1];
	mdString[MD5_DIGEST_LENGTH * 2] = 0;
	for (int i = 0; i < 16; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	return mdString;
}

void hsb2rgb(const float hue, const float saturation, const float vlue, int &outR, int &outG, int &outB, const double maxValue/* = 100.0 */)
{
	double      hh, p, q, t, ff;
	long        i;

	if(saturation <= 0.0) {
		outR = int(vlue*maxValue);
		outG = int(vlue*maxValue);
		outB = int(vlue*maxValue);
	}
	hh = hue;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = vlue * (1.0 - saturation);
	q = vlue * (1.0 - (saturation * ff));
	t = vlue * (1.0 - (saturation * (1.0 - ff)));

	switch (i) {
	case 0:
		outR = int(vlue*maxValue);
		outG = int(t*maxValue);
		outB = int(p*maxValue);
		break;
	case 1:
		outR = int(q*maxValue);
		outG = int(vlue*maxValue);
		outB = int(p*maxValue);
		break;
	case 2:
		outR = int(p*maxValue);
		outG = int(vlue*maxValue);
		outB = int(t*maxValue);
		break;

	case 3:
		outR = int(p*maxValue);
		outG = int(q*maxValue);
		outB = int(vlue*maxValue);
		break;
	case 4:
		outR = int(t*maxValue);
		outG = int(p*maxValue);
		outB = int(vlue*maxValue);
		break;
	case 5:
	default:
		outR = int(vlue*maxValue);
		outG = int(p*maxValue);
		outB = int(q*maxValue);
		break;
	}
}

void rgb2hsb(const int r, const int g, const int b, float hsbvals[3])
{
	float hue, saturation, brightness;
	if (hsbvals == NULL)
		return;
	int cmax = (r > g) ? r : g;
	if (b > cmax) cmax = b;
	int cmin = (r < g) ? r : g;
	if (b < cmin) cmin = b;

	brightness = ((float)cmax) / 255.0f;
	if (cmax != 0)
		saturation = ((float)(cmax - cmin)) / ((float)cmax);
	else
		saturation = 0;
	if (saturation == 0)
		hue = 0;
	else {
		float redc = ((float)(cmax - r)) / ((float)(cmax - cmin));
		float greenc = ((float)(cmax - g)) / ((float)(cmax - cmin));
		float bluec = ((float)(cmax - b)) / ((float)(cmax - cmin));
		if (r == cmax)
			hue = bluec - greenc;
		else if (g == cmax)
			hue = 2.0f + redc - bluec;
		else
			hue = 4.0f + greenc - redc;
		hue = hue / 6.0f;
		if (hue < 0)
			hue = hue + 1.0f;
	}
	hsbvals[0] = hue;
	hsbvals[1] = saturation;
	hsbvals[2] = brightness;
}

bool is_number(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && (isdigit(*it) || (*it == '.') || (*it == '-') || (*it == ' ') || (*it == 0x00))) ++it;
	return !s.empty() && it == s.end();
}

void padLeft(std::string &str, const size_t num, const char paddingChar)
{
	if (num > str.size())
		str.insert(0, num - str.size(), paddingChar);
}

bool IsLightOrSwitch(const int devType, const int subType)
{
	bool bIsLightSwitch = false;
	switch (devType)
	{
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeFan:
	case pTypeColorSwitch:
	case pTypeSecurity1:
	case pTypeSecurity2:
	case pTypeCurtain:
	case pTypeBlinds:
	case pTypeRFY:
	case pTypeThermostat2:
	case pTypeThermostat3:
	case pTypeThermostat4:
	case pTypeRemote:
	case pTypeGeneralSwitch:
	case pTypeHomeConfort:
	case pTypeFS20:
		bIsLightSwitch = true;
		break;
	case pTypeRadiator1:
		bIsLightSwitch = (subType == sTypeSmartwaresSwitchRadiator);
		break;
	}
	return bIsLightSwitch;
}

int MStoBeaufort(const float ms)
{
	if (ms < 0.3f)
		return 0;
	if (ms < 1.5f)
		return 1;
	if (ms < 3.3f)
		return 2;
	if (ms < 5.5f)
		return 3;
	if (ms < 8.0f)
		return 4;
	if (ms < 10.8f)
		return 5;
	if (ms < 13.9f)
		return 6;
	if (ms < 17.2f)
		return 7;
	if (ms < 20.7f)
		return 8;
	if (ms < 24.5f)
		return 9;
	if (ms < 28.4f)
		return 10;
	if (ms < 32.6f)
		return 11;
	return 12;
}

void FixFolderEnding(std::string &folder)
{
#if defined(WIN32)
	if (folder.at(folder.length() - 1) != '\\')
		folder += "\\";
#else
	if (folder.at(folder.length() - 1) != '/')
		folder += "/";
#endif
}

bool dirent_is_directory(const std::string &dir, struct dirent *ent)
{
	if (ent->d_type == DT_DIR)
		return true;
#ifndef WIN32
	if (ent->d_type == DT_LNK)
		return true;
	if (ent->d_type == DT_UNKNOWN) {
		std::string fname = dir + "/" + ent->d_name;
		struct stat st;
		if (!lstat(fname.c_str(), &st))
			return S_ISDIR(st.st_mode);
	}
#endif
	return false;
}

bool dirent_is_file(const std::string &dir, struct dirent *ent)
{
	if (ent->d_type == DT_REG)
		return true;
#ifndef WIN32
	if (ent->d_type == DT_UNKNOWN) {
		std::string fname = dir + "/" + ent->d_name;
		struct stat st;
		if (!lstat(fname.c_str(), &st))
			return S_ISREG(st.st_mode);
	}
#endif
	return false;
}

/*!
 * List entries of a directory.
 * @param entries A string vector containing the result
 * @param dir Target directory for listing
 * @param bInclDirs Boolean flag to include directories in the result
 * @param bInclFiles Boolean flag to include regular files in the result
 */
void DirectoryListing(std::vector<std::string>& entries, const std::string &dir, bool bInclDirs, bool bInclFiles)
{
	DIR *d = NULL;
	struct dirent *ent;
	if ((d = opendir(dir.c_str())) != NULL)
	{
		while ((ent = readdir(d)) != NULL) {
			std::string name = ent->d_name;
			if (bInclDirs && dirent_is_directory(dir, ent) && name != "." && name != "..") {
				entries.push_back(name);
				continue;
			}
			if (bInclFiles && dirent_is_file(dir, ent)) {
				entries.push_back(name);
				continue;
			}
		}
		closedir(d);
	}
	return;
}

std::string GenerateUserAgent()
{
	int cversion = rand() % 0xFFFF;
	int mversion = rand() % 3;
	int sversion = rand() % 3;
	std::stringstream sstr;
	sstr << "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/" << (601 + sversion) << "." << (36+mversion) << " (KHTML, like Gecko) Chrome/" << (53 + mversion) << ".0." << cversion << ".0 Safari/" << (601 + sversion) << "." << (36+sversion);
	return sstr.str();
}

std::string MakeHtml(const std::string &txt)
{
        std::string sRet = txt;

        stdreplace(sRet, "&", "&amp;");
        stdreplace(sRet, "\"", "&quot;");
        stdreplace(sRet, "'", "&apos;");
        stdreplace(sRet, "<", "&lt;");
        stdreplace(sRet, ">", "&gt;");
        stdreplace(sRet, "\r\n", "<br/>");
        return sRet;
}

//Prevent against XSS (Cross Site Scripting)
std::string SafeHtml(const std::string &txt)
{
    std::string sRet = txt;

    stdreplace(sRet, "\"", "&quot;");
    stdreplace(sRet, "'", "&apos;");
    stdreplace(sRet, "<", "&lt;");
    stdreplace(sRet, ">", "&gt;");
    return sRet;
}


#if defined WIN32
//FILETIME of Jan 1 1970 00:00:00
static const uint64_t epoch = (const uint64_t)(116444736000000000);

int gettimeofday( timeval * tp, void * tzp)
{
	FILETIME    file_time;
	SYSTEMTIME  system_time;
	ULARGE_INTEGER ularge;
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
	tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif

int getclock(struct timeval *tv) {
#ifdef CLOCK_MONOTONIC
	struct timespec ts;
		if (!clock_gettime(CLOCK_MONOTONIC, &ts)) {
			tv->tv_sec = ts.tv_sec;
			tv->tv_usec = ts.tv_nsec / 1000;
			return 0;
		}
#endif
	return gettimeofday(tv, NULL);
}
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
	/* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

const char *szInsecureArgumentOptions[] = {
	"import",
	"socket",
	"process",
	"os",
	"|",
	";",
	"&",
	"$",
	"<",
	">",
	"`",
	"\n",
	"\r",
	NULL
};

bool IsArgumentSecure(const std::string &arg)
{
	std::string larg(arg);
	std::transform(larg.begin(), larg.end(), larg.begin(), ::tolower);

	int ii = 0;
	while (szInsecureArgumentOptions[ii] != NULL)
	{
		if (larg.find(szInsecureArgumentOptions[ii]) != std::string::npos)
			return false;
		ii++;
	}
	return true;
}

uint32_t SystemUptime()
{
#if defined(WIN32)
	return static_cast<uint32_t>(GetTickCount64() / 1000u);
#elif defined(__linux__) || defined(__linux) || defined(linux)
	struct sysinfo info;
	if (sysinfo(&info) != 0)
		return -1;
	return info.uptime;
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
	struct timeval boottime;
	std::size_t len = sizeof(boottime);
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0)
		return -1;
	return time(NULL) - boottime.tv_sec;
#elif (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)) && defined(CLOCK_UPTIME)
	struct timespec ts;
	if (clock_gettime(CLOCK_UPTIME, &ts) != 0)
		return -1;
	return ts.tv_sec;
#else
	return 0;
#endif
}

// True random number generator (source: http://www.azillionmonkeys.com/qed/random.html)
static struct
{
	int which;
	time_t t;
	clock_t c;
	int counter;
} entropy = { 0, (time_t) 0, (clock_t) 0, 0 };

static unsigned char * p = (unsigned char *) (&entropy + 1);
static int accSeed = 0;

int GenerateRandomNumber(const int range)
{
	if (p == ((unsigned char *) (&entropy + 1)))
	{
		switch (entropy.which)
		{
			case 0:
				entropy.t += time (NULL);
				accSeed ^= entropy.t;
				break;
			case 1:
				entropy.c += clock();
				break;
			case 2:
				entropy.counter++;
				break;
		}
		entropy.which = (entropy.which + 1) % 3;
		p = (unsigned char *) &entropy.t;
	}
	accSeed = ((accSeed * (UCHAR_MAX + 2U)) | 1) + (int) *p;
	p++;
	srand (accSeed);
	return (rand() / (RAND_MAX / range));
}

int GetDirFilesRecursive(const std::string &DirPath, std::map<std::string, int> &_Files)
{
	DIR* dir;
	if ((dir = opendir(DirPath.c_str())) != NULL)
	{
		struct dirent *ent;
		while ((ent = readdir(dir)) != NULL)
		{
			if (dirent_is_directory(DirPath, ent))
			{
				if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0) && (strcmp(ent->d_name, ".svn") != 0))
				{
					std::string nextdir = DirPath + ent->d_name + "/";
					if (GetDirFilesRecursive(nextdir.c_str(), _Files))
					{
						closedir(dir);
						return 1;
					}
				}
			}
			else
			{
				std::string fname = DirPath + ent->d_name;
				_Files[fname] = 1;
			}
		}
	}
	closedir(dir);
	return 0;
}

#ifdef WIN32
// From https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
int SetThreadName(const std::thread::native_handle_type &thread, const char* threadName) {
	DWORD dwThreadID = ::GetThreadId( static_cast<HANDLE>( thread ) );
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
	__try{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
	}
#pragma warning(pop)
	return 0;
}
#else
// Based on https://stackoverflow.com/questions/2369738/how-to-set-the-name-of-a-thread-in-linux-pthreads
int SetThreadName(const std::thread::native_handle_type &thread, const char *name)
{
#if defined(__linux__) || defined(__linux) || defined(linux)
	char name_trunc[16];
	strncpy(name_trunc, name, sizeof(name_trunc));
	name_trunc[sizeof(name_trunc)-1] = '\0';
	return pthread_setname_np(thread, name_trunc);
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
	// Not possible to set name of other thread: https://stackoverflow.com/questions/2369738/how-to-set-the-name-of-a-thread-in-linux-pthreads
	return 0;
#elif defined(__NetBSD__)
	char name_trunc[PTHREAD_MAX_NAMELEN_NP];
	strncpy(name_trunc, name, sizeof(name_trunc));
	name_trunc[sizeof(name_trunc)-1] = '\0';
	return pthread_setname_np(thread, "%s", (void *)name_trunc);
#elif defined(__OpenBSD__) || defined(__DragonFly__)
	char name_trunc[PTHREAD_MAX_NAMELEN_NP];
	strncpy(name_trunc, name, sizeof(name_trunc));
	name_trunc[sizeof(name_trunc)-1] = '\0';
	pthread_setname_np(thread, name_trunc);
	return 0;
#elif defined(__FreeBSD__)
	char name_trunc[PTHREAD_MAX_NAMELEN_NP];
	strncpy(name_trunc, name, sizeof(name_trunc));
	name_trunc[sizeof(name_trunc)-1] = '\0';
	pthread_set_name_np(thread, name_trunc);
	return 0;
#endif
}
#endif

#if !defined(WIN32)
bool IsDebuggerPresent(void)
{
#if defined(__linux__)
	// Linux implementation: Search for 'TracerPid:' in /proc/self/status
	char buf[4096];

	const int status_fd = ::open("/proc/self/status", O_RDONLY);
	if (status_fd == -1)
		return false;

	const ssize_t num_read = ::read(status_fd, buf, sizeof(buf) - 1);
	if (num_read <= 0)
		return false;

	buf[num_read] = '\0';
	constexpr char tracerPidString[] = "TracerPid:";
	const auto tracer_pid_ptr = ::strstr(buf, tracerPidString);
	if (!tracer_pid_ptr)
		return false;

	for (const char* characterPtr = tracer_pid_ptr + sizeof(tracerPidString) - 1; characterPtr <= buf + num_read; ++characterPtr)
	{
		if (::isspace(*characterPtr))
			continue;
		else
			return ::isdigit(*characterPtr) != 0 && *characterPtr != '0';
	}

	return false;
#else
	// MacOS X / BSD: TODO
#	ifdef _DEBUG
	return true;
#	else
	return false;
#	endif
#endif
}
#endif

#if defined(__linux__)
bool IsWSL(void)
{
	// Detect WSL according to https://github.com/Microsoft/WSL/issues/423#issuecomment-221627364
	bool is_wsl = false;

	char buf[1024];

	int status_fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
	if (status_fd == -1)
		return is_wsl;

	ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);

	if (num_read > 0)
	{
		buf[num_read] = 0;
		is_wsl |= (strstr(buf, "Microsoft") != NULL);
		is_wsl |= (strstr(buf, "WSL") != NULL);
	}

	status_fd = open("/proc/version", O_RDONLY);
	if (status_fd == -1)
		return is_wsl;

	num_read = read(status_fd, buf, sizeof(buf) - 1);

	if (num_read > 0)
	{
		buf[num_read] = 0;
		is_wsl |= (strstr(buf, "Microsoft") != NULL);
		is_wsl |= (strstr(buf, "WSL") != NULL);
	}

	return is_wsl;
}
#endif

const std::string hexCHARS = "0123456789abcdef";
std::string GenerateUUID() // DCE/RFC 4122
{
	std::string uuid = std::string(36, ' ');

	uuid[8] = '-';
	uuid[13] = '-';
	uuid[14] = '4'; //M
	uuid[18] = '-';
	//uuid[19] = ' '; //N Variant 1 UUIDs (10xx N=8..b, 2 bits)
	uuid[23] = '-';

	for (size_t ii = 0; ii < uuid.size(); ii++)
	{
		if (uuid[ii] == ' ')
		{
			uuid[ii] = hexCHARS[(ii == 19) ? (8 + (std::rand() & 0x03)) : std::rand() & 0x0F];
		}
	}
	return uuid;
}
