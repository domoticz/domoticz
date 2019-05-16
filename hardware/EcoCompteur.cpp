#include "stdafx.h"
#include "EcoCompteur.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/WebServer.h"

/*
Driver for Legrand EcoCompteur IP (4 120 00)
https://www.legrand.fr/pro/catalogue/31736-ecocompteurs-ip/ecocompteur-modulaire-ip-pour-mesure-consommation-sur-6-postes-110v-a-230v-6-modules

It's only supporting the 5 electrical channels from Current Transformers.
It's getting instantaneous power from inst.json and overall power consumption for each chanel from log2.csv
TODO :
	- Supporting gas and water channels
	- Supporting radio extension (https://www.legrand.fr/pro/catalogue/31737-solution-radio/module-de-reception-radio-des-impulsions-des-compteurs-gaz-et-eau-pour-ecocompteur-ip-reference-412000)
	- Supporting main meter from TIC channel (parsing html file)
	- Retreiving custom names (parsing html files)
*/

CEcoCompteur::CEcoCompteur(const int ID, const std::string& url, const unsigned short port):
	m_url(url)
{
	m_port = port;
	m_HwdID = ID;
	m_refresh = 10;	// Refresh time in sec
	Init();
}

CEcoCompteur::~CEcoCompteur(void)
{
}

void CEcoCompteur::Init()
{
}

bool CEcoCompteur::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

bool CEcoCompteur::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CEcoCompteur::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CEcoCompteur::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CEcoCompteur::Do_Work()
{
	int sec_counter = m_refresh - 5; // Start 5 sec before refresh

	_log.Log(LOG_STATUS, "EcoCompteur: Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % m_refresh == 0) {
			GetScript();
		}
	}
	_log.Log(LOG_STATUS, "EcoCompteur: Worker stopped...");
}

void CEcoCompteur::GetScript()
{
	std::string sInst, sLog2;

	// Download instantaneous wattage
	std::stringstream szURL;
	szURL << m_url << ":" << m_port << "/inst.json";
	if (!HTTPClient::GET(szURL.str(), sInst))
	{
		_log.Log(LOG_ERROR, "EcoCompteur: Error getting 'inst.json' from url : " + m_url);
		return;
	}

	// Download hourly report
	std::stringstream szLogURL;
	szLogURL << m_url << ":" << m_port << "/log2.csv";
	if (!HTTPClient::GET(szLogURL.str(), sLog2))
	{
		_log.Log(LOG_ERROR, "EcoCompteur: Error getting 'log2.csv' from url : " + m_url);
		return;
	}

	// Parse inst.json
	Json::Value root;
	Json::Reader jReader;
	jReader.parse(sInst, root);

	// Parse log2.csv
	if (sLog2.length() == 0)
	{
		_log.Log(LOG_ERROR, "EcoCompteur: log2.csv looks empty");
		return;
	}
	size_t position = sLog2.rfind("\r\n",sLog2.length()-3);	// Looking for the beginning of lastline
	if (position == std::string::npos)	// if not found -> there is only one line in the file
	{
		position = 0;
	} else {						// else, we skip 2 leading chars (\r\n)
		position += 2;
	}
	std::string lastline = sLog2.substr(position, sLog2.length() - position - 2); // finally getting lastline
	std::vector<std::string> fields;
	StringSplit(lastline, ";", fields);

	// Sending kWh meter data
	if (!root["data1"].empty())
	{
		SendKwhMeter(m_HwdID, 1, 255, root["data1"].asFloat(), atof(fields[7].c_str()), "Conso 1");
	}

	if (!root["data2"].empty())
	{
		SendKwhMeter(m_HwdID, 2, 255, root["data2"].asFloat(), atof(fields[9].c_str()), "Conso 2");
	}

	if (!root["data3"].empty())
	{
		SendKwhMeter(m_HwdID, 3, 255, root["data3"].asFloat(), atof(fields[11].c_str()), "Conso 3");
	}

	if (!root["data4"].empty())
	{
		SendKwhMeter(m_HwdID, 4, 255, root["data4"].asFloat(), atof(fields[13].c_str()), "Conso 4");
	}

	if (!root["data5"].empty())
	{
		SendKwhMeter(m_HwdID, 5, 255, root["data5"].asFloat(), atof(fields[15].c_str()), "Conso 5");
	}
}

/* log2.csv
0	jour
1	mois
2	annee
3	heure
4	minute
5	energie_tele_info
6	prix_tele_info
7	energie_circuit1
8	prix_circuit1
9	energie_cirucit2
10	prix_circuit2
11	energie_circuit3
12	prix_circuit3
13	energie_circuit4
14	prix_circuit4
15	energie_circuit5
16	prix_circuit5
17	volume_entree1
18	volume_entree2
19	tarif
20	energie_entree1
21	energie_entree2
22	prix_entree1
23	prix_entree2
24	volume_entree_radio1
25	energie_entree_radio1
26	prix_entree_radio1
27	volume_entree_radio2
28	energie_entree_radio2
29	prix_entree_radio2
30	volume_entree_radio3
31	energie_entree_radio3
32	prix_entree_radio3
33	volume_entree_radio4
34	energie_entree_radio4
35	prix_entree_radio4
*/


/* inst.json
"data1":0.000000,		Puissance circuit 1
"data2":1436.000000,	Puissance circuit 2
"data3":1216.000000,	Puissance circuit 3
"data4":0.000000,		Puissance circuit 4
"data5":0.000000,		Puissance circuit 5
"data6":0.000000,
"data6m3":0.000000,
"data7":0.000000,
"data7m3":0.000000,
"heure":20,				Heure
"minute":23,			Minute
"CIR1_Nrj":0.000000,
"CIR1_Vol":0.000000,
"CIR2_Nrj":0.000000,
"CIR2_Vol":0.000000,
"CIR3_Nrj":0.000000,
"CIR3_Vol":0.000000,
"CIR4_Nrj":0.000000,
"CIR4_Vol":0.000000,
"Date_Time":1517343832	Unix timestamp
*/
