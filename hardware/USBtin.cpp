/*
File : USBtin.cpp
Author : X.PONCET
Version : 1.00
Description : This class manage the USBtin gateway

History :
- 2017-01-01 : Creation
*/
#include "stdafx.h"
#include "USBtin.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#include <time.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <boost/bind.hpp>
#include <ctime>

#define USBTIN_BAUD_RATE         115200
#define USBTIN_PARITY            boost::asio::serial_port_base::parity::none
#define USBTIN_CARACTER_SIZE      8
#define USBTIN_FLOW_CONTROL      boost::asio::serial_port_base::flow_control::none
#define USBTIN_STOP_BITS         boost::asio::serial_port_base::stop_bits::one

#define	TIME_3sec				3000000
#define	TIME_1sec				1000000
#define	TIME_500ms				500000
#define	TIME_200ms				200000
#define	TIME_100ms				100000
#define	TIME_10ms				10000
#define	TIME_5ms				5000

#define	USBTIN_CR							0x0D
#define	USBTIN_BELSIGNAL					0x07
#define	USBTIN_FIRMWARE_VERSION				0x76
#define	USBTIN_HARDWARE_VERSION				0x56
#define	USBTIN_SERIAL_NUMBER				0x4E
#define	USBTIN_EXT_TRAME_RECEIVE			0x54
#define	USBTIN_NOR_TRAME_RECEIVE			0x74
#define	USBTIN_EXT_RTR_TRAME_RECEIVE		0x52
#define	USBTIN_NOR_RTR_TRAME_RECEIVE		0x72
#define USBTIN_GOODSENDING_NOR_TRAME		0x5A
#define USBTIN_GOODSENDING_EXT_TRAME		0x7A

#define	USBTIN_FW_VERSION	"v"
#define	USBTIN_HW_VERSION	"V"
#define USBTIN_RETRY_DELAY  30

#define NoCanDefine		0x00
#define	Multibloc_V8	0x01
#define FreeCan			0x02

using namespace std;

USBtin::USBtin(const int ID, const std::string& devname,unsigned int BusCanType,unsigned int DebugMode) :
m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_stoprequested=false;
	m_retrycntr=USBTIN_RETRY_DELAY*5;
	Bus_CANType = BusCanType;
	if( DebugMode == 0 )BOOL_Debug = false;
	else BOOL_Debug = true;
	Init();
}

USBtin::~USBtin(void){
	StopHardware();
}

void USBtin::Init()
{
	m_bufferpos = 0;
}

bool USBtin::StartHardware()
{
	m_stoprequested = false;
	BelErrorCount = 0;
	m_retrycntr=USBTIN_RETRY_DELAY*5; //will force reconnect first thing
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&USBtin::Do_Work, this)));
	return (m_thread!=NULL);
}

void USBtin::Restart()
{
	StopHardware();
	usleep(TIME_3sec);
	StartHardware();
}

bool USBtin::StopHardware() //appelé lorsque le matériel est supprimé
{
	//ManageThreadV8(false); //arret des layers par sécu
	m_stoprequested = true; //déclenche arrêt du while dans Do_Work
	if (m_thread) m_thread->join();
	sleep_milliseconds(10);
	terminate();
	m_bIsStarted = false;
	return true;
}

void USBtin::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;
	EtapeInitCan = 0;
	
	while (!m_stoprequested) 
	{
		usleep(TIME_200ms);
		
		if (m_stoprequested){
			EtapeInitCan = 0;
			break;
		}
		
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;
			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			
			if (isOpen()) //Serial port open, we can initiate the Can BUS :
			{
				switch(EtapeInitCan){
					case 0 :
						_log.Log(LOG_STATUS, "USBtin: Serial port is now open !");
						CloseCanPort(); //more cleaner to close in first
						memset(&m_bufferUSBtin,0,sizeof(m_bufferUSBtin));
						BelErrorCount = 0;
						EtapeInitCan++;
						break;
					case 1 :
						GetHWVersion();
						EtapeInitCan++;
						break;
					case 2 :
						GetFWVersion();
						EtapeInitCan++;
						break;
					case 3 :
						GetSerialNumber();
						EtapeInitCan++;
						break;
					case 4 :
						//Reponse = 0;
						SetBaudRate250Kbd();
						//_log.Log(LOG_STATUS, "USBtin: BusCantType value: %d ",Bus_CANType);
						if( (Bus_CANType&Multibloc_V8) == Multibloc_V8 ) _log.Log(LOG_STATUS, "USBtin: MultiblocV8 is Selected !");
						if( (Bus_CANType&FreeCan) == FreeCan )  _log.Log(LOG_STATUS, "USBtin: FreeCAN is Selected !");
						
						if( Bus_CANType == 0 ) _log.Log(LOG_ERROR, "USBtin: WARNING: No Can management Selected !");
						EtapeInitCan++;
						break;
					case 5 : //openning can port :
						//Active le layer can
						// + activation thread associé
						if( (Bus_CANType&Multibloc_V8) == Multibloc_V8 ){ ManageThreadV8(true);}
						OpenCanPort();
						EtapeInitCan++;
						break;
					
					case 6 ://All is good !
						//here nothing to do, the CAN is ok....
						break;
						
					
				}
			}
		}
			
		if (!isOpen()) //serial not open
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"USBtin: serial retrying in %d seconds...", USBTIN_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr/5>=USBTIN_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_bufferpos = 0;
				m_bufferUSBtin[m_bufferpos] = 0;
				OpenSerialDevice();
			}
		}		
	}
	
	CloseCanPort(); //for security
	_log.Log(LOG_STATUS, "USBtin: Can Gateway stopped, goodbye !");
}

bool USBtin::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "USBtin: Using serial port: %s", m_szSerialPort.c_str());
		open(m_szSerialPort,115200);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "USBtin: Error opening serial port!");
		#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
		#else
		(void)e;
		#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "USBtin: Error opening serial port!!!");
		return false;
	}
	
	m_bIsStarted = true;
	m_bufferpos = 0;
	memset(&m_bufferUSBtin,0,sizeof(m_bufferUSBtin));
	setReadCallback(boost::bind(&USBtin::readCallback, this, _1, _2));	
	
	sOnConnected(this);
	
	return true;
}

void USBtin::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bEnableReceive)
		return; //receiving not enabled
	if (len > sizeof(m_bufferUSBtin)){
		_log.Log(LOG_ERROR,"USBtin: Warning Error buffer size reaches/ maybe Can is overrun...");
		return;
	}
	ParseData(data, static_cast<int>(len));
}

void USBtin::ParseData(const char *pData, int Len)
{
	char value[30] = "";
	std::string vString;
	unsigned long ulValue;
	
	unsigned long IdValue;
	unsigned int DlcValue;
	unsigned int Data[8];
	
	int i,j;
	int ii = 0;
	while (ii<Len)
	{
		m_bufferUSBtin[m_bufferpos] = pData[ii];
		// reception "BEL" = erreur dernière commande/commande déjà active
		if( USBTIN_BELSIGNAL == m_bufferUSBtin[m_bufferpos] ){
			//RAZ 1er char du buffer
			BelErrorCount++;
			if( BelErrorCount > 3 ){ //If more than 3 BEL receive : restart the Gateway !
				_log.Log(LOG_ERROR,"USBtin: 3x times BEL signal receive : restart gateway ");
				Restart();
			}
			else{
				_log.Log(LOG_ERROR,"USBtin: BEL signal (commande allready active or Gateway error) ! ");
			}
			m_bufferpos = 0;
		}
		//Si CR reçu on doit traiter la trame:
		else if( USBTIN_CR == m_bufferUSBtin[m_bufferpos] )
		{
			BelErrorCount = 0;
			if( m_bufferUSBtin[0] == USBTIN_HARDWARE_VERSION ){
				strncpy(value, (char*)&(m_bufferUSBtin[1]), 4);
				_log.Log(LOG_STATUS,"USBtin: Hardware Version: %s", value);
				
			}
			else if( m_bufferUSBtin[0] == USBTIN_FIRMWARE_VERSION ){
				strncpy(value, (char*)&(m_bufferUSBtin[1]), 4);
				_log.Log(LOG_STATUS,"USBtin: Firware Version: %s", value);
			}
			else if( m_bufferUSBtin[0] == USBTIN_SERIAL_NUMBER ){
				strncpy(value, (char*)&(m_bufferUSBtin[1]), 4);
				_log.Log(LOG_STATUS,"USBtin: Serial Number: %s", value);
			}
			else if( m_bufferUSBtin[0] == USBTIN_CR ){
				_log.Log(LOG_STATUS,"USBtin: return OK :-)");
			}
			else if( m_bufferUSBtin[0] == USBTIN_EXT_TRAME_RECEIVE ){ // Receive Extended Frame :
				strncpy(value, (char*)&(m_bufferUSBtin[1]), 8); //prend la partie "ID étendue" et la colle dans le tablea de char value
				int IDhexNumber;
				sscanf(value, "%x", &IDhexNumber); //IDhexNumber contient la valeur numérique de l'identifiant
				
				memset(&value[0], 0, sizeof(value));
				
				strncpy(value, (char*)&(m_bufferUSBtin[9]), 1); //read the DLC
				int DLChexNumber;
				sscanf(value, "%x", &DLChexNumber);
				
				memset(&value[0], 0, sizeof(value));
									
				unsigned int Buffer_Octets[8]; //buffer of 8 bytes(max)
				char i=0;
				for(i=0;i<8;i++){ //Reset of 8 bytes
					Buffer_Octets[i]=0;
				}
				unsigned int ValData;
				
				if( DLChexNumber > 0 ){ //bytes presents
					for(i=0;i<=DLChexNumber;i++){
						ValData = 0;
						
						strncpy(value, (char*)&(m_bufferUSBtin[10+(2*i)]), 2); //prend les octets (char) par paquet de 2 caractères pour recomposition
						sscanf(value, "%x", &ValData);
						
						Buffer_Octets[i]=ValData;
						memset(&value[0], 0, sizeof(value));
					}
				}
				
				if( (Bus_CANType&Multibloc_V8) == Multibloc_V8 ){ //Traitement trame multibloc V8
					Traitement_MultiblocV8(IDhexNumber,DLChexNumber,Buffer_Octets);
					if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Traitement trame multiblocV8 : #%s#",m_bufferUSBtin);
				}
				
				if( Bus_CANType == 0 ){
					if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Frame receive not managed: #%s#",m_bufferUSBtin);
				}
				
			}
			else if( m_bufferUSBtin[0] == USBTIN_NOR_TRAME_RECEIVE ){ // Receive Normale Frame :
				//_log.Log(LOG_NORM,"USBtin: Traitement trame normale : #%s#",m_buffer);
			}
			else if( m_bufferUSBtin[0] == USBTIN_GOODSENDING_NOR_TRAME || m_bufferUSBtin[0] == USBTIN_GOODSENDING_EXT_TRAME ){
				//envoi d'une trame normale/étendu = OK !
				if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Frame Send OK !");
			}
			else{ //over things here...
				if( BOOL_Debug == true) _log.Log(LOG_ERROR,"USBtin: receive command not supported : #%s#", m_bufferUSBtin);
				
			}
			//remis à 0 du pointeur de buffer
			m_bufferpos = 0;
		}
		else{
			m_bufferpos++;
		}
	
		ii++;
	}
}

bool USBtin::writeFrame(const std::string & data)
{
	char length = (uint8_t)data.size();
	if (!isOpen()){
		return false;
	}
	if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: write frame to Can Gateway: #%s# ",data.c_str());
	std::string frame;
	frame.append(data);
	frame.append("\x0D"); //ajout "carry return"
	write(frame);
	return true;
}

void USBtin::GetHWVersion()
{
	//Reponse = 0;
	//while(Reponse==0){
		std::string data("V");
		writeFrame(data);
		usleep(TIME_200ms);
	//}
}
void USBtin::GetSerialNumber()
{
	//Reponse = 0;
	//while(Reponse==0){
		std::string data("N");
		writeFrame(data);
		usleep(TIME_200ms);
	//}
}
void USBtin::GetFWVersion()
{
	//Reponse = 0;
	//while(Reponse==0){
		std::string data("v");
		writeFrame(data);
		//usleep(TIME_200ms);
	//}
}
void USBtin::SetBaudRate250Kbd()
{
	//Reponse = 0;
	//while(Reponse==0){
		_log.Log(LOG_STATUS,"USBtin: Setting Can speed to 250Kb/s");
		std::string data("S5");
		writeFrame(data);
		//usleep(TIME_200ms);
	//}
}
void USBtin::OpenCanPort()
{
	//Reponse = 0;
	//while(Reponse==0){
		_log.Log(LOG_STATUS, "USBtin: Openning Canport...");
		std::string data("O");
		writeFrame(data);
		//usleep(TIME_200ms);
	//}
}
void USBtin::CloseCanPort()
{
	//Reponse = 0;
	//while(Reponse==0){
		_log.Log(LOG_STATUS, "USBtin: Closing Canport...");
		std::string data("C");
		writeFrame(data);
		//usleep(TIME_200ms);
	//}
}