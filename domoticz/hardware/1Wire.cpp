
#include "stdafx.h"
#include "1Wire.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef WIN32
	#include "../main/dirent_windows.h"
#else
	#include <dirent.h>
#endif

#define Wire1_POLL_INTERVAL 30

//Support for w1-gpio and OWFS

#ifdef _DEBUG
	#define Wire1_Base_Dir "E:\\w1\\devices"
	#define OWFS_Base_Dir "E:\\w1\\1wire\\uncached"
#else
	#define Wire1_Base_Dir "/sys/bus/w1/devices"
	#define OWFS_Base_Dir "/mnt/1wire/uncached"
#endif

#define round(a) ( int ) ( a + .5 )

C1Wire::C1Wire(const int ID)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_bIsGPIO=IsGPIOSystem();
	m_bIsOWFS=IsOWFSSystem();
	Init();
}

C1Wire::~C1Wire(void)
{
}

void C1Wire::Init()
{
	m_LastPollTime=time(NULL)-Wire1_POLL_INTERVAL+2;
}

bool C1Wire::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&C1Wire::Do_Work, this)));
	sOnConnected(this);

	return (m_thread!=NULL);
}

bool C1Wire::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    return true;
}

void C1Wire::Do_Work()
{
	time_t atime;
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		atime=time(NULL);
		if (atime-m_LastPollTime>=Wire1_POLL_INTERVAL)
		{
			GetSensorDetails();
			m_LastPollTime=time(NULL);
		}
	}
	_log.Log(LOG_NORM,"1-Wire Worker stopped...");
}

void C1Wire::WriteToHardware(const char *pdata, const unsigned char length)
{

}

bool C1Wire::IsGPIOSystem()
{
	//Check if system have the w1-gpio interface
	std::ifstream infile1wire;
#ifdef _DEBUG
	std::string wire1catfile="E:\\w1\\devices";
#else
	std::string wire1catfile="/sys/bus/w1/devices";
#endif
	wire1catfile+="/w1_bus_master1/w1_master_slaves";
	infile1wire.open(wire1catfile.c_str());
	if (infile1wire.is_open())
	{
		infile1wire.close();
		return true;
	}
	return false;
}

bool C1Wire::IsOWFSSystem()
{
	DIR *d=NULL;

	d=opendir(OWFS_Base_Dir);
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while(de = readdir(d))
		{
			std::string dirname = de->d_name;
			if (de->d_type==DT_DIR) {
				if ((dirname!=".")&&(dirname!=".."))
				{
					closedir(d);
					return true;
				}
			}
		}
		closedir(d);
	}
	return false;
}


bool C1Wire::Have1WireSystem()
{
	return (IsGPIOSystem()||IsOWFSSystem());
}

void C1Wire::GetSensorDetails()
{
	if (m_bIsOWFS)
		GetOWFSSensorDetails();
	else if (m_bIsGPIO)
		GetGPIOSensorDetails();
}

void C1Wire::GetGPIOSensorDetails()
{
	std::string catfile=Wire1_Base_Dir;
	catfile+="/w1_bus_master1/w1_master_slaves";
	std::vector<std::string> _devices;
	std::string sLine = "";
	std::ifstream infile;

	infile.open(catfile.c_str());
	if (infile.is_open())
	{
		while (!infile.eof())
		{
			getline(infile, sLine);
			if (sLine.size()!=0)
				_devices.push_back(sLine);
		}
	}
	if (_devices.size()==0)
		return;
	std::vector<std::string>::const_iterator itt;
	for (itt=_devices.begin(); itt!=_devices.end(); ++itt)
	{
		std::string devfile=Wire1_Base_Dir;
		std::string devid=(*itt);
		devfile+="/"+devid+"/w1_slave";
		std::ifstream infile2;
		infile2.open(devfile.c_str());
		if (infile2.is_open())
		{
			bool bFoundCRC=false;
			while (!infile2.eof())
			{
				getline(infile2, sLine);
				if (sLine.find("crc=")!=std::string::npos)
				{
					if (sLine.find("YES")!=std::string::npos)
						bFoundCRC=true;
				}
				int tpos=sLine.find("t=");
				if ((tpos!=std::string::npos)&&(bFoundCRC))
				{
					std::string stemp=sLine.substr(tpos+2);
					float temp=(float)atoi(stemp.c_str())/1000.0f;
					if ((temp>-300)&&(temp<300))
					{
						std::string sID=devid.substr(devid.size()-4);

						unsigned int xID;   
						std::stringstream ss;
						ss << std::hex << sID;
						ss >> xID;
						//Temp
						RBUF tsen;
						memset(&tsen,0,sizeof(RBUF));
						tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
						tsen.TEMP.packettype=pTypeTEMP;
						tsen.TEMP.subtype=sTypeTEMP10;
						tsen.TEMP.battery_level=9;
						tsen.TEMP.rssi=6;
						tsen.TEMP.id1=(BYTE)((xID&0xFF00)>>8);
						tsen.TEMP.id2=(BYTE)(xID&0xFF);

						tsen.TEMP.tempsign=(temp>=0)?0:1;
						int at10=round(abs(temp*10.0f));
						tsen.TEMP.temperatureh=(BYTE)(at10/256);
						at10-=(tsen.TEMP.temperatureh*256);
						tsen.TEMP.temperaturel=(BYTE)(at10);

						sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
						m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP));
					}
				}
			}
		}
	}
}

void C1Wire::GetOWFSSensorDetails()
{
	DIR *d=NULL;

	d=opendir(OWFS_Base_Dir);
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while(de = readdir(d))
		{
			std::string dirname = de->d_name;
			if (de->d_type==DT_DIR)
			{
				if ((dirname!=".")&&(dirname!=".."))
				{
					std::string devfile=OWFS_Base_Dir;
					devfile+="/" + dirname + "/temperature";
					std::string devid=dirname;
					devid=devid.substr(3,4);
					std::ifstream infile2;
					infile2.open(devfile.c_str());
					if (infile2.is_open())
					{
						std::string sLine;
						getline(infile2, sLine);
						float temp=(float)atof(sLine.c_str());
						if ((temp>-300)&&(temp<300))
						{
							unsigned int xID;   
							std::stringstream ss;
							ss << std::hex << devid;
							ss >> xID;

							if (!((xID==0)&&(temp==0)))
							{
								//Temp
								RBUF tsen;
								memset(&tsen,0,sizeof(RBUF));
								tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
								tsen.TEMP.packettype=pTypeTEMP;
								tsen.TEMP.subtype=sTypeTEMP10;
								tsen.TEMP.battery_level=9;
								tsen.TEMP.rssi=6;
								tsen.TEMP.id1=(BYTE)((xID&0xFF00)>>8);
								tsen.TEMP.id2=(BYTE)(xID&0xFF);

								tsen.TEMP.tempsign=(temp>=0)?0:1;
								int at10=round(abs(temp*10.0f));
								tsen.TEMP.temperatureh=(BYTE)(at10/256);
								at10-=(tsen.TEMP.temperatureh*256);
								tsen.TEMP.temperaturel=(BYTE)(at10);

								sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
								m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP));
							}
						}
					}
				}
			}
		}
		closedir(d);
	}
}
