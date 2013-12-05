
#include "stdafx.h"
#ifndef WIN32
#error This file must be only compiled for Windows platform
#endif // WIN32
#include "1WireForWindows.h"
#include "../main/Logger.h"
#include <boost/thread.hpp>
#include <boost/optional.hpp>


#define _1WIRE_SERVICE_PORT "1664"


void Log(const _eLogLevel level, const char* logline, ...)
{
#ifdef _DEBUG
   va_list argList;
	char cbuffer[1024];
	va_start(argList, logline);
	vsnprintf(cbuffer, 1024, logline, argList);
	va_end(argList);

   _log.Log(level,cbuffer);
#else // _DEBUG
#endif // _DEBUG
}

SOCKET ConnectToService()
{
   // Connection
   WSADATA wsaData;
   SOCKET theSocket = INVALID_SOCKET;
   struct addrinfo *result = NULL,
      *ptr = NULL,
      hints;

   // Initialize Winsock
   if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0)
      return INVALID_SOCKET;

   ZeroMemory(&hints,sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;

   // Resolve the server address and port
   int getaddrinfoRet = getaddrinfo("localhost",_1WIRE_SERVICE_PORT,&hints,&result);
   if (getaddrinfoRet != 0)
   {
      Log(LOG_ERROR,"getaddrinfo failed with error: %d\n", getaddrinfoRet);
      WSACleanup();
      return INVALID_SOCKET;
   }

   // Attempt to connect to an address until one succeeds
   for(ptr=result;ptr!=NULL;ptr=ptr->ai_next)
   {
      // Create a SOCKET for connecting to server
      theSocket = socket(ptr->ai_family,ptr->ai_socktype,ptr->ai_protocol);
      if (theSocket == INVALID_SOCKET)
      {
         Log(LOG_ERROR,"socket function failed with error = %d\n", WSAGetLastError());
         WSACleanup();
         return INVALID_SOCKET;
      }

      // Connect to server
      if (connect(theSocket,ptr->ai_addr,(int)ptr->ai_addrlen) == SOCKET_ERROR)
      {
         Log(LOG_ERROR,"connect function failed with error: %ld\n", WSAGetLastError());
         closesocket(theSocket);
         theSocket = INVALID_SOCKET;
         continue;
      }
      break;
   }

   freeaddrinfo(result);

   if (theSocket==INVALID_SOCKET)
   {
      WSACleanup();
      return INVALID_SOCKET;
   }

   // Set receive timeout
   struct timeval tv;
   tv.tv_sec = 10*1000;  /* 10 Secs Timeout */
   if (setsockopt(theSocket,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval))==SOCKET_ERROR)
   {
      Log(LOG_ERROR,"setsockopt failed with error: %u\n", WSAGetLastError());
      closesocket(theSocket);
      WSACleanup();
      return INVALID_SOCKET;
   }

   return theSocket;
}

void DisconnectFromService(SOCKET theSocket)
{
   closesocket(theSocket);
   WSACleanup();
}

bool Send(SOCKET theSocket,std::string requestToSend)
{
   // Send message size
   size_t requestSize = requestToSend.length();
   if (send(theSocket,(char*)&requestSize,sizeof(requestSize),0) == SOCKET_ERROR)
      return false;

   // Send message
   if (send(theSocket,requestToSend.c_str(),requestToSend.length(),0) == SOCKET_ERROR)
      return false;

   return true;
}

std::string Receive(SOCKET theSocket)
{
   // Wait for answer (with timeout, see ConnectToService)
   // First receive answer size
   size_t answerSize;
   if (recv(theSocket,(char*)&answerSize,sizeof(answerSize),0)==SOCKET_ERROR)
      return "";

   // Now the answer content
   char* answerBuffer = new char[answerSize];
   if (recv(theSocket,answerBuffer,answerSize,0)==SOCKET_ERROR)
      return "";

   std::string answer(answerBuffer, answerSize);
   delete[] answerBuffer;
   return answer;
}

std::string SendAndReceive(SOCKET theSocket,std::string requestToSend)
{
   if (!Send(theSocket,requestToSend))
      return "";

   return Receive(theSocket);
}

std::string C1WireForWindows::SendAndReceive(std::string requestToSend) const
{
   // SendAndReceive can be called by 2 different thread contexts : writeData and GetDevices
   // So we have to set protection
   boost::lock_guard<boost::mutex> locker(*(const_cast<boost::mutex*>(&m_SocketMutex)));

   return ::SendAndReceive(m_Socket,requestToSend);
}

bool C1WireForWindows::IsAvailable()
{
   static boost::optional<bool> IsAvailable;

   if (IsAvailable.is_initialized())
      return IsAvailable.get();

   // Make a connection only to know if 1-wire is avalaible
   SOCKET theSocket = ConnectToService();
   if (theSocket == INVALID_SOCKET)
   {
      IsAvailable=false;
      return false;
   }

   // Request
   Json::Value reqRoot;
   reqRoot["IsAvailable"]="";
   Json::FastWriter writer;

   // Send request and wait for answer
   std::string answer = ::SendAndReceive(theSocket,writer.write(reqRoot));

   // Answer processing
   if (answer.empty())
   {
      IsAvailable=false;
      DisconnectFromService(theSocket);
      return false;
   }

   Json::Value ansRoot;
   Json::Reader reader;
   if (!reader.parse(answer,ansRoot))
   {
      IsAvailable=false;
      DisconnectFromService(theSocket);
      return false;
   }

   DisconnectFromService(theSocket);

   IsAvailable=ansRoot.get("Available",false).asBool();
   return IsAvailable.get();
}

C1WireForWindows::C1WireForWindows()
{
   // Connect to service
   m_Socket = ConnectToService();
}

C1WireForWindows::~C1WireForWindows()
{
   // Disconnect from service
   if (m_Socket==INVALID_SOCKET)
      return;
      
   DisconnectFromService(m_Socket);
}

void C1WireForWindows::GetDevices(/*out*/std::vector<_t1WireDevice>& devices) const
{
   if (m_Socket==INVALID_SOCKET)
      return;

   // Request
   Json::Value reqRoot;
   reqRoot["GetDevices"]="";
   Json::FastWriter writer;

   // Send request and wait for answer
   std::string answer = SendAndReceive(writer.write(reqRoot));

   // Answer processing
   Json::Value ansRoot;
   Json::Reader reader;
   if (answer.empty() || !reader.parse(answer,ansRoot))
      return;

   if (!ansRoot["InvalidRequest"].isNull())
   {
      Log(LOG_ERROR,"1-wire GetDevices : get an InvalidRequest answer with reason \"%s\"\n",
         ansRoot.get("Reason","unknown reason").asString());
      return;
   }
   
   for ( Json::ArrayIndex itDevice = 0; itDevice<ansRoot["Devices"].size(); itDevice++)
   {
      _t1WireDevice device;
      device.family=(_e1WireFamilyType)ansRoot["Devices"][itDevice]["Family"].asUInt();
      device.devid=ansRoot["Devices"][itDevice]["Id"].asString();
      device.filename="";
      if (!device.devid.empty())
         devices.push_back(device);
   }
}

Json::Value C1WireForWindows::readData(const _t1WireDevice& device,int unit) const
{
   if (m_Socket==INVALID_SOCKET)
      throw C1WireForWindowsReadException("invalid socket");

   // Request
   Json::Value reqRoot;
   reqRoot["ReadData"]["Id"]=device.devid;
   reqRoot["ReadData"]["Unit"]=unit;
   Json::FastWriter writer;

   // Send request and wait for answer
   std::string answer = SendAndReceive(writer.write(reqRoot));

   // Answer processing
   Json::Value ansRoot;
   Json::Reader reader;
   if (answer.empty() || !reader.parse(answer,ansRoot))
      throw C1WireForWindowsReadException("invalid answer");

   if (!ansRoot["InvalidRequestReason"].isNull())
      throw C1WireForWindowsReadException(std::string("1-wire readData : get an InvalidRequest answer with reason ").append(ansRoot.get("InvalidRequestReason","unknown").asString()));

   return ansRoot;
}

unsigned int C1WireForWindows::readChanelsNb(const _t1WireDevice& device) const
{
   if (m_Socket==INVALID_SOCKET)
      throw C1WireForWindowsReadException("invalid socket");

   // Request
   Json::Value reqRoot;
   reqRoot["ReadChanelsNb"]["Id"]=device.devid;
   Json::FastWriter writer;

   // Send request and wait for answer
   std::string answer = SendAndReceive(writer.write(reqRoot));

   // Answer processing
   Json::Value ansRoot;
   Json::Reader reader;
   if (answer.empty() || !reader.parse(answer,ansRoot))
      throw C1WireForWindowsReadException("invalid answer");


   if (!ansRoot["InvalidRequestReason"].isNull())
      throw C1WireForWindowsReadException(std::string("1-wire readData : get an InvalidRequest answer with reason ").append(ansRoot.get("InvalidRequestReason","unknown").asString()));

   return ansRoot.get("ChanelsNb",0).asUInt();
}

float C1WireForWindows::GetTemperature(const _t1WireDevice& device) const
{
   Json::Value ansRoot;
   try
   {
      ansRoot=readData(device);
   }
   catch (C1WireForWindowsReadException&)
   {
      return 0.0;
   }
   return ansRoot.get("Temperature",0.0f).asFloat();
}

float C1WireForWindows::GetHumidity(const _t1WireDevice& device) const
{
   Json::Value ansRoot;
   try
   {
      ansRoot=readData(device);
   }
   catch (C1WireForWindowsReadException&)
   {
      return 0.0;
   }
   return ansRoot.get("Humidity",0.0f).asFloat();
}

bool C1WireForWindows::GetLightState(const _t1WireDevice& device,int unit) const
{
   Json::Value ansRoot;
   try
   {
      ansRoot=readData(device,unit);
   }
   catch (C1WireForWindowsReadException&)
   {
      return false;
   }

   return !ansRoot.get("DigitalIo",false).asBool();
}

unsigned int C1WireForWindows::GetNbChannels(const _t1WireDevice& device) const
{
   try
   {
      return readChanelsNb(device);
   }
   catch (C1WireForWindowsReadException&)
   {
      return false;
   }
}

unsigned long C1WireForWindows::GetCounter(const _t1WireDevice& device,int unit) const
{
   Json::Value ansRoot;
   try
   {
      ansRoot=readData(device,unit);
   }
   catch (C1WireForWindowsReadException&)
   {
      return false;
   }

   return ansRoot.get("Counter",0).asUInt();
}

int C1WireForWindows::GetVoltage(const _t1WireDevice& device,int unit) const
{
   Json::Value ansRoot;
   try
   {
      ansRoot=readData(device,unit);
   }
   catch (C1WireForWindowsReadException&)
   {
      return false;
   }

   return ansRoot.get("Voltage",0).asInt();
}

void C1WireForWindows::SetLightState(const std::string& sId,int unit,bool value)
{
   if (m_Socket==INVALID_SOCKET)
      return;

   // Request
   Json::Value reqRoot;
   reqRoot["WriteData"]["Id"]=sId;
   reqRoot["WriteData"]["Unit"]=unit;
   reqRoot["WriteData"]["Value"]=!value;
   Json::FastWriter writer;

   // Send request and wait for answer
   SendAndReceive(writer.write(reqRoot));

   // No answer processing
}

void C1WireForWindows::GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const
{
   // 1W-Kernel device name format : ff-iiiiiiiiiiii, with :
   // - ff : family (1 byte)
   // - iiiiiiiiiiii : id (6 bytes)

   // Retrieve family from the first 2 chars
   device.family=ToFamily(deviceName.substr(0,2));

   // Device Id (6 chars after '-')
   device.devid=deviceName.substr(3,3+6*2);

   // Filename (full path)
   device.filename=deviceName;
}

