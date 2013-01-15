#include "stdafx.h"
#include "Helper.h"
#if !defined WIN32
	#include <dirent.h>
#endif

void StringSplit(std::string str, std::string delim, std::vector<std::string> &results)
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

std::vector<std::string> GetSerialPorts()
{
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
	//scan /dev for /dev/ttyUSB* or /dev/ttyS* or /dev/tty.usbserial*
	DIR *d=NULL;
	d=opendir("/dev");
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while(de = readdir(d))
		{
			std::string fname = de->d_name;
			if (
				(fname.find("ttyUSB")!=std::string::npos)||
				(fname.find("tty.usbserial")!=std::string::npos)
				)
			{
				ret.push_back(fname);
			}
		}
	}	
#endif
	return ret;
}
