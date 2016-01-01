#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

class TiXmlElement;

class RemehaSerial: public AsyncSerial, public CDomoticzHardwareBase
{
public:
	RemehaSerial(const int ID, const std::string& devname, const unsigned int baud_rate);
	~RemehaSerial();
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool WriteToHardware(const char *pdata, const unsigned char length);
	std::string m_Version;

protected:
	typedef struct
	{
		BYTE start;
		BYTE source_addr;
		BYTE dest_addr;
		BYTE unk_protocol_id;
		BYTE size;
		BYTE unk_unit_id;
		BYTE unk_function;
		BYTE DATA[64];
		BYTE CRC[2];
		BYTE end;
	} RemehaDataSample;
	void SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	void ParseData(const unsigned char *pData, int Len);
	void ParseSample(RemehaDataSample* data);
	bool WriteInt(const unsigned char *pData, const unsigned char Len);
	void RequestActualValues();
	unsigned char m_buffer[1028];
	size_t m_bufferpos;

private:
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();

	void StartPollerThread();
	void StopPollerThread();
	void Do_PollWork();

	typedef int TXTID;
	typedef std::map<int, TXTID> tIntTXTMap;

	class Sample {
	public:
		// Constructor
		Sample();

		// config
		std::string type;
		std::string label;
		int group;
		// present
		TXTID name;
		TXTID description;
		TXTID unit;
		// function
		int byte;
		std::string expression;
		std::string format;
		// function
		int bit;
		bool invert;
		// node-ref
		std::string node_type;
		std::string node_id;
		// limits
		int min;
		TXTID txt_min;
		int max;
		TXTID txt_max;
		tIntTXTMap selection;
	};

	unsigned short crc16_update(unsigned short crc, unsigned char a);
	unsigned short Crc16(const unsigned char* byteArray,size_t arraySize);

	void ReadLanguageFile(std::string lang);
	void ReadConfigFile(std::string sFileName);

	void XMLParseSelection(TiXmlElement * pSelections, tIntTXTMap& SelectionMap);

	int m_retrycntr;
	boost::shared_ptr<boost::thread> m_pollerthread;
	volatile bool m_stoprequestedpoller;

	std::map<TXTID, std::string> m_languagestrings;
	std::map<std::string, tIntTXTMap> m_SelectionsMap;
	std::list<Sample> m_SampleInfo;
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

