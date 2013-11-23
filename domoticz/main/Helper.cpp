#include "stdafx.h"
#include "Helper.h"
#if !defined WIN32
	#include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <math.h>
#include <algorithm>


void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results)
{
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

std::string stdreplace(
	std::string result, 
	const std::string& replaceWhat, 
	const std::string& replaceWithWhat)
{
	while(1)
	{
		const int pos = result.find(replaceWhat);
		if (pos==-1) break;
		result.replace(pos,replaceWhat.size(),replaceWithWhat);
	}
	return result;
}

std::vector<std::string> GetSerialPorts(bool &bUseDirectPath)
{
	bUseDirectPath=false;

	std::vector<std::string> ret;
#if defined WIN32
	//windows
	COMMCONFIG cc;
	DWORD dwSize = sizeof(COMMCONFIG);
	
	char szPortName[40];
	for (int ii=0; ii<256; ii++)
	{
		sprintf(szPortName,"COM%d",ii);
		if (GetDefaultCommConfig(szPortName, &cc, &dwSize))
		{
			sprintf(szPortName,"COM%d",ii);
			ret.push_back(szPortName);
		}
	}
#else
	//scan /dev for /dev/ttyUSB* or /dev/ttyS* or /dev/tty.usbserial* or /dev/ttyAMA*

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
		while(de = readdir(d))
		{
			std::string fname = de->d_name;
			if (fname.find("ttyUSB")!=std::string::npos)
			{
				ret.push_back("/dev/" + fname);
			}
			if (fname.find("tty.usbserial")!=std::string::npos)
			{
				bUseDirectPath=true;
				ret.push_back("/dev/" + fname);
			}
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
		while(de = readdir(d))
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

std::string &stdstring_ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string &stdstring_rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &stdstring_trim(std::string &s)
{
	return stdstring_ltrim(stdstring_rtrim(s));
}

double CalculateDewPoint(double temp, int humidity)
{
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
