#include "stdafx.h"
#include "SolarEdgeBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"
#include <string>

#define SYNC_BYTE_1 0x12
#define SYNC_BYTE_2 0x34
#define SYNC_BYTE_3 0x56
#define SYNC_BYTE_4 0x79

#define round(a) ( int ) ( a + .5 )

SolarEdgeBase::SolarEdgeBase(void)
{
	m_state=E_WAITING_SYNC;
	m_bufferpos=0;
	m_wantedlength=0;
}


SolarEdgeBase::~SolarEdgeBase(void)
{
}

void SolarEdgeBase::ParseData(const unsigned char *pData, int Len)
{
	while (Len>0)
	{
		switch (m_state)
		{
		case E_WAITING_SYNC:
			if (m_bufferpos==0)
			{
				if (pData[0]==SYNC_BYTE_1)
				{
					m_RXBuffer[m_bufferpos++]=pData[0];
				}
			}
			else if (m_bufferpos==1)
			{
				if (pData[0]==SYNC_BYTE_2)
				{
					m_RXBuffer[m_bufferpos++]=pData[0];
				}
				else
					m_bufferpos=0;
			}
			else if (m_bufferpos==2)
			{
				if (pData[0]==SYNC_BYTE_3)
				{
					m_RXBuffer[m_bufferpos++]=pData[0];
				}
				else
					m_bufferpos=0;
			}
			else if (m_bufferpos==3)
			{
				if (pData[0]==SYNC_BYTE_4)
				{
					m_RXBuffer[m_bufferpos++]=pData[0];
					m_state=E_WAITING_LENGTH;
				}
				else
					m_bufferpos=0;
			}
			break;
		case E_WAITING_LENGTH:
			m_RXBuffer[m_bufferpos++]=pData[0];
			if (m_bufferpos==6)
			{
				//Length here
				m_wantedlength=(m_RXBuffer[5]<<8)|m_RXBuffer[4];
				if (m_wantedlength>1000)
				{
					//For sure something wrong here!
					m_bufferpos=0;
					m_state=E_WAITING_SYNC;
				}
				else
				{
					m_wantedlength+=(16+6);
					m_state=E_WAITING_DATA;
				}
			}
			break;
		case E_WAITING_DATA:
			m_RXBuffer[m_bufferpos++]=pData[0];
			if (m_bufferpos==m_wantedlength)
			{
				unsigned short ret=ParsePacket((unsigned char*)&m_RXBuffer,m_wantedlength);
				m_state = E_WAITING_SYNC;
				m_bufferpos=0;
			}
			break;
		}
		pData++;
		Len--;
	}
}

int SolarEdgeBase::ParsePacket(const unsigned char *pData, int len)
{
	if (len<8)
		return 0;

	const unsigned char *b=pData;
	int lenorg=len;

	int ii=0;

	unsigned long StartHeader=(b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3];
	if (StartHeader!=0x12345679)
	{
		_log.Log(LOG_ERROR,"SolarEdge: Invalid SYNC received!");
		return 0;
	}
	b+=4;
	len-=4;

	short *pShort;

	pShort=(short*)b;

	short dlen=*pShort+16;
	unsigned short orgdlen=dlen+6; //+6 because of previous header

	if (dlen==16)
	{
		//Empty packet, ignore
		return orgdlen;
	}

	unsigned short *pCustomID=(unsigned short *)(b+14);
	if (
		(*pCustomID==0x0090)
		)
	{
		//Dont want you
		return orgdlen;
	}

	//if (*pCustomID!=0x0500)
	//return orgdlen;

	b+=2;
	len-=2;

	if (len!=dlen)
	{
		_log.Log(LOG_ERROR,"SolarEdge: struct length mismatch!");
		return 0;
	}

	unsigned short TypeVal=*(unsigned short*)b;
	b+=2;
	dlen-=2;

	unsigned short Counter=*(short*)b;
	b+=2;
	dlen-=2;

	unsigned long ID1=*(unsigned long*)b;
	b+=4;
	dlen-=4;

	//2 strange pairs
	//sprintf_s(szTmp,"\"*%02X%02X\"",b[0],b[1]);
	b+=2;
	dlen-=2;
	//sprintf_s(szTmp,"\"*%02X%02X\"",b[0],b[1]);
	b+=2;
	dlen-=2;

	//sprintf_s(szTmp,"\"%04X\"",*pCustomID);
	b+=2;
	dlen-=2;
	if (dlen<3)
		return orgdlen; //no more data (except sync byte)

	if (*pCustomID==0x0280)
	{
		int ret=ParsePacket0x0280(b,dlen);
		b+=ret;
		dlen-=ret;
	}
	else if (*pCustomID==0x0281)
	{
		int ret=ParsePacket0x0281(b,dlen);
		b+=ret;
		dlen-=ret;
	}
	else if (*pCustomID==0x0282)
	{
		int ret=ParsePacket0x0282(b,dlen);
		b+=ret;
		dlen-=ret;
	}
	else if (*pCustomID==0x0283)
	{
		int ret=ParsePacket0x0283(b,dlen);
		b+=ret;
		dlen-=ret;
	}
	else if (*pCustomID==0x0500)
	{
		int ret=ParsePacket0x0500(b,dlen);
		b+=ret;
		dlen-=ret;
	}
	else
	{
		_log.Log(LOG_ERROR,"SolarEdge: Unknown Type: %04d",*pCustomID);
	}

	//CRC word left
	if (dlen!=2)
	{
		_log.Log(LOG_ERROR,"SolarEdge: Something not handled correctly, having extra bytes!");
	}
	return orgdlen;
}

bool SolarEdgeBase::GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal)
{
	if (m_pMainWorker==NULL)
		return false;
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name, sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		return false;
	}
	std::vector<std::string> splitresult;
	StringSplit(result[0][1],";",splitresult);
	if (splitresult.size()!=2)
		return false;
	musage=atof(splitresult[0].c_str());
	mtotal=atof(splitresult[1].c_str());
	return true;
}

void SolarEdgeBase::SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.ENERGY.packettype=pTypeENERGY;
	tsen.ENERGY.subtype=sTypeELEC2;
	tsen.ENERGY.id1=ID1;
	tsen.ENERGY.id2=ID2;
	tsen.ENERGY.count=1;
	tsen.ENERGY.rssi=12;

	tsen.ENERGY.battery_level=9;

	unsigned long long instant=(unsigned long long)(musage*1000.0);
	tsen.ENERGY.instant1=(unsigned char)(instant/0x1000000);
	instant-=tsen.ENERGY.instant1*0x1000000;
	tsen.ENERGY.instant2=(unsigned char)(instant/0x10000);
	instant-=tsen.ENERGY.instant2*0x10000;
	tsen.ENERGY.instant3=(unsigned char)(instant/0x100);
	instant-=tsen.ENERGY.instant3*0x100;
	tsen.ENERGY.instant4=(unsigned char)(instant);

	double total=(mtotal*1000.0)*223.666;
	tsen.ENERGY.total1=(unsigned char)(total/0x10000000000ULL);
	total-=tsen.ENERGY.total1*0x10000000000ULL;
	tsen.ENERGY.total2=(unsigned char)(total/0x100000000ULL);
	total-=tsen.ENERGY.total2*0x100000000ULL;
	tsen.ENERGY.total3=(unsigned char)(total/0x1000000);
	total-=tsen.ENERGY.total3*0x1000000;
	tsen.ENERGY.total4=(unsigned char)(total/0x10000);
	total-=tsen.ENERGY.total4*0x10000;
	tsen.ENERGY.total5=(unsigned char)(total/0x100);
	total-=tsen.ENERGY.total5*0x100;
	tsen.ENERGY.total6=(unsigned char)(total);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

void SolarEdgeBase::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype=sTypeVoltage;
	gDevice.id=1;
	gDevice.floatval1=Volt;
	gDevice.intval1=(int)Idx;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);
}

void SolarEdgeBase::SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=Idx;

	tsen.TEMP.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

float SolarEdgeBase::GetFloat(const unsigned char *pData)
{
	unsigned long ul=(pData[2]<<24)|(pData[3]<<16)|(pData[0]<<8)|pData[1];
	if (ul==0x7FFFFFFF)
		return 0.0f;
	float *pFloat=(float*)pData;
	return *pFloat;
}

//"12345679","0056","D3FF","00749","id","*FDFF","*FFFF","0280","325.60001","225.50000","1.60000","224.79999","50.00000","5.56000","15.39000","0.01220","0.01000","1254.00000","0.00000","B794"
int SolarEdgeBase::ParsePacket0x0280(const unsigned char *pData, int dlen)
{
	const unsigned char *b=pData;

	//11 Float values
	int totfloats=dlen/4;
	if (totfloats!=11)
		return 0;

	//DC Voltage
	float voltageDC=GetFloat(b);
	b+=4;
	dlen-=4;
	//AC Voltage1
	float voltageAC1=GetFloat(b);
	b+=4;
	dlen-=4;
	//Iets1
	float Iets1=GetFloat(b);
	b+=4;
	dlen-=4;
	//AC Voltage2
	float voltageAC2=GetFloat(b);
	b+=4;
	dlen-=4;
	//Frequency
	float freq=GetFloat(b);
	b+=4;
	dlen-=4;
	//Ampere
	float ampere=GetFloat(b);
	b+=4;
	dlen-=4;
	//Temp
	float temp=GetFloat(b);
	b+=4;
	dlen-=4;
	//Iets2
	float Iets2=GetFloat(b);
	b+=4;
	dlen-=4;
	//Iets3
	float Iets3=GetFloat(b);
	b+=4;
	dlen-=4;
	//Watt P-Out
	float Watt=GetFloat(b);
	b+=4;
	dlen-=4;
	//Iets4 (unsigned long)
	unsigned long *pUL=(unsigned long*)b;
	b+=4;
	dlen-=4;
/*
	double musage,mtotal;
	if (GetMeter(0,1,musage,mtotal))
	{
		SendMeter(0,1, Watt/100.0f, mtotal/1000.0f, "SolarMain");
		UpdateTempSensor(1,temp,"SolarMain");
	}
*/
	return (b-pData);
}

//"12345679","0044","DFFF","00750","id","*FDFF","*FFFF","0281","226.00000","1728.00000","0.00000","0.00000","0.99060","1.98170","32767.00000","0.00000","E19E"
int SolarEdgeBase::ParsePacket0x0281(const unsigned char *pData, int dlen)
{
	const unsigned char *b=pData;

	//8 Floats
	int totfloats=dlen/4;
	if (totfloats!=8)
		return 0;

	//AC Voltage
	float voltageAC1=GetFloat(b);
	b+=4;
	//Watt P-Out
	float Watt=GetFloat(b);
	b+=4;
/*
	double musage,mtotal;
	if (GetMeter(0,1,musage,mtotal))
	{
		SendMeter(0,1, Watt/100.0f, mtotal/1000.0f, "SolarMain");
	}
*/
	//skip the rest
	return dlen-2;
}

//"12345679","0052","D7FF","00747","id","*FDFF","*FFFF","0282","324.29999","224.89999","317.89999","1.60000","224.20000","-0.00300","50.01000","0.01180","1.00550","2.01350","9292"
int SolarEdgeBase::ParsePacket0x0282(const unsigned char *pData, int dlen)
{
	const unsigned char *b=pData;
	//10 Floats
	int totfloats=dlen/4;
	if (totfloats!=10)
		return 0;

	//DC1 Voltage
	float voltageDC1=GetFloat(b);
	b+=4;
	//AC1 Voltage
	float voltageAC1=GetFloat(b);
	b+=4;
	//DC2 Voltage
	float voltageDC2=GetFloat(b);
	b+=4;
	//Iets1
	float Iets1=GetFloat(b);
	b+=4;
	//AC2 Voltage
	float voltageAC2=GetFloat(b);
	b+=4;
	//Iets2
	float Iets2=GetFloat(b);
	b+=4;
	//Frequency
	float freq=GetFloat(b);
	b+=4;
	//skip the rest
	return dlen-2;
}

int SolarEdgeBase::ParsePacket0x0283(const unsigned char *pData, int dlen)
{
	const unsigned char *b=pData;

	short *pShort;
	pShort=(short*)b;
	//	sprintf_s(szTmp,"\"%04X\"",*pShort);
	b+=2;
	pShort=(short*)b;
	//	sprintf_s(szTmp,"\"%04X\"",*pShort);
	b+=2;
	//PanelID/Main
	unsigned long ID2=*(unsigned long*)b;
	//	sprintf_s(szTmp,"\"%08X\"",ID2);
	b+=4;

	//14 Floats
	int totfloats=(dlen-8)/4;
	if (totfloats!=14)
		return 0;

	//DC Voltage
	float voltageDC=GetFloat(b);
	b+=4;
	//AC Voltage
	float voltageAC=GetFloat(b);
	b+=4;
	//Watt P-Out
	float Watt=GetFloat(b);
	b+=4;
	//Frequency
	float freq=GetFloat(b);
	b+=4;
	//Temp
	float temp=GetFloat(b);
	b+=4;

/*
	double musage,mtotal;
	if (GetMeter(0,1,musage,mtotal))
	{
		SendMeter(0,1, Watt/100.0f, mtotal/1000.0f, "SolarMain");
		UpdateTempSensor(1,temp,"SolarMain");
	}
*/

	//skip the rest
	return dlen-2;
}

int SolarEdgeBase::ParsePacket0x0500(const unsigned char *pData, int dlen)
{
	const unsigned char *b=pData;
	short *pShort;

	int orgdlen=dlen;
	//Meter and Panel report
	while (dlen>20)
	{
		bool bIsPanel=false;
		bool bIsMain=false;
		bool bIs0300=false;
		//0000 for Panel, 0010 for Main?
		pShort=(short*)b;
		int ReportType=*pShort;
		b+=2;
		dlen-=2;

		if (ReportType==0x000)
		{
			bIsPanel=true;
			bIsMain=false;
			bIs0300=false;
		}
		else if (ReportType==0x0010)
		{
			bIsPanel=false;
			bIsMain=true;
			bIs0300=false;
		}
		else if (ReportType==0x0300)
		{
			bIsPanel=false;
			bIsMain=false;
			bIs0300=true;
		}
		else
		{
			//don't know you!
			return orgdlen-2;
		}

		//PanelID/Main
		unsigned long ID2=*(unsigned long*)b;
		//sprintf_s(szTmp,"\"%08X\"",ID2);
		b+=4;
		dlen-=4;
		//rest bytes
		pShort=(short*)b;
		int restbytes=*pShort;
		//sprintf_s(szTmp,"\"%04d\"",restbytes);
		b+=2;
		dlen-=2;

		const unsigned char *b2=(const BYTE*)b;
		int len2=restbytes;
		//Something
		//sprintf_s(szTmp,"\"%02X%02X\"",b2[0],b2[1]);
		b2+=2;
		len2-=2;
		//F052 (F352 for 0300)
		b2+=2;
		len2-=2;
		//2 times ID
		b2+=4;
		len2-=4;
		//uL=*(unsigned long*)b2;
		b2+=4;
		len2-=4;

		if (bIsMain)
		{
			//Temp
			float temp=GetFloat(b2);
			b2+=4;
			//Watt P-Out
			float Watt=GetFloat(b2);
			b2+=4;
			float Pac=GetFloat(b2);
			b2+=4;
			//AC Voltage
			float voltageAC=GetFloat(b2);
			b2+=4;
			//Iets2
			float Iets2=GetFloat(b2);
			b2+=4;
			//Frequency
			float freq=GetFloat(b2);
			b2+=4;
			//Iets3
			float Iets3=GetFloat(b2);
			b2+=4;
			//Iets4
			float Iets4=GetFloat(b2);
			b2+=4;
			//DC Voltage
			float voltageDC=GetFloat(b2);
			b2+=4;
			//Iets5
			float Iets5=GetFloat(b2);
			b2+=4;
			//Counter
			float counter=GetFloat(b2);
			b2+=4;
			SendMeter(0,1, Pac/100.0f, counter/1000.0f, "SolarMain");
			SendTempSensor(1,temp,"SolarMain");
		}
		b+=restbytes;
		dlen-=restbytes;
		continue;
	}
	return (b-pData);
}

