/************************************************************************
 *
 PiFace Interface board driver for Domoticz
 Date: 01-10-2013
 Written by: Robin Stevenhagen
 License: Public domain

 Date: 18-06-2015
 GizMoCuz: Added messages to a send queue

 Date: 08-04-2016
 BobKersten: Added multiple counter types

 SPI code is based on code from:
 The WiringPi project
 *    Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * See    https://projects.drogon.net/raspberry-pi/wiringpi/
 ***********************************************************************

 Code uses SPI dev 0.0, as this is the interface were the PiFace resides.
 PiFace has 8 Input (4 buttons) and 8 Output pins, of which output 0 and 1 are relay outputs.
 Driver autodetects all available PiFace boards on startup, and scans them.

 Default config (without config):
 Input 0..3 enabled with rising edge detection
 Input 4..7 enabled for pulse counting
 Output 0..7 enbled with level feedback reporting.

 ************************************************************************/

#include "stdafx.h"
#include "PiFace.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <string>
#ifdef HAVE_LINUX_SPI
    #include <linux/ioctl.h>
    #include <linux/types.h>
    #include <linux/spi/spidev.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "MCP23x17.h"
#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include "../main/localtime_r.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"

#define PIFACE_INPUT_PROCESS_INTERVAL           5                // 100 msec
#define PIFACE_COUNTER_COUNTER_INTERVAL         1
#define PIFACE_WORKER_THREAD_SLEEP_INTERVAL_MS  20

#define DISABLE_NEW_FUNCTIONS

extern std::string szUserDataFolder;

const std::string CPiFace::ParameterNames[CONFIG_NR_OF_PARAMETER_TYPES]                       = {"enable","enabled","pin_type","count_enable","count_enabled","count_update_interval_sec","count_update_interval_s","count_update_interval","count_update_interval_diff_perc","count_initial_value","count_minimum_pulse_period_msec","count_type","count_divider"};
const std::string CPiFace::ParameterBooleanValueNames[CONFIG_NR_OF_PARAMETER_BOOL_TYPES]      = {"false","0","true","1"};
const std::string CPiFace::ParameterPinTypeValueNames[CONFIG_NR_OF_PARAMETER_PIN_TYPES]       = {"level","inv_level","rising","falling"};
const std::string CPiFace::ParameterCountTypeValueNames[CONFIG_NR_OF_PARAMETER_COUNT_TYPES]   = {"generic","rfxmeter","energy"};

CPiFace::CPiFace(const int ID)
{
    m_HwdID=ID;
    m_bIsStarted=false;

    for (int i = 0; i < 4; i++)
    {
        m_Inputs[i].Callback_pntr = (void*)this; // Set callback pointer, to enable events from IO class
        m_Inputs[i].SetID(i);
        m_Outputs[i].Callback_pntr = (void*)this; // Set callback pointer, to enable events from IO class
        m_Outputs[i].SetID(i);
    }
    m_fd = 0;
}

/***** config file stuff *****/

// trim from start
std::string & CPiFace::ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string & CPiFace::rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string & CPiFace::trim(std::string &s)
{
    return ltrim(rtrim(s));
}

//strip all the unwanted data from the string before its passed back for further processing
std::string & CPiFace::preprocess(std::string &s)
{
    std::string temp;
    std::string tempstripped;

    temp.resize(s.size());
    std::transform(s.begin(),s.end(),temp.begin(),::tolower);

    tempstripped = trim(temp);

    s=tempstripped;
    return s;
}

int CPiFace::LocateValueInParameterArray(const std::string &Parametername, const std::string *ParameterArray, int Items)
{
    int NameFound=-1; //assume not found
    int Parameter_Index;

    for (Parameter_Index=0;(Parameter_Index < Items) && (NameFound==-1);Parameter_Index++)
    {
	    if (Parametername == ParameterArray[Parameter_Index])
	    {
		    NameFound = Parameter_Index;
	    }
    }
    return (NameFound);
}

int CPiFace::GetParameterString(const std::string &TargetString, const char *SearchStr, int StartPos, std::string &Parameter)
{
    int EndPos=-1;
    std::string Substring;
    std::string SearchString(SearchStr);

    EndPos=TargetString.find(SearchString,StartPos);
    if (EndPos >= 0)
    {
        Substring=TargetString.substr(StartPos,EndPos-StartPos);
        Parameter=preprocess(Substring);
        EndPos=EndPos+SearchString.length(); //set endposition to new search start position
    }
    return EndPos;
}

int CPiFace::LoadConfig()
{
    int result=-1;

    int StartPos=-1;
    int EndPos=-1;

    std::string input;

    std::string Line;
    std::string Parameter;
    std::string Porttype;
    std::string Pinnumber;
    std::string Parametername;
    std::string Parametervalue;

    unsigned char Address=0;
    unsigned char PinNumber=0;
    unsigned char PortType=0;
    int NameFound=0;
    int ValueFound=0;
    CIOPort *IOport = nullptr;
    bool Regenerate_Config=false;

    std::string configfile=szUserDataFolder + "piface.conf";

    std::fstream ConfigFile(configfile.c_str(), std::ios::in);

    if (!ConfigFile.is_open())
    {
        //try to create one for the default board 0
        ConfigFile.close();
        _log.Log(LOG_ERROR,"PiFace: Error config file: %s not found", configfile.c_str() );
        _log.Log(LOG_ERROR,"PiFace: Auto creating for board 0");
        LoadDefaultConfig();
        AutoCreate_piface_config();
        std::fstream ConfigFile(configfile.c_str(), std::ios::in);
    }


    if (ConfigFile.is_open())
    {
        while (ConfigFile.good()==true)
        {
            do
            {
                StartPos=-1;
                EndPos=-1;
                getline(ConfigFile,input,'\n');    //get line from config file
                Line=preprocess(input);

                //find any comments
                StartPos=Line.find("//",0);
                if (StartPos>=0)
                {
                    //comment found, remove it
                    Line=Line.substr(0,StartPos);
                    StartPos=-1; // reset marker
                }

                //find linesync
                EndPos=GetParameterString(Line,".",0,Parameter);
                if (EndPos>=0)
                {
			if (Parameter == "piface")
			{
				StartPos = EndPos;
			}
		}
	    } while ((StartPos < 0) && (ConfigFile.good() == true));

	    if (StartPos > 0)
	    {
		    // first lets get PiFace Address
		    EndPos = GetParameterString(Line, ".", StartPos, Parameter);
		    if (EndPos >= 0)
		    {
			    Address = (unsigned char)(strtol(Parameter.c_str(), nullptr, 0) & 0xFF); // data can be restricted further but as we check later on keep it wide for
												     // future use.
			    StartPos = EndPos;
		    }

		    // Now lets get Port Type
		    EndPos = GetParameterString(Line, ".", StartPos, Parameter);
		    if (EndPos >= 0)
		    {
			    if (Parameter == "input")
			    {
				    // we have an input
				    PortType = 'I';
			    }
			    else if (Parameter == "output")
			    {
				    // we have an output
				    PortType = 'O';
			    }

			    StartPos = EndPos;
		    }

		    // Now lets get Pinnumber
		    EndPos = GetParameterString(Line, ".", StartPos, Parameter);
		    if (EndPos >= 0)
		    {
			    PinNumber = (unsigned char)(strtol(Parameter.c_str(), nullptr, 0) & 0xFF); // data can be restricted further but as we check later on keep it wide for
												       // future use.
			    StartPos = EndPos;
		    }

		    // Now lets get parameter name
		    EndPos = GetParameterString(Line, "=", StartPos, Parameter);
		    if (EndPos >= 0)
		    {
			    Parametername = Parameter;
			    StartPos = EndPos;
		    }

		    // finaly lets get parameter value
		    Parametervalue = Line.substr(StartPos, Line.length() - StartPos);
		    Parametervalue = preprocess(Parametervalue);

		    if ((Address <= 3) && (PinNumber <= 7) && ((PortType == 'I') || (PortType == 'O')) && (Parametername.length() > 0) && (Parametervalue.length() > 0))
		    {
			    _log.Log(LOG_STATUS, "PiFace: config file: Valid address: %d , Pin: %d and Port %c Parameter: %s , Value %s", Address, PinNumber, PortType, Parametername.c_str(),
				     Parametervalue.c_str());
			    NameFound = LocateValueInParameterArray(Parametername, ParameterNames, CONFIG_NR_OF_PARAMETER_TYPES);

			    if (PortType == 'I')
			    {
				    IOport = &m_Inputs[Address];
				    m_Inputs[Address].Pin[PinNumber].Direction = 'I';
			    }
			    else
			    {
				    IOport = &m_Outputs[Address];
				    m_Outputs[Address].Pin[PinNumber].Direction = 'O';
			    }

			    result = 0;

			    switch (NameFound)
			    {
				    default:
					    _log.Log(LOG_ERROR, "PiFace: Error config file: unknown parameter %s found", Parametername.c_str());
					    break;
				    case 0:
				    case 1:
					    // found enable(d)
					    ValueFound = LocateValueInParameterArray(Parametervalue, ParameterBooleanValueNames, CONFIG_NR_OF_PARAMETER_BOOL_TYPES);
					    if (ValueFound >= 0)
					    {
						    if ((ValueFound == 0) || (ValueFound == 1))
						    {
							    IOport->Pin[PinNumber].Enabled = false;
						    }
						    else
						    {
							    IOport->Pin[PinNumber].Enabled = true;
						    }
						    result++;
					    }
					    else
						    _log.Log(LOG_ERROR, "PiFace: Error config file: unknown value %s found", Parametervalue.c_str());
					    break;

				    case 2:
					    // found pintype
					    ValueFound = LocateValueInParameterArray(Parametervalue, ParameterPinTypeValueNames, CONFIG_NR_OF_PARAMETER_PIN_TYPES);
					    switch (ValueFound)
					    {
						    default:
							    _log.Log(LOG_ERROR,
								     "PiFace: Error config file: unknown value %s found =>setting default "
								     "level %d",
								     Parametervalue.c_str(), ValueFound);
							    IOport->Pin[PinNumber].Type = LEVEL;
							    break;

						    case 0:
							    IOport->Pin[PinNumber].Type = LEVEL;
							    break;
						    case 1:
							    IOport->Pin[PinNumber].Type = INV_LEVEL;
							    break;
						    case 2:
							    IOport->Pin[PinNumber].Type = TOGGLE_RISING;
							    break;
						    case 3:
							    IOport->Pin[PinNumber].Type = TOGGLE_FALLING;
							    break;
					    }
					    result++;
					    break;

				    case 3:
				    case 4:
					    // found count_enable(d)
					    ValueFound = LocateValueInParameterArray(Parametervalue, ParameterBooleanValueNames, CONFIG_NR_OF_PARAMETER_BOOL_TYPES);

					    if (ValueFound >= 0)
					    {
						    if ((ValueFound == 0) || (ValueFound == 1))
						    {
							    IOport->ConfigureCounter(PinNumber, false);
						    }
						    else
						    {
							    IOport->ConfigureCounter(PinNumber, true);
						    }
						    result++;
					    }
					    else
						    _log.Log(LOG_ERROR, "PiFace: Error config file: unknown value %s found", Parametervalue.c_str());
					    break;

				    case 5:
				    case 6:
				    case 7:
					    // count_update_interval(_s)(ec)
					    unsigned long UpdateInterval;

					    UpdateInterval = strtol(Parametervalue.c_str(), nullptr, 0);
					    IOport->Pin[PinNumber].Count.SetUpdateInterval(UpdateInterval * 1000);
					    result++;
					    break;

				    case 8:
					    // count_update_interval_diff_perc
					    unsigned long UpdateIntervalPerc;

					    UpdateIntervalPerc = strtol(Parametervalue.c_str(), nullptr, 0);
					    if (UpdateIntervalPerc < 1 || UpdateIntervalPerc > 1000)
					    {
						    _log.Log(LOG_ERROR, "PiFace: Error config file: invalid value %s found", Parametervalue.c_str());
						    break;
					    }
					    IOport->Pin[PinNumber].Count.SetUpdateIntervalPerc(UpdateIntervalPerc);
					    result++;

#ifndef DISABLE_NEW_FUNCTIONS
                            /*  disabled until code part is completed and tested */
                        case 9:
                            //count_initial_value
                            unsigned long StartValue;

			    StartValue = strtol(Parametervalue.c_str(), nullptr, 0);
			    IOport->Pin[PinNumber].Count.SetTotal(StartValue);
			    result++;
			    Regenerate_Config = true;
			    break;
#endif
                        case 10:
                            //count_minimum_pulse_period_msec
                            unsigned long Min_Pulse_Period;

			    Min_Pulse_Period = strtol(Parametervalue.c_str(), nullptr, 0); // results in 0 if str is invalid
			    IOport->Pin[PinNumber].Count.SetRateLimit(Min_Pulse_Period);
			    result++;
			    break;

			case 11:
				// count_type
				ValueFound = LocateValueInParameterArray(Parametervalue, ParameterCountTypeValueNames,
									 CONFIG_NR_OF_PARAMETER_COUNT_TYPES);
				switch (ValueFound)
				{
					default:
						_log.Log(LOG_ERROR, "PiFace: Error config file: unknown value %s found",
							 Parametervalue.c_str());
						break;

					case 0: // generic
						IOport->Pin[PinNumber].Count.Type = COUNT_TYPE_GENERIC;
						break;
					case 1: // rfxmeter
						IOport->Pin[PinNumber].Count.Type = COUNT_TYPE_RFXMETER;
						break;
					case 2: // energy
						IOport->Pin[PinNumber].Count.Type = COUNT_TYPE_ENERGY;
						break;
				}
				result++;
				break;

			case 12:
				// count_divider
				IOport->Pin[PinNumber].Count.SetDivider(strtol(Parametervalue.c_str(), nullptr, 0));
				result++;
				break;
			    }
		    }
		    else
			    _log.Log(LOG_ERROR, "PiFace: Error config file: misformed config line %s found", Line.c_str());
	    }
	}
	ConfigFile.close();
#ifndef DISABLE_NEW_FUNCTIONS
        if (Regenerate_Config)
        {
            Regenerate_Config=false;
            _log.Log(LOG_ERROR,"PiFace: We got an initial value setting, so we are now recreating the logfile to avoid looping", configfile.c_str() );
            AutoCreate_piface_config();
        }
#endif
    }
    else {
        _log.Log(LOG_ERROR,"PiFace: Error PiFace config file: %s not found", configfile.c_str() );
        _log.Log(LOG_ERROR,"PiFace: loading defaults for PiFace(s)");
        LoadDefaultConfig();
        AutoCreate_piface_config();
    }
    return result;
}

void CPiFace::LoadDefaultConfig()
{
    for (int i=0; i<=3 ;i++)
    {
        for (int PinNr=0; PinNr<=7 ;PinNr++)
        {
            if (PinNr < 4)
            {
                m_Inputs[i].Pin[PinNr].Enabled=true;
            }
            m_Inputs[i].Pin[PinNr].Type=TOGGLE_RISING;
            m_Inputs[i].Pin[PinNr].Count.SetUpdateInterval(10000);
            m_Inputs[i].Pin[PinNr].Direction = 'I';

            if (PinNr >= 4)
            {
                m_Inputs[i].ConfigureCounter(PinNr,true);
            }
            m_Outputs[i].ConfigureCounter(PinNr,true);
            m_Outputs[i].Pin[PinNr].Type=LEVEL;
            m_Outputs[i].Pin[PinNr].Count.SetUpdateInterval(10000);
            m_Outputs[i].Pin[PinNr].Direction = 'O';
        }
    }
}

void CPiFace::AutoCreate_piface_config()
{
	char const *const explanation[] = {
		"//piface.conf [Autogenerated]\r\n",
		"//This file is part of the PiFace driver (written by Robin Stevenhagen) for Domoticz, and allows for more advanced pin "
		"configuration options\r\n",
		"//This file can optionally hold all the configuration for all attached PiFace boards.\r\n",
		"//Please note that if this file is found the default config is discarded, and only the configuration in this file is "
		"used, all IO that is not specified will\r\n/",
		"//be configured as not used.\r\n",
		"//\r\n",
		"// Configuration line syntax:\r\n",
		"// piface.<board address>.<I/O>.<pin number>.<parameter>=<value>\r\n",
		"//\r\n",
		"// <board address>:                 Check piface (JP1 & JP2) jumper settings, value between 0..3\r\n",
		"//\r\n",
		"// <I/O>:                           input\r\n",
		"//                                  output\r\n",
		"//\r\n",
		"// <pinnumber>:                     value between 0..7\r\n",
		"//\r\n",
		"// <parameter>:                     enabled\r\n",
		"//                                  pin_type\r\n",
		"//                                  count_enabled\r\n",
		"//                                  count_type\r\n",
		"//                                  count_divider\r\n",
		"//                                  count_update_interval_sec\r\n",
		"//                                  count_update_interval_diff_perc\r\n",
#ifndef DISABLE_NEW_FUNCTIONS
		"//                                  count_initial_value\r\n",
#endif
		"//                                  count_minimum_pulse_period\r\n",
		"//\r\n",
		"// Available <parameter> / <values>:\r\n",
		"// enabled:                         true  or 1\r\n",
		"//                                  false or 0\r\n",
		"//\r\n",
		"// pin_type:                        level                     // best to keep this parameter to level for outputs, as "
		"Domoticz does not like other values\r\n",
		"//                                  inv_level\r\n",
		"//                                  rising\r\n",
		"//                                  falling\r\n",
		"//\r\n",
		"// count_enabled:                   true  or 1\r\n",
		"//                                  false or 0\r\n",
		"//\r\n",
		"// count_type:                      generic or rfxmeter       // default\r\n",
		"//                                  energy\r\n",
		"//\r\n",
		"// count_divider:                   1000                      // default\r\n",
		"//                                  Pulses per unit.\r\n",
		"//\r\n",
		"// count_update_interval_sec:       0 to 3600\r\n",
		"// count_update_interval_diff_perc: 1 to 1000\r\n",
		"//                                  This parameter can be used to force trigger an update when the difference in pulse "
		"intervals is above a certain percentage.\r\n",
		"//                                  The value is a percentage of the previous interval. This can be useful for energy "
		"counters to quickly report large fluctuations\r\n",
		"//                                  in active power.\r\n",
		"//\r\n",
#ifndef DISABLE_NEW_FUNCTIONS
		"// count_initial_value:             0 to 4294967295 counts\r\n",
		"// Note1: The initial_value will only be used once. After it has been set, a new piface.conf will automatically be "
		"generated without the initial_value parameter(s)\r\n",
		"// to ensure that it will only be used once.\r\n",
		"// Using the initial_value to set a counter to a desired (start) value, will result in spikes in the Domoticz graphs.\r\n",
		"// Note2: The initial_value is in counter counts, Domoticz scales (divides) this by the factor that is set in the GUI "
		"settings \r\n",
		"//\r\n",
#endif
		"// count_minimum_pulse_period_msec: 0 to 4294967295 milliseconds\r\n",
		"//                                  This parameter sets a pulse rate limiter.\r\n",
		"//                                  If multiple pulses are detected within the specified time period, they are ignored "
		"until the specified idle time has been met.\r\n",
		"//                                  This can be useful for opto reflective sensors that count slow moving dials. Like "
		"water meters or gas meters,\r\n",
		"//                                  where the dial can stop on an opto barrier and cause oscillations (and unwanted "
		"counts).\r\n",
		"//                                  0 = off \r\n",
		"//                                  < 100 msec times will not be very effective.\r\n",
		"//\r\n",
		nullptr
	};

    char configline[100];
    std::string ValueText;
    std::string PortType;
    int Value;
    CIOPort *IOport;

    std::string configfile=szUserDataFolder + "piface.conf";
    std::fstream ConfigFile(configfile.c_str(), std::ios::out);

	int total_explanations = sizeof(explanation);

    if (ConfigFile.is_open())
    {
		int i = 0;
		while (explanation[i] != nullptr)
		{
			ConfigFile.write(explanation[i], strlen(explanation[i]));
			i++;
		}

		for (int BoardNr = 0; BoardNr < 1;
		     BoardNr++) // Note there could be 4 boards, but this will create a lot of entires in the configfile
		{
			for (int PinNr = 0; PinNr <= 7; PinNr++)
			{
				for (int IOType = 0; IOType <= 1; IOType++)
				{
					if (IOType == 0)
					{
						PortType = "output";
						IOport = &m_Outputs[BoardNr];
					}
					else
					{
						PortType = "input";
						IOport = &m_Inputs[BoardNr];
					}

					if (IOport->Pin[PinNr].Enabled)
						ValueText = "true";
					else
						ValueText = "false";
					sprintf(configline, "piface.%d.%s.%d.enabled=%s\r\n", BoardNr, PortType.c_str(), PinNr,
						ValueText.c_str());
					ConfigFile.write(configline, strlen(configline));

					ValueText = ParameterPinTypeValueNames[IOport->Pin[PinNr].Type];
					sprintf(configline, "piface.%d.%s.%d.pin_type=%s\r\n", BoardNr, PortType.c_str(), PinNr,
						ValueText.c_str());
					ConfigFile.write(configline, strlen(configline));

					if (IOport->Pin[PinNr].Count.Enabled)
						ValueText = "true";
					else
						ValueText = "false";
					sprintf(configline, "piface.%d.%s.%d.count_enabled=%s\r\n", BoardNr, PortType.c_str(), PinNr,
						ValueText.c_str());
					ConfigFile.write(configline, strlen(configline));

					ValueText = ParameterCountTypeValueNames[IOport->Pin[PinNr].Count.Type];
					sprintf(configline, "piface.%d.%s.%d.count_type=%s\r\n", BoardNr, PortType.c_str(), PinNr,
						ValueText.c_str());
					ConfigFile.write(configline, strlen(configline));

					if (IOport->Pin[PinNr].Count.Type != COUNT_TYPE_GENERIC
					    && IOport->Pin[PinNr].Count.Type != COUNT_TYPE_RFXMETER)
					{
						Value = IOport->Pin[PinNr].Count.GetDivider();
						sprintf(configline, "piface.%d.%s.%d.count_divider=%d\r\n", BoardNr, PortType.c_str(),
							PinNr, Value);
						ConfigFile.write(configline, strlen(configline));
					}

					Value = IOport->Pin[PinNr].Count.GetUpdateInterval() / 1000;
					sprintf(configline, "piface.%d.%s.%d.count_update_interval_sec=%d\r\n", BoardNr, PortType.c_str(),
						PinNr, Value);
					ConfigFile.write(configline, strlen(configline));

					Value = IOport->Pin[PinNr].Count.GetUpdateIntervalPerc();
					if (Value > 0)
					{
						sprintf(configline, "piface.%d.%s.%d.count_update_interval_diff_perc=%d\r\n", BoardNr,
							PortType.c_str(), PinNr, Value);
						ConfigFile.write(configline, strlen(configline));
					}

					Value = IOport->Pin[PinNr].Count.GetRateLimit();
					if (Value > 0)
					{
						sprintf(configline, "piface.%d.%s.%d.count_minimum_pulse_period_msec=%d\r\n", BoardNr,
							PortType.c_str(), PinNr, Value);
						ConfigFile.write(configline, strlen(configline));
					}

					sprintf(configline, "\r\n");
					ConfigFile.write(configline, strlen(configline));
				}
			}
		}
    }
    ConfigFile.close();
}

/***** end of config file stuff *****/

void CPiFace::CallBackSendEvent(const unsigned char *pEventPacket, const unsigned int PacketLength)
{
    std::string sendData;
    sendData.insert(sendData.begin(), pEventPacket, pEventPacket + PacketLength);
    std::lock_guard<std::mutex> l(m_queue_mutex);
    if (m_send_queue.size() < 100)
        m_send_queue.push_back(sendData);
    else
        _log.Log(LOG_ERROR, "PiFace: to much messages on queue!");
}

void CPiFace::CallBackSetPinInterruptMode(unsigned char devId,unsigned char pinID, bool Interrupt_Enable)
{
    unsigned char Cur_Int_Enable_State;
    unsigned char pinmask;
    //make sure that we clear the interrupts states before we update the port
    Read_MCP23S17_Register (devId,MCP23x17_INTFB);
    Read_MCP23S17_Register (devId,MCP23x17_INTCAPB);

    pinmask=1;
    pinmask<<=pinID;
    Cur_Int_Enable_State=Read_MCP23S17_Register (devId,MCP23x17_GPINTENB);

    if (Interrupt_Enable)
        Cur_Int_Enable_State|=pinmask; //enable pin interrupt
    else Cur_Int_Enable_State&=~pinmask; //disable pin interrupt

    Write_MCP23S17_Register (devId, MCP23x17_GPINTENB, Cur_Int_Enable_State);

    //_log.Log(LOG_NORM,"PiFace: SetPin Interrupt mode: devid: %d, Pinnr: %d, Enable %d-- Prev 0x%02X Cur 0x%02X",devId,pinID,Interrupt_Enable,Prev_Int_Enable_State,Cur_Int_Enable_State);
}


bool CPiFace::StartHardware()
{
    StopHardware();

	RequestStart();
	m_TaskQueue.RequestStart();

    m_InputSample_waitcntr=(PIFACE_INPUT_PROCESS_INTERVAL)*20;
    m_CounterEdgeSample_waitcntr=(PIFACE_COUNTER_COUNTER_INTERVAL)*20;

    memset(m_DetectedHardware,0,sizeof(m_DetectedHardware));

#ifndef DISABLE_NEW_FUNCTIONS
    LoadConfig();
#endif
    //Start worker thread
	if (Init_SPI_Device(1) > 0)
	{
		if (Detect_PiFace_Hardware() > 0)
		{
			//we have hardware, so lets use it
			for (int devId = 0; devId < 4; devId++)
			{
				if (m_DetectedHardware[devId] == true)
					Init_Hardware(devId);
			}
#ifdef DISABLE_NEW_FUNCTIONS
			LoadConfig();
#endif

			for (int devId = 0; devId < 4; devId++)
			{
				if (m_DetectedHardware[devId] == true)
					GetAndSetInitialDeviceState(devId);
			}

			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
			SetThreadNameInt(m_thread->native_handle());
			m_queue_thread = std::make_shared<std::thread>([this] { Do_Work_Queue(); });
			SetThreadName(m_queue_thread->native_handle(), "PiFaceQueue");
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
    m_bIsStarted=true;
    sOnConnected(this);
    return (m_thread != nullptr);
}

bool CPiFace::StopHardware()
{
	RequestStop();
	m_TaskQueue.RequestStop();

	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}

	if (m_queue_thread)
	{
		m_queue_thread->join();
		m_queue_thread.reset();
	}
#ifdef HAVE_LINUX_SPI
    if (m_fd > 0) {
        close(m_fd);
        m_fd = 0;
    }
#endif
    m_bIsStarted=false;
    return true;
}

bool CPiFace::WriteToHardware(const char *pdata, const unsigned char length)
{
    const tRBUF *SendData = reinterpret_cast<const tRBUF*>(pdata);
    int mask=0x01;
    unsigned char CurrentLatchState;
    unsigned char OutputData;
    unsigned char PortType=' ';

    int devId =0;
    int pinnr =0;

    if ((SendData->LIGHTING1.packettype == pTypeLighting1) && (SendData->LIGHTING1.subtype == sTypeIMPULS))
    {
        PortType=(SendData->LIGHTING1.housecode);
        devId=(SendData->LIGHTING1.unitcode /10)&0x03;
        pinnr =(SendData->LIGHTING1.unitcode %10)&0x07;
        if (PortType == 'O')
        {
            //_log.Log(LOG_NORM,"Piface: WriteToHardware housecode %c, packetlength %d", SendData->LIGHTING1.housecode,SendData->LIGHTING1.packetlength );
            CurrentLatchState = Read_MCP23S17_Register(devId, MCP23x17_OLATA);
            //_log.Log(LOG_NORM,"PiFace: Read input state 0x%02X", m_OutputState[devId].Current);

            OutputData = CurrentLatchState;
            mask <<= pinnr;

            if (SendData->LIGHTING1.cmnd == light1_sOff)
            {
                OutputData &= ~mask;
            }
            else OutputData |= mask;

            Write_MCP23S17_Register(devId, MCP23x17_GPIOA, OutputData);
            //  _log.Log(LOG_NORM,"Piface: WriteToHardware housecode %c, devid %d, output %d, PrevOut 0x%02X, Set 0x%02X",PortType, devId, pinnr, CurrentLatchState,OutputData );
        }
        else
        {
            _log.Log(LOG_ERROR, "Piface: wrong housecode %c", PortType);
            return false;
        }
    }
    else
    {
        _log.Log(LOG_ERROR, "PiFace: WriteToHardware packet type %d or subtype %d unknown", SendData->LIGHTING1.packettype, SendData->LIGHTING1.subtype);
        return false;
    }
    return true;
}

void CPiFace::Do_Work()
{
    int devId;
    _log.Log(LOG_STATUS,"PiFace: Worker started...");
    int msec_counter = 0;
    int sec_counter = 0;
    while (!IsStopRequested(PIFACE_WORKER_THREAD_SLEEP_INTERVAL_MS))
    {
        msec_counter++;
        if (msec_counter == (1000 / PIFACE_WORKER_THREAD_SLEEP_INTERVAL_MS))
        {
            msec_counter = 0;
            sec_counter++;
            if (sec_counter % 12 == 0) {
		    m_LastHeartbeat = mytime(nullptr);
	    }
	}

	m_InputSample_waitcntr++;
	m_CounterEdgeSample_waitcntr++;

	// sample interrupt states faster for edge detection on count
	if (m_CounterEdgeSample_waitcntr >= PIFACE_COUNTER_COUNTER_INTERVAL)
	{
		m_CounterEdgeSample_waitcntr = 0;
		for (devId = 0; devId < 4; devId++)
		{
			Sample_and_Process_Input_Interrupts(devId);
		}
	}

	// sample pin states more slowly to avoid spamming Domoticz
	if (m_InputSample_waitcntr >= PIFACE_INPUT_PROCESS_INTERVAL)
	{
		m_InputSample_waitcntr = 0;
		for (devId = 0; devId < 4; devId++)
		{
			Sample_and_Process_Inputs(devId);
			Sample_and_Process_Outputs(devId);
		}
	}
    }
    _log.Log(LOG_STATUS,"PiFace: Worker stopped...");
}

void CPiFace::Do_Work_Queue()
{
    std::vector<std::string>::iterator itt;
    while (!m_TaskQueue.IsStopRequested(100))
    {
        if (m_send_queue.empty())
            continue;

        std::string sendData;
        m_queue_mutex.lock();
        itt = m_send_queue.begin();
        sendData = *itt;
        m_send_queue.erase(itt);
        m_queue_mutex.unlock();
	sDecodeRXMessage(this, (const unsigned char *)sendData.c_str(), nullptr, 255, m_Name.c_str());
    }
}

// Open a connection to the piface
// Returns a file id, or -1 if not open/error
int CPiFace::Init_SPI_Device(int Init)
{
    m_fd=0;
    int result=-1;
    unsigned char spiMode  = 0 ;
    unsigned char spiBPW   = 8 ;
    int           speed       = 4000000 ;

    _log.Log(LOG_STATUS,"PiFace: Starting PiFace_SPI_Start()");
#ifdef HAVE_LINUX_SPI
    // Open port for reading and writing
    if ((m_fd = open("/dev/spidev0.0", O_RDWR)) >= 0)
    {
        // Set the port options and set the address of the device
        if (ioctl (m_fd, SPI_IOC_WR_MODE, &spiMode)         >= 0)
        {
            if (ioctl (m_fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) >= 0)
            {
                if (ioctl (m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)   >= 0)
                {
                    result=1;
                    //we are successfull
                    _log.Log(LOG_NORM,"PiFace: SPI device opened successfully");

                }
                else
                    _log.Log(LOG_NORM,"PiFace: SPI Speed Change failure: %s", strerror (errno)) ;
            }
            else
                _log.Log(LOG_NORM,"PiFace: SPI BPW Change failure: %s", strerror (errno)) ;

        }
        else
            _log.Log(LOG_NORM,"PiFace: SPI Mode Change failure: %s", strerror (errno)) ;
    }
    else
        _log.Log(LOG_NORM,"PiFace: Unable to open SPI device: %s", strerror (errno));

#endif

    if (result == -1)
    {
#ifdef HAVE_LINUX_SPI
        close(m_fd);
#endif
        return -1;
    }
    return m_fd;
}

int CPiFace::Detect_PiFace_Hardware()
{
    int NrOfFoundBoards=0;
    unsigned char read_iocon;
    unsigned char read_ioconb;
    int devId;

    //first write to all possible PiFace addresses.
    for (devId=0; devId<4; devId++)
    {
        Write_MCP23S17_Register (devId, MCP23x17_IOCON,  IOCON_INIT | IOCON_HAEN) ;
        Write_MCP23S17_Register (devId, MCP23x17_IOCONB,  IOCON_INIT | IOCON_HAEN) ;
    }

    //now read them back, to determine if they are present

    for (devId=0; devId<4; devId++)
    {
        read_iocon=Read_MCP23S17_Register (devId, MCP23x17_IOCON);
        read_ioconb=Read_MCP23S17_Register (devId, MCP23x17_IOCONB);

        if ((read_iocon == ( IOCON_INIT | IOCON_HAEN)) && (read_ioconb == ( IOCON_INIT | IOCON_HAEN)))
        {
            //hé someone appears to be home...
            m_DetectedHardware[devId]=true;
            NrOfFoundBoards++;
        }
    }

    if (NrOfFoundBoards)
    {
        _log.Log(LOG_STATUS,"PiFace: Found the following PiFaces:");
        for (devId=0; devId<4; devId++)
        {
            if (m_DetectedHardware[devId]==true)
                _log.Log(LOG_STATUS,"PiFace: %d",devId);
        }
    }
    else _log.Log(LOG_STATUS,"PiFace: Sorry, no PiFaces were found");
    return NrOfFoundBoards;
}

void CPiFace::Init_Hardware(unsigned char devId)
{
    Write_MCP23S17_Register (devId, MCP23x17_IOCON,  IOCON_INIT | IOCON_HAEN) ;
    Write_MCP23S17_Register (devId, MCP23x17_IOCONB, IOCON_INIT | IOCON_HAEN) ;

    //setup PortA as output
    Write_MCP23S17_Register (devId, MCP23x17_IODIRA,  0x00) ;         //set all pins on Port A as output

    //lets init the other registers so we always have a clear startup situation
    Write_MCP23S17_Register (devId, MCP23x17_GPINTENA, 0x00);          //The PiFace does not support the interrupt capabilities, so set it to 0, and useless for output.
    Write_MCP23S17_Register (devId, MCP23x17_DEFVALA,     0x00);      //Default compare value register, useless for output
    Write_MCP23S17_Register (devId, MCP23x17_INTCONA,     0x00);        //Interrupt based on current and prev pin state, not Default value
    //Write_MCP23S17_Register (devId, MCP23x17_INTFA,     0);           // Read only interrupt flag register,not used on output
    //Write_MCP23S17_Register (devId, MCP23x17_INTCAPA, );             // Read only interrupt status register, captures port status on interrupt, not ued on output
    Write_MCP23S17_Register (devId, MCP23x17_OLATA, 0xFF);             // Set the output latches to 1, otherwise the relays are activated

    //setup PortB as input
    Write_MCP23S17_Register (devId, MCP23x17_IODIRB, 0xFF) ;         //set all pins on Port B as input
    Write_MCP23S17_Register (devId, MCP23x17_GPPUB,  0xFF) ;          //Enable pullup resistors, so the buttons work immediately
    Write_MCP23S17_Register (devId, MCP23x17_IPOLB,  0xFF) ;          //Invert input pin state, so we will see an active state as as 1.

    //lets init the other registers so we always have a clear startup situation
    Write_MCP23S17_Register (devId, MCP23x17_GPINTENB, 0x00);          //The PiFace does not support the interrupt capabilities, so set it to 0.
    //Todo: use the int detection for small pulses
    Write_MCP23S17_Register (devId, MCP23x17_DEFVALB,     0x00);      //Default compare value register, we do not use it (yet), but lets set it to 0 (assuming not active state).
    Write_MCP23S17_Register (devId, MCP23x17_INTCONB,     0x00);         //Interrupt based on current and prev pin state, not Default value
    //Write_MCP23S17_Register (devId, MCP23x17_INTFB,     0);           // Read only interrupt flag register, listed as a reminder
    //Write_MCP23S17_Register (devId, MCP23x17_INTCAPB, );             // Read only interrupt status register, captures port status on interrupt, listed as a reminder
    Write_MCP23S17_Register (devId, MCP23x17_OLATB, 0x00);             // Set the output latches to 0, note: not used.

    Write_MCP23S17_Register (devId, MCP23x17_GPIOA,  0x00) ;         //set all pins on Port A as output, and deactivate
}

void CPiFace::GetAndSetInitialDeviceState(int devId)
{
    unsigned char PortState;
    PortState= Read_MCP23S17_Register (devId, MCP23x17_OLATA) ;
    m_Outputs[devId].Init(true,m_HwdID,devId,'O',PortState);

    PortState= Read_MCP23S17_Register (devId, MCP23x17_GPIOB) ;
    m_Inputs[devId].Init(true,m_HwdID,devId,'I',PortState);
}

int CPiFace::Read_Write_SPI_Byte(unsigned char *data, int len)
{
#ifdef HAVE_LINUX_SPI
    struct spi_ioc_transfer spi ;
    memset (&spi, 0, sizeof(spi));

    spi.tx_buf        = (unsigned long)data ;
    spi.rx_buf        = (unsigned long)data ;
    spi.len           = len ;
    spi.delay_usecs   = 0 ;
    spi.speed_hz      = 4000000;
    spi.bits_per_word = 8 ;

    return ioctl (m_fd, SPI_IOC_MESSAGE(1), &spi) ;
#else
    return 0;
#endif
}

int CPiFace::Read_MCP23S17_Register(unsigned char devId, unsigned char reg)
{
    unsigned char spiData [4] ;

    spiData [0] = CMD_READ | ((devId & 7) << 1);
    spiData [1] = reg ;

    Read_Write_SPI_Byte(spiData, 3);

    return spiData [2];
}

int CPiFace::Write_MCP23S17_Register(unsigned char devId, unsigned char reg, unsigned char data)
{
    unsigned char spiData [4] ;

    spiData [0] = CMD_WRITE | ((devId & 7) << 1) ;
    spiData [1] = reg ;
    spiData [2] = data ;

    return (Read_Write_SPI_Byte( spiData, 3)) ;
}

void CPiFace::Sample_and_Process_Inputs(unsigned char devId)
{
    unsigned char PortState;

    if ( m_Inputs[devId].IsDevicePresent())
    {
        PortState=Read_MCP23S17_Register (devId, MCP23x17_GPIOB);
        m_Inputs[devId].Update(PortState);
    }
}

void CPiFace::Sample_and_Process_Outputs(unsigned char devId)
{
    unsigned char PortState;

    if ( m_Outputs[devId].IsDevicePresent())
    {
        PortState=Read_MCP23S17_Register (devId, MCP23x17_GPIOA);
        m_Outputs[devId].Update(PortState);
    }
}

void CPiFace::Sample_and_Process_Input_Interrupts(unsigned char devId)
{
    unsigned char PortInterruptState;
    unsigned char PortState;

    if ( m_Inputs[devId].IsDevicePresent())
    {
        PortInterruptState=Read_MCP23S17_Register(devId, MCP23x17_INTFB);
        PortState=Read_MCP23S17_Register(devId, MCP23x17_INTCAPB);
        m_Inputs[devId].UpdateInterrupt(PortInterruptState,PortState);
    }
}

void CIOCount::SetUpdateInterval(unsigned long NewValue_ms)
{
    UpdateInterval_ms=NewValue_ms;
    UpdateDownCount_ms=UpdateInterval_ms;
}

bool CIOCount::ProcessUpdateInterval(unsigned long PassedTime_ms)
{
    bool Update=false;
    if (Enabled)
    {
        if (UpdateDownCount_ms > PassedTime_ms)
            UpdateDownCount_ms-=PassedTime_ms;
        else {
            UpdateDownCount_ms=UpdateInterval_ms;
            Update=true;
        }
        if (!InitialStateSent)
        {
            InitialStateSent=true;
            Update=true;
        }

        boost::posix_time::ptime Now = boost::posix_time::microsec_clock::universal_time();

        if (
            UpdateIntervalPerc > 0 &&
            Cur_Interval.total_milliseconds() > 0 &&
            Last_Interval.total_milliseconds() > 0 &&
            //rate limit the callbacks to max 1 per sec to prevent spamming the upper layer when power suddenly drops
            ( Now - Last_Callback ).total_milliseconds() >= 1000
        )
        {
            if ( Now - Cur_Pulse > Cur_Interval )
            {
                Cur_Interval = Now - Cur_Pulse;
            }

			double perc = (100. / Last_Interval.total_milliseconds()) * labs((long)(Cur_Interval.total_milliseconds() - Last_Interval.total_milliseconds()));
            if ( perc > UpdateIntervalPerc )
            {
                UpdateDownCount_ms=UpdateInterval_ms;
                Update=true;
            }
        }
    }
    return Update;
}

int CIOCount::Update(unsigned long Counts)
{
    int result = -1;

    Last_Pulse = Cur_Pulse;
    Cur_Pulse = boost::posix_time::microsec_clock::universal_time();
    Cur_Interval = Cur_Pulse - Last_Pulse;

    if (
        Enabled && (
            Minimum_Pulse_Period_ms == 0 ||
            Minimum_Pulse_Period_ms <= Cur_Interval.total_milliseconds()
        )
    )
    {
        Total += Counts;
        result = 1;
    }

    return result;
}


CIOCount::CIOCount()
{
    ResetTotal();
    UpdateInterval_ms = 10000;
    UpdateIntervalPerc = 0;
    Enabled = false;
    Type = COUNT_TYPE_GENERIC;
    InitialStateSent = false;
    UpdateDownCount_ms = 0;
    Minimum_Pulse_Period_ms = 0;
    Divider = 1000;
    Last_Pulse = boost::posix_time::microsec_clock::universal_time();
    Cur_Pulse = boost::posix_time::microsec_clock::universal_time();
    Last_Callback = boost::posix_time::microsec_clock::universal_time();
    Last_Interval = boost::posix_time::milliseconds(0);
    Cur_Interval = boost::posix_time::milliseconds(0);
};

int CIOPinState::Update(bool New)
{
    int StateChange=-1; //nothing changed

    Last=Current;
    Current=New;

    if (Enabled)
    {
        switch (Type)
        {
            default:
            case LEVEL:
                if ((Last ^ Current) == true)
                {
                    if (Current)
                        StateChange=1;
                    else
                        StateChange=0;
                }
                break;
            case INV_LEVEL:    //inverted input
                if ((Last ^ Current) == true)
                {
                    if (Current)
                        StateChange=0;
                    else
                        StateChange=1;
                }
                break;
            case TOGGLE_RISING:
                if (((Last ^ Current) == true) && (Current == true))
                {
                    if (Toggle)
                        StateChange=1;
                    else
                        StateChange=0;
                    Toggle=!Toggle;
                }
                break;
            case TOGGLE_FALLING:
                if (((Last ^ Current) == true) && (Current == false))
                {
                    if (Toggle)
                        StateChange=1;
                    else
                        StateChange=0;
                    Toggle=!Toggle;
                }
                break;
        }
    }

    if (Direction == 'O')
    {
        if (((Last ^ Current) == true) && (Current == true))
        {
            Count.Update(1);
        }
    }

    return StateChange;
}

int CIOPinState::UpdateInterrupt(bool IntFlag,bool PinState)
{
    int Changed=-1;

    if (IntFlag && PinState) //we have a rising edge so count
    {
        Changed=Count.Update(1);
    }
    return (Changed);
}

int CIOPinState::GetInitialState()
{
    int CurState=-1; //Report not enabled

    if (Enabled)
    {
        switch (Type)
        {
            default:
            case LEVEL:
                if (Current)
                    CurState=1;
                else
                    CurState=0;
                break;
            case INV_LEVEL:    //inverted input
                if (Current)
                    CurState=0;
                else
                    CurState=1;
                break;
            case TOGGLE_RISING:
                if (Toggle)
                    CurState=1;
                else
                    CurState=0;
                break;

            case TOGGLE_FALLING:
                if (Toggle)
                    CurState=1;
                else
                    CurState=0;
                break;
        }
    }
    return CurState;
}


CIOPinState::CIOPinState()
{
    Last=false;
    Current=true;
    Id=0;
    Type=0;
    Enabled=false;
    Toggle=false;
    InitialStateSent=false;
    Direction=' ';
};

void CIOPort::Init(bool Available, int hwdId, int devId /* 0 - 4 */, unsigned char housecode, unsigned char initial_state)
{
    int PinNr;
    unsigned long value;
    bool found;

    std::vector<std::vector<std::string> > result;
    std::vector<std::string> resultparts;

    Present = Available;

    //set generic packet info for LIGHTING1 packet, so we do not have every packet
    IOPinStatusPacket.LIGHTING1.packetlength = sizeof(IOPinStatusPacket.LIGHTING1) -1;
    IOPinStatusPacket.LIGHTING1.housecode = housecode;//report to user as input
    IOPinStatusPacket.LIGHTING1.packettype = pTypeLighting1;
    IOPinStatusPacket.LIGHTING1.subtype = sTypeIMPULS;
    IOPinStatusPacket.LIGHTING1.rssi = 12;
    IOPinStatusPacket.LIGHTING1.seqnbr = 0;

    for (PinNr=0;PinNr <= 7;PinNr++)
    {
        found = false;
        switch( Pin[PinNr].Count.Type )
        {
            case COUNT_TYPE_GENERIC:
            case COUNT_TYPE_RFXMETER:
                //set generic packet info for RFXMETER packet, so we do not have every packet
                Pin[PinNr].IOPinCounterPacket.RFXMETER.packetlength = sizeof(Pin[PinNr].IOPinCounterPacket.RFXMETER) -1;
                Pin[PinNr].IOPinCounterPacket.RFXMETER.packettype = pTypeRFXMeter;
                Pin[PinNr].IOPinCounterPacket.RFXMETER.subtype = sTypeRFXMeterCount;
                Pin[PinNr].IOPinCounterPacket.RFXMETER.rssi = 12;
                Pin[PinNr].IOPinCounterPacket.RFXMETER.seqnbr = 0;

                int meterid;
                if (housecode == 'I')
                    meterid=100+PinNr + (devId*10);
                else
                    meterid=0+PinNr + (devId*10);

                result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND Type=%d AND DeviceID='%d')", hwdId, int(pTypeRFXMeter), meterid);
                if (result.size() == 1)
                {
                    value = atol(result[0][0].c_str());
                    found = true;
                }
                break;

            case COUNT_TYPE_ENERGY:
                int nodeId;
                if (housecode == 'I')
                    nodeId=300+devId;
                else
                    nodeId=200+devId;

                int dID = (nodeId << 8) | PinNr;

                result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND Type=%d AND DeviceID='%08X')", hwdId, int(pTypeGeneral), dID);
                if (result.size() == 1)
                {
                    StringSplit(result[0][0], ";", resultparts);
                    double dValue = 0;
                    if (resultparts.size() == 1)
                    {
                        dValue = atol(resultparts[0].c_str()) / ( 1000. / Pin[PinNr].Count.GetDivider() );
                        found = true;
                    }
                    else if (resultparts.size() == 2)
                    {
                        dValue = atol(resultparts[1].c_str()) / ( 1000. / Pin[PinNr].Count.GetDivider() );
                        found = true;
                    }
                    value = (unsigned long)dValue;
                }
                break;
        }

        if ( found ) {
            Pin[PinNr].Count.SetTotal(value);
        }
    }

    Last=initial_state;
    Current=initial_state;
    Update(initial_state);
}

int CIOPort::Update(unsigned char New)
{
    int mask=0x01;
    int ChangeState=-1;
    bool UpdateCounter=false;

    CPiFace *myCallback = reinterpret_cast<CPiFace*>(Callback_pntr);

    Last=Current;
    Current=New;

    for (int PinNr=0; PinNr<=7 ;PinNr++)
    {
        ChangeState=Pin[PinNr].Update((New&mask)==mask);
        if ((ChangeState >=0) || ((!Pin[PinNr].InitialStateSent) && Pin[PinNr].Enabled))
        {
            Pin[PinNr].InitialStateSent=true;
            if (ChangeState <0)
            {
                //we have to sent current state
                ChangeState=Pin[PinNr].GetInitialState();
            }
            //We have something to report to the upper layer
            IOPinStatusPacket.LIGHTING1.cmnd = (ChangeState != 0) ? light1_sOn : light1_sOff;
            IOPinStatusPacket.LIGHTING1.seqnbr++;
            IOPinStatusPacket.LIGHTING1.unitcode = PinNr + (devId*10) ; //report inputs from PiFace X (X0..X9)
            myCallback->CallBackSendEvent((const unsigned char *)&IOPinStatusPacket, sizeof(IOPinStatusPacket.LIGHTING1));
        }

        UpdateCounter=Pin[PinNr].Count.ProcessUpdateInterval(PIFACE_INPUT_PROCESS_INTERVAL*PIFACE_WORKER_THREAD_SLEEP_INTERVAL_MS);
        if (UpdateCounter)
        {
            unsigned long Count = Pin[PinNr].Count.GetTotal();
            unsigned long LastCount = Pin[PinNr].Count.GetLastTotal();

            Pin[PinNr].Count.Last_Callback = boost::posix_time::microsec_clock::universal_time();

            switch( Pin[PinNr].Count.Type )
            {
                case COUNT_TYPE_GENERIC:
                case COUNT_TYPE_RFXMETER:
                    int meterid;
                    if (IOPinStatusPacket.LIGHTING1.housecode == 'I')
                        meterid=100+PinNr + (devId*10);
                    else
                        meterid=0+PinNr + (devId*10);

                    Pin[PinNr].IOPinCounterPacket.RFXMETER.id1=((meterid >>8) & 0xFF);
                    Pin[PinNr].IOPinCounterPacket.RFXMETER.id2= (meterid & 0xFF);

                    if (Count != LastCount)
                    {
                        Pin[PinNr].Count.SetLastTotal(Count);
                        //    _log.Log(LOG_NORM,"Counter %lu\n",Count);
                        Pin[PinNr].IOPinCounterPacket.RFXMETER.count1 = (unsigned char)((Count >> 24) & 0xFF);
                        Pin[PinNr].IOPinCounterPacket.RFXMETER.count2 = (unsigned char)((Count >> 16) & 0xFF);
                        Pin[PinNr].IOPinCounterPacket.RFXMETER.count3 = (unsigned char)((Count >> 8) & 0xFF);
                        Pin[PinNr].IOPinCounterPacket.RFXMETER.count4 = (unsigned char)((Count)& 0xFF);
                        Pin[PinNr].IOPinCounterPacket.RFXMETER.seqnbr++;
                        //    _log.Log(LOG_NORM,"RFXMeter Packet C1 %d, C2 %d C3 %d C4 %d\n",Pin[PinNr].IOPinCounterPacket.RFXMETER.count1,Pin[PinNr].IOPinCounterPacket.RFXMETER.count2,Pin[PinNr].IOPinCounterPacket.RFXMETER.count3,Pin[PinNr].IOPinCounterPacket.RFXMETER.count4);
                        myCallback->CallBackSendEvent((const unsigned char *)&Pin[PinNr].IOPinCounterPacket, sizeof(Pin[PinNr].IOPinCounterPacket.RFXMETER));
                    }
                    break;

                case COUNT_TYPE_ENERGY:
                    int nodeId;
                    if (IOPinStatusPacket.LIGHTING1.housecode == 'I')
                        nodeId=300+devId;
                    else
                        nodeId=200+devId;

                    // Energy devices are updated *every* time because their current power usage (Watt) drops if no pulses
                    // are registered between the interval.
                    Pin[PinNr].Count.SetLastTotal(Count);

                    if (
                        Pin[PinNr].Count.Cur_Interval.total_milliseconds() > 0 &&
                        Pin[PinNr].Count.GetDivider() > 0
                    )
                    {
                        Pin[PinNr].Count.Last_Interval = Pin[PinNr].Count.Cur_Interval;
                        double dEnergy = ( ( 1. / Pin[PinNr].Count.GetDivider() ) * Count  ); // kWh
                        double dPower = ( ( 1000. / Pin[PinNr].Count.GetDivider() ) * ( 1000. / Pin[PinNr].Count.Cur_Interval.total_milliseconds() ) * 3600 ); // Watt
                        myCallback->SendKwhMeter(nodeId, PinNr, 255, dPower, dEnergy, "kWh Meter" );
                    }
                    break;
            }
        }
        mask<<=1;
    }

    return (ChangeState);
}

int CIOPort::UpdateInterrupt(unsigned char IntFlag,unsigned char PinState)
{
    int mask=0x01;
    int ChangeState=-1; //nothing changed
    int Changed;

    for (int PinNr=0; PinNr<=7 ;PinNr++)
    {
        Changed=Pin[PinNr].UpdateInterrupt(((IntFlag&mask)==mask),((PinState&mask)==mask));
        if (Changed != -1)
            ChangeState=Changed;
        mask<<=1;
    }
    return (ChangeState);
}

void CIOPort::ConfigureCounter(unsigned char Pinnr,bool Enable)
{
    CPiFace *myCallback = reinterpret_cast<CPiFace*>(Callback_pntr);

    Pin[Pinnr].Count.Enabled=Enable;
    if (Pin[Pinnr].Direction == 'I')
    {
        myCallback->CallBackSetPinInterruptMode(devId, Pinnr, Enable);
    }
}

CIOPort::CIOPort()
{
    Last=0;
    Current=0;
    Present=false;
    Callback_pntr = nullptr;
    PortType = 0;
    devId = 0;
    for (int PinNr=0; PinNr<=7 ;PinNr++)
    {
        Pin[PinNr].Id=PinNr;
    }

};

//Webserver helpers
namespace http {
    namespace server {
        void CWebServer::ReloadPiFace(WebEmSession & session, const request& req, std::string & redirect_uri)
        {
            redirect_uri = "/index.html";
            if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

            std::string idx = request::findValue(&req, "idx");
	    if (idx.empty())
	    {
		    return;
	    }

	    m_mainworker.RestartHardware(idx);
	}
    } // namespace server
} // namespace http
