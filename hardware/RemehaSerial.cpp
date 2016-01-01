#include "stdafx.h"
#include "RemehaSerial.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include "hardwaretypes.h"
#include "../hardware/openzwave/control_panel/tinyxml/tinyxml.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"

#define RETRY_DELAY 30

extern std::string szStartupFolder;

const static char RemehaHeader[3] = { 0x02, 0x01, 0xFE };

typedef struct
{
	BYTE start;
	BYTE source_addr;
	BYTE dest_addr;
	BYTE unk_protocol_id;
	BYTE size;
	BYTE unk_unit_id;
	BYTE unk_function;
	
	unsigned short CRC;
	BYTE end;
} RemehaDataSample;

//
// My Remeha is a Calenta 40C, which reports itself as PCU-02/03 (P4) in the Remeha Recom application
// Note that the below implementation is specific for this version
//
// TODO: Add a check to validate the device
//

// Interesting read about the Remeha protocol: http://blog.hekkers.net/2010/10/10/more-about-the-remeha-protocol/
// Remeha packet header information (all values are in HEX)
// Remeha packets look similar to MODBUS
// 02 - start byte
// 01 - source address
// FE - destination address
// 06 - unknown (modbus: protocol identifier)
// 48 - packet size (which includes everything except the start and stop bytes)
// 02 - unknown (modbus: Unit identifier)
// 01 - unknown (modbus: Function code)
// payload[x]
// crc16[2] - 2 bytes with Modbus-like CRC16 (initial value is 0xFF instead of 0x00)
// 03 - stop byte


//
//Class RemehaSerial
//
RemehaSerial::RemehaSerial(const int ID, const std::string& devname, const unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequestedpoller=false;
	m_bufferpos = 0;

	ReadLanguageFile("en");
	ReadConfigFile("PCU-03_P4.xml");

	_log.Log(LOG_NORM, "RemehaSerial: Data packet length: %d", sizeof(RemehaDataSample));
}

RemehaSerial::~RemehaSerial()
{
	clearReadCallback();
}


void RemehaSerial::SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
}

void RemehaSerial::ParseData(const unsigned char *pData, int Len)
{
	RemehaDataSample* data = (RemehaDataSample*)m_buffer;

	for (unsigned int ii = 0; ii<Len; ii++)
	{
		m_buffer[m_bufferpos++] = pData[ii];
		if(memcmp(m_buffer, RemehaHeader, std::min(sizeof(RemehaHeader), m_bufferpos)) != 0)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Incorrect header");
			// discard anything
			m_bufferpos = 0;
			continue;
		}
		if (m_bufferpos < sizeof(RemehaDataSample))
			continue;

		if (data->end != 0x03)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Data packet didn't end correctly");
			m_bufferpos = 0;
			continue;
		}
		
		if (data->size != 0x48)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Unkown data packet length: %d", data->size);
			m_bufferpos = 0;
			continue;
		}

#if 0
		char d[4];
		std::string debug;
		for (int i = 0; i<data->size + 2; i++) {
			sprintf(d, "%02x-", m_buffer[i]);
			debug += std::string(d);
		}
		_log.Log(LOG_NORM, "RemehaSerial: Received data: %s", debug.c_str());
#endif

		// Validate the CRC
		unsigned short crc = Crc16(&m_buffer[1], data->size - 2);
		if (memcmp(data->CRC, &crc, sizeof(crc)) != 0)
		{
			unsigned short packet_crc;
			memcpy(data->CRC, &packet_crc, sizeof(packet_crc));
			_log.Log(LOG_ERROR, "RemehaSerial: Data packet CRC not matching: 0x%04x while 0x%04x expected", crc, packet_crc);
			m_bufferpos = 0;
			continue;
		}
		ParseSample(data);
		m_bufferpos = 0;
	}
}

void RemehaSerial::ParseSample(RemehaDataSample* data)
{
	_log.Log(LOG_NORM, "RemehaSerial: Extracting valid packet");
	int i = 1;
	for (std::list<Sample>::const_iterator it = m_SampleInfo.begin(); it != m_SampleInfo.end(); i++, it++)
	{
		const Sample& sample = *it;

		if (sample.expression == std::string("A"))
		{
			BYTE value = data->DATA[sample.byte];
			/*if (sample.node_type == std::string("selections") || !sample.selection.empty())
			{
				// skip the fields for now
				continue;
			}*/
			if (sample.type == std::string("bit"))
			{
				BYTE value = (data->DATA[sample.byte] >> sample.bit) & 1;
				if (sample.invert)
					value ^= 0x1;
				_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %d %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), value, m_languagestrings[sample.unit].c_str());
			}
			else if (sample.type == std::string("field"))
			{
				int iValue = data->DATA[sample.byte];
				if (iValue >= sample.min && iValue <= sample.max)
				{
					if (sample.unit == 346) // %
						SendPercentageSensor(i, 0, 255, iValue, m_languagestrings[sample.description].c_str());
					else
						_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %d %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), iValue, m_languagestrings[sample.unit].c_str());
				}
			}
			else
			{
				int iValue = data->DATA[sample.byte];
				if (iValue >= sample.min && iValue <= sample.max)
				{
					_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %d %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), iValue, m_languagestrings[sample.unit].c_str());
				}
			}
		}
		else if (sample.expression == std::string("sgn(A.0 + B.1) x 0.01"))
		{
			float fValue = (data->DATA[sample.byte] + (data->DATA[sample.byte+1] << 8)) * 0.01f;
			if (fValue >= sample.min && fValue <= sample.max)
			{
				if (sample.unit == 312) // temperature (C)
					SendTempSensor(i, 255, fValue, m_languagestrings[sample.description].c_str());
				else
					_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %.2f %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), fValue, m_languagestrings[sample.unit].c_str());
			}
		}
		else if (sample.expression == std::string("A.0 + B.1"))
		{
			int iValue = data->DATA[sample.byte] + (data->DATA[sample.byte+1] << 8);
			if (iValue >= sample.min && iValue <= sample.max)
			{
				_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %d %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), iValue, m_languagestrings[sample.unit].c_str());
			}
		}
		else if (sample.expression == std::string("A x 0.1"))
		{
			float fValue = data->DATA[sample.byte] * 0.1f;
			if (fValue >= sample.min && fValue <= sample.max)
			{
				_log.Log(LOG_NORM, "RemehaSerial: %s - %s - %.1f %s", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), fValue, m_languagestrings[sample.unit].c_str());
			}
		}
		else
		{
			_log.Log(LOG_NORM, "RemehaSerial: UNHANDLED EXPRESSION: %s - %s (expression: %s)", m_languagestrings[sample.name].c_str(), m_languagestrings[sample.description].c_str(), sample.expression.c_str());
		}
	}
}

bool RemehaSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}


void RemehaSerial::RequestActualValues()
{
	_log.Log(LOG_NORM, "RemehaSerial: Requesting values");
	const unsigned char szCmd[10] = { 0x02, 0xfe, 0x01, 0x05, 0x08, 0x02, 0x01, 0x69, 0xab, 0x03 };
	WriteInt(szCmd, sizeof(szCmd));
}

bool RemehaSerial::StartHardware()
{
	m_retrycntr=RETRY_DELAY; //will force reconnect first thing
	StartPollerThread();
	return true;
}

bool RemehaSerial::StopHardware()
{
	m_bIsStarted=false;
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	StopPollerThread();
	return true;
}


void RemehaSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void RemehaSerial::StartPollerThread()
{
	m_pollerthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RemehaSerial::Do_PollWork, this)));
}

void RemehaSerial::StopPollerThread()
{
	if (m_pollerthread!=NULL)
	{
		assert(m_pollerthread);
		m_stoprequestedpoller = true;
		m_pollerthread->join();
	}
}

bool RemehaSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_NORM,"RemehaSerial: Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RemehaSerial: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RemehaSerial: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_bufferpos=0;
	setReadCallback(boost::bind(&RemehaSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void RemehaSerial::Do_PollWork()
{
	bool bFirstTime=true;
	int sec_counter = 25;
	while (!m_stoprequestedpoller)
	{
		sleep_seconds(1);

		sec_counter++;

		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat=mytime(NULL);
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"RemehaSerial: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (OpenSerialDevice())
				{
					bFirstTime = true;
				}
			}
		}
		else
		{
			if ((sec_counter % 30 == 0) || (bFirstTime))	//updates every 30 seconds
			{
				bFirstTime = false;
				RequestActualValues();
			}
		}
	}
	_log.Log(LOG_NORM,"RemehaSerial: Worker stopped...");
}

bool RemehaSerial::WriteInt(const unsigned char *pData, const unsigned char Len)
{
	if (!isOpen())
		return false;
	write((const char*)pData, Len);
	return true;
}

unsigned short RemehaSerial::crc16_update(unsigned short crc, unsigned char a)
{
	crc ^= a;
	for (int i = 0; i < 8; ++i)
	{
		if (crc & 1)
			crc = (crc >> 1) ^ 0xA001;
		else
			crc = (crc >> 1);
	}

	return crc;
}

unsigned short RemehaSerial::Crc16(const unsigned char* byteArray,size_t arraySize)
{
	// Note that the initial value is different from normal CRC16 (like MODBUS)
	unsigned short crc16=0xFFFF;
	for (size_t i = 0; i < arraySize; i++)
	{
		crc16 = crc16_update(crc16, byteArray[i]);
	}
	return crc16;
}

void RemehaSerial::ReadLanguageFile(std::string lang)
{
	m_languagestrings.clear();
	m_languagestrings[-1] = std::string("");

	std::string sLanguageFilePath = szStartupFolder + "Config/remeha/language/language-" + lang + ".xml";

	TiXmlDocument doc;
	if (!doc.LoadFile(sLanguageFilePath.c_str()))
	{
		_log.Log(LOG_ERROR, "RemehaSerial: Unable to parse language file\n%d - %s", doc.ErrorId(), doc.ErrorDesc());
		return;
	}


	TiXmlElement *pRoot = doc.FirstChildElement("language");
	if (pRoot == NULL)
	{
		_log.Log(LOG_ERROR, "RemehaSerial: Unable to parse language file - no language element");
		return;
	}

	// parse all the translations
	for (TiXmlElement *pText = pRoot->FirstChildElement("text"); pText != NULL; pText = pText->NextSiblingElement("text"))
	{
		int id = -1;
		std::string translation;

		if (pText->QueryIntAttribute("id", &id) != TIXML_SUCCESS)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Failed to get id from language file");
			continue;
		}

		const char * value = pText->Attribute("value");
		if (value == NULL)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Failed to get value from language file (id=%d)", id);
			continue;
		}
		translation.assign(value);

		// Place the translation in the map
		m_languagestrings[id] = translation;
	}
}

void RemehaSerial::XMLParseSelection(TiXmlElement * pSelections, tIntTXTMap& SelectionMap)
{
	if (pSelections == NULL)
		return;
	for (TiXmlElement *pSel = pSelections->FirstChildElement("selection"); pSel != NULL; pSel = pSel->NextSiblingElement("selection"))
	{
		int value, txtID;

		if (pSel->QueryIntAttribute("value", &value) != TIXML_SUCCESS)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Failed to get selection/value");
			continue;
		}
		if (pSel->QueryIntAttribute("result.nr", &txtID) != TIXML_SUCCESS)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Failed to get selection/result.nr");
			continue;
		}
		SelectionMap[value] = txtID;
	}
}

void RemehaSerial::ReadConfigFile(std::string sFileName)
{
	std::string sConfigFilePath = szStartupFolder + "Config/remeha/config/" + sFileName;

	TiXmlDocument doc;
	if (!doc.LoadFile(sConfigFilePath .c_str()))
	{
		_log.Log(LOG_ERROR, "RemehaSerial: Unable to parse config file\n%d - %s", doc.ErrorId(), doc.ErrorDesc());
		return;
	}


	TiXmlElement *pRoot = doc.FirstChildElement("configuration");
	if (pRoot == NULL)
	{
		_log.Log(LOG_ERROR, "RemehaSerial: Unable to parse config file - no language element");
		return;
	}

	TiXmlElement *pProperties = pRoot->FirstChildElement("properties");
	if (pProperties != NULL)
	{
		const char *sBoilerName = "";
		const char *sVersion = "";
		for (TiXmlElement *pProperty = pProperties->FirstChildElement("property"); pProperty != NULL; pProperty = pProperty->NextSiblingElement("property"))
		{
			const char* sName;
			sName = pProperty->Attribute("name");
			if (strcmp(sName, "boilername") == 0)
				sBoilerName = pProperty->Attribute("value");
			if (strcmp(sName, "version") == 0)
				sVersion = pProperty->Attribute("value");
		}
		_log.Log(LOG_NORM, "RemehaSerial: %s (ver %s)", sBoilerName , sVersion );
	}


	m_SelectionsMap.clear();
	for (TiXmlElement *pSelections = pRoot->FirstChildElement("selections"); pSelections != NULL; pSelections = pSelections->NextSiblingElement("selections"))
	{
		if (pSelections == NULL) {
			_log.Log(LOG_ERROR, "RemehaSerial: No selections found");
			break;
		}

		const char *sId = pSelections->Attribute("id");
		if (sId == NULL)
		{
			_log.Log(LOG_ERROR, "RemehaSerial: Failed to parse selection id attribute");
			break;
		}

		tIntTXTMap Selection;
		XMLParseSelection(pSelections, Selection);
		std::string s(sId);
		m_SelectionsMap[s] = Selection;
	}

	m_SampleInfo.clear();
	for (TiXmlElement *pConfigurations = pRoot->FirstChildElement("configurations"); pConfigurations != NULL; pConfigurations = pConfigurations->NextSiblingElement("configurations"))
	{
		if (pConfigurations == NULL) {
			_log.Log(LOG_ERROR, "RemehaSerial: No configurations found");
			break;
		}

		const char *sName = pConfigurations->Attribute("name");
		if (strcmp(sName, "sample") != 0)
			continue;

		// fetch the config elements
		for (TiXmlElement *pConfig = pConfigurations->FirstChildElement("config"); pConfig != NULL; pConfig = pConfig->NextSiblingElement("config"))
		{
			const char *s;
			Sample data;
			s = pConfig->Attribute("type");
			if (s == NULL)
			{
				_log.Log(LOG_ERROR, "RemehaSerial: Failed to get config/type");
				continue;
			}
			data.type.assign(s);
			s = pConfig->Attribute("label");
			if (s != NULL)
			{
				data.label.assign(s);
			}
			if (pConfig->QueryIntAttribute("group", &data.group) != TIXML_SUCCESS)
			{
				_log.Log(LOG_ERROR, "RemehaSerial: Failed to get config/group");
				continue;
			}
			
			TiXmlElement *pPresent = pConfig->FirstChildElement("present");
			if (pPresent != NULL)
			{
				if (pPresent->QueryIntAttribute("name.nr", &data.name) != TIXML_SUCCESS)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get present/name.nr");
					continue;
				}
				if (pPresent->QueryIntAttribute("description.nr", &data.description) != TIXML_SUCCESS)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get present/description.nr");
					continue;
				}
				// optional:
				pPresent->QueryIntAttribute("unit.nr", &data.unit);
			}
			
			TiXmlElement *pFunction = pConfig->FirstChildElement("function");
			if (pFunction != NULL)
			{
				if (pFunction->QueryIntAttribute("byte", &data.byte) != TIXML_SUCCESS)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get function/byte");
				}
				s = pFunction->Attribute("expression");
				if (s == NULL)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get function/expression");
				}
				else
				{
					data.expression.assign(s);
				}
				s = pFunction->Attribute("format");
				if (s == NULL)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get function/format");
				}
				else
				{
					data.format.assign(s);
				}
				// optional:
				pFunction->QueryIntAttribute("bit", &data.bit);
				s = pFunction->Attribute("invert");
				if (s != NULL) {
					data.invert = (strcmp(s, "true") == 0);
				}
			}

			TiXmlElement *pNodeRef = pConfig->FirstChildElement("node-ref");
			if (pNodeRef != NULL)
			{
				s = pNodeRef->Attribute("type");
				if (s == NULL)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get node-ref/type");
				}
				else
				{
					data.node_type.assign(s);
				}
				s = pNodeRef->Attribute("id");
				if (s == NULL)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get node-ref/id");
				}
				else
				{
					data.node_id.assign(s);
				}
			}

			TiXmlElement *pLimits = pConfig->FirstChildElement("limits");
			if (pLimits != NULL)
			{
				if (pLimits->QueryIntAttribute("min", &data.min) != TIXML_SUCCESS)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get limits/min");
				}
				if (pLimits->QueryIntAttribute("max", &data.max) != TIXML_SUCCESS)
				{
					_log.Log(LOG_ERROR, "RemehaSerial: Failed to get limits/max");
				}
				// optional:
				pLimits->QueryIntAttribute("min.nr", &data.txt_min);
				pLimits->QueryIntAttribute("max.nr", &data.txt_max);
			}

			TiXmlElement *pSelections = pConfig->FirstChildElement("selections");
			if (pSelections != NULL)
			{
				XMLParseSelection(pSelections, data.selection);
			}

			m_SampleInfo.push_back(data);
		}
	}
}

RemehaSerial::Sample::Sample()
: group(-1),
  name(-1),
  description(-1),
  unit(-1),
  byte(-1),
  bit(-1),
  invert(false),
  min(0.0f),
  max(0.0f),
  txt_min(-1),
  txt_max(-1)
{
}
