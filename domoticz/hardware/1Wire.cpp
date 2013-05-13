
#include "stdafx.h"
#include "1Wire.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"

#define Wire1_POLL_INTERVAL 30

#define Wire1_Base_Dir "/sys/bus/w1/devices"

#define round(a) ( int ) ( a + .5 )

//at this moment it does not work under windows... no idea why... help appriciated!
//for this we fake the data previously read by a station on unix (weatherdata.bin)

C1Wire::C1Wire(const int ID)
{
	m_HwdID=ID;
	m_stoprequested=false;
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

void C1Wire::GetSensorDetails()
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
