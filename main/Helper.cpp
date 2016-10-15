#include "stdafx.h"
#include "Helper.h"
#ifdef WIN32
#include "dirent_windows.h"
#else
#include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <sstream>
#include <openssl/md5.h>

#if defined WIN32
#include "../msbuild/WindowsHelper.h"
#endif

#include "RFXtrx.h"
#include "../hardware/hardwaretypes.h"

void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results)
{
	results.clear();
	size_t cutAt;
	while( (cutAt = str.find_first_of(delim)) != str.npos )
	{
		if(cutAt > 0)
		{
			results.push_back(str.substr(0,cutAt));
		}
		str = str.substr(cutAt+1);
	}
	if(str.length() > 0)
	{
		results.push_back(str);
	}
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
		std::vector<int>::const_iterator itt;
		for (itt = ports.begin(); itt != ports.end(); ++itt)
		{
			sprintf(szPortName, "COM%d", *itt);
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
			std::vector<std::string>::const_iterator itt;
			bool bFound = false;
			for (itt = ret.begin(); itt != ret.end(); ++itt)
			{
				if (*itt == szPortName)
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
			std::vector<SerialPortInfo>::const_iterator itt;
			for (itt = serialports.begin(); itt != serialports.end(); ++itt)
			{
				ret.push_back(itt->szPortName); // add port
			}
		}
	}

#else
	//scan /dev for /dev/ttyUSB* or /dev/ttyS* or /dev/tty.usbserial* or /dev/ttyAMA* or /dev/ttySAC*

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
#ifdef __FreeBSD__            
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
pressure (in hPa), sea-level pressure (in hPa), and temperature (in �C)
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
	/* T = temperature (in �C) */
	return (((float)pow((seaLevel / atmospheric), 0.190223F) - 1.0F)
		* (temp + 273.15F)) / 0.0065F;
}

/**************************************************************************/
/*!
Calculates the sea-level pressure (in hPa) based on the current
altitude (in meters), atmospheric pressure (in hPa), and temperature
(in �C)
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
	/* T = Temperature (in �C) */
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
#if (BOOST_VERSION < 105000)
	boost::this_thread::sleep(boost::posix_time::seconds(seconds));
#else
	boost::this_thread::sleep_for(boost::chrono::seconds(seconds));
#endif
}

void sleep_milliseconds(const long milliseconds)
{
#if (BOOST_VERSION < 105000)
	boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
#else
	boost::this_thread::sleep_for(boost::chrono::milliseconds(milliseconds));
#endif
}

int mkdir_deep(const char *szDirName, int secattr)
{
	char DirName[260];
	DirName[0] = 0;
	const char* p = szDirName;
	char* q = DirName; 
	while(*p)
	{
		if (('\\' == *p) || ('/' == *p))
		{
		 if (':' != *(p-1))
		 {
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(IMN_PIM)
			CreateDirectory(DirName, NULL);
#else
			 mkdir(DirName,secattr);
#endif
		 }
		}
		*q++ = *p++;
		*q = '\0';
	}
	if (DirName[0])
	{
		#if (defined(__WIN32__) || defined(_WIN32)) && !defined(IMN_PIM)
				CreateDirectory(DirName, NULL);
		#else
				mkdir(DirName, secattr);
		#endif
	}
	return 0;
}

double ConvertToCelsius(const double Fahrenheit)
{
	return (Fahrenheit-32.0)/1.8;
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

std::vector<std::string> ExecuteCommandAndReturn(const std::string &szCommand)
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
			_pclose(fp);
#else
			pclose(fp);
#endif
		}
	}
	catch (...)
	{
		
	}
	return ret;
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

void hue2rgb(const float hue, int &outR, int &outG, int &outB, const double maxValue)
{
	double      hh, p, q, t, ff;
	long        i;
	hh = hue;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	double saturation = 1.0;
	double vlue = 1.0;
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
	case pTypeLimitlessLights:
	case pTypeSecurity1:
	case pTypeSecurity2:
	case pTypeCurtain:
	case pTypeBlinds:
	case pTypeRFY:
	case pTypeThermostat2:
	case pTypeThermostat3:
	case pTypeRemote:
	case pTypeGeneralSwitch:
	case pTypeHomeConfort:
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

bool dirent_is_directory(std::string dir, struct dirent *ent)
{
	if (ent->d_type == DT_DIR)
		return true;
#ifndef WIN32
	if (ent->d_type == DT_UNKNOWN) {
		std::string fname = dir + "/" + ent->d_name;
		struct stat st;
		if (!lstat(fname.c_str(), &st))
			return S_ISDIR(st.st_mode);
	}
#endif
	return false;
}

bool dirent_is_file(std::string dir, struct dirent *ent)
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

std::string GenerateUserAgent()
{
	srand((unsigned int)time(NULL));
	int cversion = rand() % 0xFFFF;
	int mversion = rand() % 3;
	int sversion = rand() % 3;
	std::stringstream sstr;
	sstr << "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/" << (601 + sversion) << "." << (36+mversion) << " (KHTML, like Gecko) Chrome/" << (53 + mversion) << ".0." << cversion << ".0 Safari/" << (601 + sversion) << "." << (36+sversion);
	return sstr.str();
}
