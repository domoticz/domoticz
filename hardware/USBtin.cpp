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
using namespace std;

USBtin::USBtin(const int ID, const std::string& devname,unsigned int BusCanType,unsigned int DebugMode) :
m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_stoprequested=false;
	m_retrycntr=USBTIN_RETRY_DELAY*5;
	BusCANType = BusCanType;
	if( DebugMode == 0 )BOOL_Debug = false;
	else BOOL_Debug = true;
	Init();
}

USBtin::~USBtin(void)
{
	
}

void USBtin::Init()
{
	m_bufferpos = 0;
}

bool USBtin::StartHardware()
{
	// StartHeartbeatThread();
	m_stoprequested = false;
	BelErrorCount = 0;
	m_retrycntr=USBTIN_RETRY_DELAY*5; //will force reconnect first thing
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&USBtin::Do_Work, this)));
	return (m_thread!=NULL);
}

bool USBtin::StopHardware() //appelé lorsque le matériel est supprimé
{
	//std::string data("\x43\x0D"); //-> fermeture du portCan
	//writeFrame(data);
	m_stoprequested = true; //déclenche arrêt du while dans Do_Work
	// StopHeartbeatThread();
	if (m_thread)
	{
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
		//usleep(TIME_200ms);
	}
	terminate();
	m_bIsStarted = false;
	_log.Log(LOG_STATUS, "USBtin: Can Gateway stopped, goodbye !");
	return true;
}

void USBtin::Restart()
{
	StopHardware();
	usleep(TIME_1sec);
	StartHardware();
}

void USBtin::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;
	EtapeInitCan = 0;
	
	while (!m_stoprequested) 
	{
		usleep(TIME_100ms);
		
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
						BelErrorCount = 0;
						//réinitialise les Layers actif avant l'ouverture du CAN:
						// + kill thread associé
						
						
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
						
						_log.Log(LOG_STATUS, "USBtin: BusCantType value: %d ",BusCANType);
						if( (BusCANType&Multibloc_V8) == Multibloc_V8 ) _log.Log(LOG_STATUS, "USBtin: MultiblocV8 is Selected !");
						if( (BusCANType&FreeCan) == FreeCan )  _log.Log(LOG_STATUS, "USBtin: FreeCAN is Selected !");
						
						if( BusCANType == 0 ) _log.Log(LOG_STATUS, "USBtin: No Can management Selected !");
						
						EtapeInitCan++;
						break;
					case 5 : //openning can port :
						if( (BusCANType&Multibloc_V8) == Multibloc_V8 ){ ManageThreadV8(true);}
						
						OpenCanPort();
						//Active le layer can
						// + activation thread associé
						EtapeInitCan++;
						break;
					case 6 ://All is good !
						/*if( Reponse == 0x07 ){ //Si erreur reçu arrivé ici :
							Reponse = 0;
							//_log.Log(LOG_ERROR, "USBtin: Error openning Can port / retrying 1 time in 1sec...");
							//
							CloseCanPort();
							usleep(TIME_1sec);
							EtapeInitCan = 4;
						}*/
						/*else{ //all is good :-)
							EtapeInitCan++;
						}*/
						break;
						
					
				}
			}
		}
			
		//if( BusCANType == Multibloc_V8 ){ Do_Work_MultiblocV8(); }
			
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
				m_buffer[m_bufferpos] = 0;
				OpenSerialDevice();
			}
		}		
	}
	
	//ManageThreadV8(false);
	CloseCanPort();
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
	m_buffer[m_bufferpos] = 0;
	setReadCallback(boost::bind(&USBtin::readCallback, this, _1, _2));	
	
	sOnConnected(this);
	
	return true;
}

void USBtin::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bEnableReceive)
		return; //receiving not enabled
	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void USBtin::ParseData(const unsigned char *pData, int Len)
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
		const unsigned char c = pData[ii];
		
		if( m_bufferpos == sizeof(m_buffer) - 1 ){
			/*for(j=0;j<sizeof(m_buffer);j++){
				m_buffer[j] = 0;
			}*/
			m_bufferpos = 0;
			//m_buffer[m_bufferpos] = 0;
			_log.Log(LOG_ERROR,"USBtin: Warning Error buffer size reaches/ maybe Can is overrun...");
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			// reception "BEL" = erreur dernière commande/commande déjà active
			if( USBTIN_BELSIGNAL == c ){
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
			else if( USBTIN_CR == c)
			{
				BelErrorCount = 0;
				// discard newline, close string, parse line and clear it.
				//_log.Log(LOG_NORM,"CR receive - traitement !");
				//if (m_bufferpos > 0)
					//m_buffer[m_bufferpos] = 0;
				//_log.Log(LOG_STATUS,"USBtin: CR réponse à traiter: #%s#", m_buffer);
				//MatchLine();

				if( m_buffer[0] == USBTIN_HARDWARE_VERSION ){
					//Reponse = 0x0D;
					strncpy(value, (char*)&(m_buffer[1]), 4);
					_log.Log(LOG_STATUS,"USBtin: Hardware Version: %s", value);
					
				}
				else if( m_buffer[0] == USBTIN_FIRMWARE_VERSION ){
					//Reponse = 0x0D;
					strncpy(value, (char*)&(m_buffer[1]), 4);
					_log.Log(LOG_STATUS,"USBtin: Firware Version: %s", value);
				}
				else if( m_buffer[0] == USBTIN_SERIAL_NUMBER ){
					//Reponse = 0x0D;
					strncpy(value, (char*)&(m_buffer[1]), 4);
					_log.Log(LOG_STATUS,"USBtin: Serial Number: %s", value);
				}
				else if( m_buffer[0] == 0x0D ){
					//Reponse = 0x0D;
					_log.Log(LOG_STATUS,"USBtin: return OK :-)");
				}
				else if( m_buffer[0] == USBTIN_EXT_TRAME_RECEIVE ){ // Receive Extended Frame :
					//_log.Log(LOG_NORM,"USBtin: Traitement trame etendue : #%s#",m_buffer);
					//string str = "";
					strncpy(value, (char*)&(m_buffer[1]), 8); //prend la partie "ID étendue" et la colle dans le tablea de char value
					int IDhexNumber;
					sscanf(value, "%x", &IDhexNumber); //IDhexNumber contient la valeur numérique de l'identifiant
					
					memset(&value[0], 0, sizeof(value));
					
					strncpy(value, (char*)&(m_buffer[9]), 1); //read the DLC
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
							
							strncpy(value, (char*)&(m_buffer[10+(2*i)]), 2); //prend les octets (char) par paquet de 2 caractères pour recomposition
							sscanf(value, "%x", &ValData);
							
							Buffer_Octets[i]=ValData;
							memset(&value[0], 0, sizeof(value));
						}
					}
					
					if( (BusCANType&Multibloc_V8) > 0 ){ //Traitement trame multibloc V8
						Traitement_MultiblocV8(IDhexNumber,DLChexNumber,Buffer_Octets,BOOL_Debug);
						if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Traitement trame multiblocV8 : #%s#",m_buffer);
					}
					/*else{ //autres...
						if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Traitement trame autre : #%s#",m_buffer);
					}*/
					
				}
				else if( m_buffer[0] == USBTIN_NOR_TRAME_RECEIVE ){ // Receive Normale Frame :
					//_log.Log(LOG_NORM,"USBtin: Traitement trame normale : #%s#",m_buffer);
				}
				else if( m_buffer[0] == USBTIN_GOODSENDING_NOR_TRAME || m_buffer[0] == USBTIN_GOODSENDING_EXT_TRAME ){
					//envoi d'une trame normale/étendu = OK !
					if( BOOL_Debug == true) _log.Log(LOG_NORM,"USBtin: Frame Send OK !");
				}
				else{ //over things here...
					if( BOOL_Debug == true) _log.Log(LOG_ERROR,"USBtin: receive command not supported : #%s#", m_buffer);
					
				}
				//remis à 0 du pointeur de buffer
				m_bufferpos = 0;
			}
			else{
				m_bufferpos++;
			}
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