
#include "stdafx.h"
#include "1Wire.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"

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

typedef struct _t1WireSensor
{
	std::string devid;
	std::string filename;
} T1WIRESENSOR;

C1Wire::C1Wire(const int ID)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_bIsGPIO=false;
	m_bIsOWFS=false;
	Init();
#ifdef _DEBUG
	GetOWFSSensorDetails();
#endif
}

C1Wire::~C1Wire(void)
{
}

void C1Wire::Init()
{
	m_bDetectSystem=true;
	m_LastPollTime=mytime(NULL);
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
		atime=mytime(NULL);
		if (atime-m_LastPollTime>=Wire1_POLL_INTERVAL)
		{
			if (m_bDetectSystem)
			{
				m_bDetectSystem=false;
				m_bIsGPIO=IsGPIOSystem();
				m_bIsOWFS=IsOWFSSystem();
			}

			GetSensorDetails();
			m_LastPollTime=mytime(NULL);
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
					}
				}
			}
		}
	}
}

bool HaveSupported1WireSensor(const std::string basedir)
{
	std::string devfile=basedir+"/temperature";
	if (file_exist(devfile.c_str()))
		return true;
	devfile=basedir+"/humidity";
	if (file_exist(devfile.c_str()))
		return true;
	devfile=basedir+"/counters.A";
	if (file_exist(devfile.c_str()))
		return true;

	return false;
}

void C1Wire::GetOWFSSensorDetails()
{
	DIR *d=NULL;

	std::vector<_t1WireSensor> _wiresensors;

	d=opendir(OWFS_Base_Dir);
	if (d != NULL)
	{
		struct dirent *de=NULL;
		// Loop while not NULL
		while(de = readdir(d))
		{
			if (de->d_type==DT_DIR)
			{
				std::string dirname = de->d_name;
				if ((dirname==".")||(dirname=="..")||(dirname.size()<3))
					continue;
				unsigned char fchar=dirname[0];
				if (
					(dirname[2]=='.')&&
						(
							((fchar>='0')&&(fchar<='9'))||
							((fchar>='A')&&(fchar<='F'))
						)
					)
				{
					_t1WireSensor sensor;
					std::string devid=dirname;
					devid=devid.substr(3,4);
					sensor.devid=devid;
					sensor.filename=OWFS_Base_Dir;
					sensor.filename+="/" + dirname;

					std::string devfile=OWFS_Base_Dir;
					devfile+="/" + dirname;
					if (HaveSupported1WireSensor(devfile))
					{
						_wiresensors.push_back(sensor);
					}
					else
					{
						//Might be a w-wire hub
						DIR *d2=NULL;

						//First scan all /main folders
						std::string dirname2=OWFS_Base_Dir;
						dirname2+="/"+dirname;
						dirname2+="/main";
						d2=opendir(dirname2.c_str());
						if (d2!=NULL)
						{
							struct dirent *de2=NULL;
							while(de2 = readdir(d2))
							{
								if (de2->d_type==DT_DIR)
								{
									std::string dirname3 = de2->d_name;
									if ((dirname3!=".")&&(dirname3!=".."))
									{
										unsigned char fchar=dirname3[0];
										if (
											((fchar>='0')&&(fchar<='9'))||
											((fchar>='A')&&(fchar<='F'))
											)
										{
											std::string devfile=dirname2+"/"+dirname3;
											if (HaveSupported1WireSensor(devfile))
											{
												_t1WireSensor sensor;
												std::string devid=dirname3;
												devid=devid.substr(3,4);
												sensor.devid=devid;
												sensor.filename=dirname2+"/"+dirname3;
												_wiresensors.push_back(sensor);
											}
										}
									}
								}
							}
							closedir(d2);
						}
						//Next scan all /_aux folders
						dirname2=OWFS_Base_Dir;
						dirname2+="/"+dirname;
#ifdef WIN32							
						dirname2+="/_aux";
#else
						dirname2+="/aux";
#endif
						d2=opendir(dirname2.c_str());
						if (d2!=NULL)
						{
							struct dirent *de2=NULL;
							while(de2 = readdir(d2))
							{
								if (de2->d_type==DT_DIR)
								{
									std::string dirname3 = de2->d_name;
									if ((dirname3!=".")&&(dirname3!=".."))
									{
										unsigned char fchar=dirname3[0];
										if (
											((fchar>='0')&&(fchar<='9'))||
											((fchar>='A')&&(fchar<='F'))
											)
										{
											std::string devfile=dirname2+"/"+dirname3;
											if (HaveSupported1WireSensor(devfile))
											{
												_t1WireSensor sensor;
												std::string devid=dirname3;
												devid=devid.substr(3,4);
												sensor.devid=devid;
												sensor.filename=dirname2+"/"+dirname3;
												_wiresensors.push_back(sensor);
											}
										}
									}
								}
							}
							closedir(d2);
						}
					}
				}
			}
		}
		closedir(d);
	}
	//parse our sensors
	std::vector<_t1WireSensor>::const_iterator itt;
	for (itt=_wiresensors.begin(); itt!=_wiresensors.end(); ++itt)
	{
		std::string filename;
		float temp=0x1234;
		float humidity=0x1234;
		unsigned long counterA=0xFEDCBA98;

		unsigned int xID;   
		std::stringstream ss;
		ss << std::hex << itt->devid;
		ss >> xID;

		if (xID!=0)
		{
			//try to read temperature
			std::ifstream infile;
			filename=itt->filename+"/temperature";
			infile.open(filename.c_str());
			if (infile.is_open())
			{
				std::string sLine;
				getline(infile, sLine);
				float temp2=(float)atof(sLine.c_str());
				if ((temp2>-300)&&(temp2<300))
					temp=temp2;
				infile.close();
			}
			//try to read humidity
			std::ifstream infile2;
			filename=itt->filename+"/humidity";
			infile2.open(filename.c_str());
			if (infile2.is_open())
			{
				std::string sLine;
				getline(infile2, sLine);
				float himidity2=(float)atof(sLine.c_str());
				if ((himidity2>=0)&&(himidity2<=100))
					humidity=himidity2;
				infile2.close();
			}
			//try to read counters.A
			std::ifstream infile3;
			filename=itt->filename+"/counters.A";
			infile3.open(filename.c_str());
			if (infile3.is_open())
			{
				std::string sLine;
				getline(infile3, sLine);
				counterA=(unsigned long)atol(sLine.c_str());
				infile3.close();
			}
			if ((temp!=0x1234)&&(humidity==0x1234))
			{
				//temp
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
			}
			else if ((temp!=0x1234)&&(humidity!=0x1234))
			{
				//temp+hum
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
				tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
				tsen.TEMP_HUM.subtype=sTypeTH5;
				tsen.TEMP_HUM.battery_level=9;
				tsen.TEMP_HUM.rssi=6;
				tsen.TEMP.id1=(BYTE)((xID&0xFF00)>>8);
				tsen.TEMP.id2=(BYTE)(xID&0xFF);

				tsen.TEMP_HUM.tempsign=(temp>=0)?0:1;
				int at10=round(abs(temp*10.0f));
				tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
				at10-=(tsen.TEMP_HUM.temperatureh*256);
				tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
				tsen.TEMP_HUM.humidity=(BYTE)round(humidity);
				tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);//decode message
			}
			else if (humidity!=0x1234)
			{
				//humidity
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
				tsen.HUM.packettype=pTypeHUM;
				tsen.HUM.subtype=sTypeHUM2;
				tsen.HUM.battery_level=9;
				tsen.HUM.rssi=6;
				tsen.TEMP.id1=(BYTE)((xID&0xFF00)>>8);
				tsen.TEMP.id2=(BYTE)(xID&0xFF);

				tsen.HUM.humidity=(BYTE)round(humidity);
				tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);//decode message
			}
			else if (counterA!=0xFEDCBA98)
			{
				//Counter
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
				tsen.RFXMETER.packettype=pTypeRFXMeter;
				tsen.RFXMETER.subtype=sTypeRFXMeterCount;
				tsen.RFXMETER.rssi=6;
				tsen.RFXMETER.id1=(BYTE)((xID&0xFF00)>>8);
				tsen.RFXMETER.id2=(BYTE)(xID&0xFF);

				tsen.RFXMETER.count1 = (BYTE)((counterA & 0xFF000000) >> 24);
				tsen.RFXMETER.count2 = (BYTE)((counterA & 0x00FF0000) >> 16);
				tsen.RFXMETER.count3 = (BYTE)((counterA & 0x0000FF00) >> 8);
				tsen.RFXMETER.count4 = (BYTE)(counterA & 0x000000FF);
				sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);//decode message

			}
		}
	}
}
