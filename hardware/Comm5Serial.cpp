#include "stdafx.h"
#include "Comm5Serial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <iostream>

#include <boost/lexical_cast.hpp>

#define RETRY_DELAY 30
#define Max_Comm5_MA_Relais 16

static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

static inline std::vector<std::string> tokenize(const std::string &s) {
	std::vector<std::string> tokens;
	std::istringstream iss(s);
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter(tokens));
	return tokens;
}

static inline bool startsWith(const std::string &haystack, const std::string &needle) {
	return needle.length() <= haystack.length()
		&& std::equal(needle.begin(), needle.end(), haystack.begin());
}

Comm5Serial::Comm5Serial(const int ID, const std::string& devname, unsigned int baudRate /*= 115200*/) :
	m_szSerialPort(devname),
	m_baudRate(baudRate)
{
	m_HwdID=ID;
	m_stoprequested=false;
	lastKnownSensorState = 0;
	initSensorData = true;
	reqState = Idle;
	notificationEnabled = false;
	m_bReceiverStarted = false;
	currentState = STSTART_OCTET1;
	frameSize = 0;
	frameCRC = 0;
}

bool Comm5Serial::WriteToHardware(const char * pdata, const unsigned char length) {
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2 && pSen->LIGHTING2.id3 == 0)
	{
		//light command

		uint8_t Relay = pSen->LIGHTING2.id4 - 1;
		if (Relay > 8)
			return false;
		std::string data("\x02\x02", 2);
		uint8_t relayMask = relayStatus;
		if (pSen->LIGHTING2.cmnd == light2_sOff) {
			relayMask &= ~(1 << Relay);
		} else {
			relayMask |= 1 << Relay;
		}

		data.push_back(relayMask);
		writeFrame(data);

		relayStatus = relayMask;
		return true;
	}
	return false;

}

bool Comm5Serial::StartHardware()
{
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Comm5Serial::Do_Work, this)));

	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"Comm5 MA-4200: Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_baudRate,
			boost::asio::serial_port_base::parity(
			boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"Comm5 MA-4200: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"Comm5 MA-4200: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&Comm5Serial::readCallBack, this, _1, _2));
	
	sOnConnected(this);
	return true;
}

bool Comm5Serial::StopHardware()
{
	terminate();
	m_stoprequested = true;
	if (m_thread)
	{
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
	m_bIsStarted = false;
	return true;
}

void Comm5Serial::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;
	
	queryRelayState();
	querySensorState();
	enableNotifications();

	while (!m_stoprequested)
	{
		sleep_milliseconds(200);
		if (m_stoprequested)
			break;
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;

			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
		}
	}
	_log.Log(LOG_STATUS, "Comm5 MA-42XX: Serial Worker stopped...");
}

void Comm5Serial::requestDigitalInputResponseHandler(const std::string & frame)
{
	uint8_t mask = frame[5];
	uint8_t sensorStatus = frame[6];

	for (int i = 0; i < 8; ++i) {
		bool on = (sensorStatus & (1 << i)) != 0 ? true : false;
		if (((lastKnownSensorState & (1 << i)) ^ (sensorStatus & (1 << i))) || initSensorData) {
			SendSwitch((i + 1) << 8, 1, 255, on, 0, "Sensor " + boost::lexical_cast<std::string>(i + 1));
		}
	}
	lastKnownSensorState = sensorStatus;
	initSensorData = false;
	reqState = Idle;
}

void Comm5Serial::requestDigitalOutputResponseHandler(const std::string & frame)
{
	//  0     1     2       3       4        5        6
	// 0x05 0x64 <size> <control> <id> <operation> <value>

	relayStatus = frame[6];
	for (int i = 0; i < 8; ++i) {
		bool on = (relayStatus & (1 << i)) != 0 ? true : false;
		SendSwitch(i + 1, 1, 255, on, 0, "Relay " + boost::lexical_cast<std::string>(i + 1));
	}
}

void Comm5Serial::enableNotificationResponseHandler(const std::string & frame)
{
	uint8_t operation = frame[5];
	uint8_t operationType = frame[6];
	uint8_t mask = frame[7];

}

void Comm5Serial::readCallBack(const char * data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

static uint16_t crc16_update(uint16_t crc, uint8_t data)
{
#define POLYNOME 0x13D65
	int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i = 0; i<8; i++)
	{
		if (crc & 0x8000)
			crc = (crc << 1) ^ POLYNOME;
		else
			crc <<= 1;
	}

	return crc;
#undef POLYNOME
}

void Comm5Serial::ParseData(const unsigned char* data, const size_t len)
{

	uint16_t readCRC;
	for (size_t i = 0; i < len; ++i) {
		switch ( currentState ) {
		case STSTART_OCTET1:
			frame.clear();
			if (data[i] == 0x05) {
				frame.push_back(data[i]);
				frameCRC = crc16_update(0, data[i]);
				currentState = STSTART_OCTET2;

			}
			else {
				_log.Log(LOG_ERROR, "Comm5 MA-4200: Framing error");
				currentState = STSTART_OCTET1;
			}
			break;

		case STSTART_OCTET2:
			if (data[i] == 0x64) {
				frame.push_back(data[i]);
				frameCRC = crc16_update(frameCRC, data[i]);
				currentState = STFRAME_SIZE;
			}
			else {
				_log.Log(LOG_ERROR, "Comm5 MA-4200: Framing error");
				currentState = STSTART_OCTET1;
			}
			break;

		case STFRAME_SIZE:
			frameSize = data[i] + 2 + 1 + 1; // FrameSize + Start Tokens + Control Byte + Frame Size field
			frame.push_back(data[i]);
			frameCRC = crc16_update(frameCRC, data[i]);
			currentState = STFRAME_CONTROL;
			break;

		case STFRAME_CONTROL:
			frame.push_back(data[i]);
			frameCRC = crc16_update(frameCRC, data[i]);
			currentState = STFRAME_DATA;
			break;

		case STFRAME_DATA:
			frame.push_back(data[i]);
			frameCRC = crc16_update(frameCRC, data[i]);
			
			if ( frame.size() >= frameSize )
				currentState = STFRAME_CRC1;
			break;
		
		case STFRAME_CRC1:
			frame.push_back(data[i]);
			frameCRC = crc16_update(frameCRC, 0);
			currentState = STFRAME_CRC2;
			break;

		case STFRAME_CRC2:
			frame.push_back(data[i]);
			frameCRC = crc16_update(frameCRC, 0);
			readCRC =  (uint16_t)(frame.at(frame.size() - 2) << 8) | (frame.at(frame.size() - 1) & 0xFF);
			if (frameCRC == readCRC)
				parseFrame(frame);
			currentState = STSTART_OCTET1;
			frame.clear();
			break;
		}

	}
}

void Comm5Serial::parseFrame(std::string & frame)
{
	switch (frame.at(4)) {
	case 0x01:
		requestDigitalInputResponseHandler(frame);
		break;
	case 0x02:
		requestDigitalOutputResponseHandler(frame);
		break;
	case 0x04:
		enableNotificationResponseHandler(frame);
		break;
	}
}

bool Comm5Serial::writeFrame(const std::string & data)
{
	char length = (uint8_t)data.size();
	std::string frame("\x05\x64", 2);
	frame.push_back(length);
	frame.push_back(0x00);
	frame.append(data);
	uint16_t crc = 0;
	for (size_t i = 0; i < frame.size(); ++i)
		crc = crc16_update(crc, frame.at(i));
	crc = crc16_update(crc, 0);
	crc = crc16_update(crc, 0);

	frame.append(1, crc >> 8);
	frame.append(1, crc & 0xFF);
	write(frame);
	return true;
}

void Comm5Serial::queryRelayState()
{
	//                  id    op     mask
	std::string data("\x02\x01\xFF", 3);
	writeFrame(data);
}

void Comm5Serial::querySensorState()
{
	//                  id    mask
	std::string data("\x01\xFF", 2);
	writeFrame(data);
}

void Comm5Serial::enableNotifications()
{
	//                  id    op   on/off  mask
	std::string data("\x04\x02\x01\xFF");
	writeFrame(data);
}

void Comm5Serial::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData, length);
}

void Comm5Serial::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "Comm5 MA-4200: Error: %s", e.what());
}


