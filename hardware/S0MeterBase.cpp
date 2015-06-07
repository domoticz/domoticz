#include "stdafx.h"
#include "S0MeterBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>


S0MeterBase::S0MeterBase(void)
{
	m_bufferpos = 0;
	int ii;
	for (ii = 0; ii < max_s0_meters; ii++)
	{
		m_meters[ii].m_counter_start = 0;
		m_meters[ii].m_current_counter = 0;
		m_meters[ii].total_pulses = 0;
		m_meters[ii].first_total_pulses_received = 0;
		m_meters[ii].m_CurrentUsage = 0;
		m_meters[ii].m_value_buffer_total = 0;
		m_meters[ii].m_value_buffer_write_pos = 0;
		m_meters[ii].m_PacketsSinceLastPulseChange = 0;
		m_meters[ii].m_last_values[0] = 0;
		m_meters[ii].m_last_values[1] = 0;
		m_meters[ii].m_last_values[2] = 0;
		m_meters[ii].m_last_values[3] = 0;
		m_meters[ii].m_firstTime = true;
	}
}


S0MeterBase::~S0MeterBase(void)
{
}

void S0MeterBase::ReloadLastTotals()
{
	//Reset internals
	int ii;
	for (ii=0; ii<max_s0_meters; ii++)
	{
		m_meters[ii].m_counter_start=0;
		m_meters[ii].m_current_counter = 0;
		m_meters[ii].total_pulses = 0;
		m_meters[ii].first_total_pulses_received = 0;
		m_meters[ii].m_CurrentUsage = 0;
		m_meters[ii].m_value_buffer_total = 0;
		m_meters[ii].m_value_buffer_write_pos = 0;
		m_meters[ii].m_PacketsSinceLastPulseChange = 0;
		m_meters[ii].m_last_values[0] = 0;
		m_meters[ii].m_last_values[1]=0;
		m_meters[ii].m_last_values[2]=0;
		m_meters[ii].m_last_values[3]=0;
		m_meters[ii].m_firstTime = true;

		std::vector<std::vector<std::string> > result;
		std::vector<std::string> results;
		/*
		sprintf(szTmp,"SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%d' AND Unit=0 AND Type=%d AND SubType=%d)",m_HwdID, ii+1, pTypeENERGY, sTypeELEC2);
		result=m_sql.query(szTmp);
		if (result.size()==1)
		{
			StringSplit(result[0][0],";",results);
			if (results.size()==2)
			{
				m_meters[ii].m_counter_start=atof(results[1].c_str())/1000.0;
			}
		}
		*/

		std::stringstream szQuery;
		szQuery << "SELECT sValue FROM DeviceStatus WHERE (HardwareID=" << m_HwdID << " AND DeviceID=" << (ii+1) << ")";
		result = m_sql.query(szQuery.str());
		if (result.size() == 1)
		{
			StringSplit(result[0][0], ";", results);
			m_meters[ii].m_firstTime = false;
			if (results.size() == 1)
			{
				m_meters[ii].m_counter_start = atof(results[0].c_str()) / 1000.0;
			}
			else if (results.size() == 2)
			{
				m_meters[ii].m_counter_start = atof(results[1].c_str()) / 1000.0;
			}
		}
	}
}

void S0MeterBase::ParseData(const unsigned char *pData, int Len)
{
	int ii=0;
	while (ii<Len)
	{
		const unsigned char c = pData[ii];
		if(c == 0x0d)
		{
			ii++;
			continue;
		}

		if(c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

void S0MeterBase::SendMeter(unsigned char ID, double musage, double mtotal)
{
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	int meterype=m_meters[ID-1].m_type;

	if (meterype==MTYPE_ENERGY)
	{
		tsen.ENERGY.packettype=pTypeENERGY;
		tsen.ENERGY.subtype=sTypeELEC2;
		tsen.ENERGY.packetlength=sizeof(tsen.ENERGY)-1;
		tsen.ENERGY.id1=0;
		tsen.ENERGY.id2=ID;
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

	}
	else if (meterype==MTYPE_GAS)
	{
		P1Gas	m_p1gas;
		m_p1gas.len=sizeof(P1Gas)-1;
		m_p1gas.type=pTypeP1Gas;
		m_p1gas.subtype=sTypeP1Gas;
		m_p1gas.gasusage=(unsigned long)(mtotal*1000.0);
		m_p1gas.ID = ID;
		sDecodeRXMessage(this, (const unsigned char *)&m_p1gas);//decode message
	}
	else
	{
		//show as a counter, user needs to change meter type later
		//Counter
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
		tsen.RFXMETER.packettype=pTypeRFXMeter;
		tsen.RFXMETER.subtype=sTypeRFXMeterCount;
		tsen.RFXMETER.rssi=12;
		tsen.RFXMETER.id1=0;
		tsen.RFXMETER.id2=ID;

		unsigned long counterA=(unsigned long)(mtotal*1000.0);

		tsen.RFXMETER.count1 = (BYTE)((counterA & 0xFF000000) >> 24);
		tsen.RFXMETER.count2 = (BYTE)((counterA & 0x00FF0000) >> 16);
		tsen.RFXMETER.count3 = (BYTE)((counterA & 0x0000FF00) >> 8);
		tsen.RFXMETER.count4 = (BYTE)(counterA & 0x000000FF);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);//decode message
	}
}

void S0MeterBase::ParseLine()
{
	if (m_bufferpos<2)
		return;
	std::string sLine((char*)&m_buffer);

	std::vector<std::string> results;
	StringSplit(sLine,":",results);
	if (results.size()<1)
		return; //invalid data

	if (results[0][0]=='/') {
		//Meter restart
		//m_bMeterRestart=true;
		//Software Version

		std::string MeterID=results[0].substr(1);
		std::string SoftwareVersion=results[1];
		_log.Log(LOG_STATUS,"S0 Meter: ID: %s, Version: %s",MeterID.c_str(),SoftwareVersion.c_str());
		return;
	}
	if (results.size()<4)
	{
		_log.Log(LOG_ERROR,"S0 Meter: Invalid Data received! %s",sLine.c_str());
		return;
	}
	int totmeters=(results.size()-4)/3;
	if (totmeters>max_s0_meters)
		totmeters=max_s0_meters;
	//ID:0001:I:99:M1:123:456:M2:234:567 = ID(1)/Pulse Interval(3)/M1Actual(5)/M1Total(7)/M2Actual(8)/M2Total(9)
	//std::string MeterID=results[1];
	double s0_pulse_interval=atof(results[3].c_str());

	int roffset = 4;
	if (results[0] == "ID")
	{
		for (int ii = 0; ii < totmeters; ii++)
		{
			m_meters[ii].m_PacketsSinceLastPulseChange++;

			double s0_pulse = atof(results[roffset + 1].c_str());

			unsigned long LastTotalPulses = m_meters[ii].total_pulses;
			m_meters[ii].total_pulses = (unsigned long)atol(results[roffset + 2].c_str());
			if (m_meters[ii].total_pulses < LastTotalPulses)
			{
				//counter has looped
				m_meters[ii].m_counter_start += (LastTotalPulses + m_meters[ii].total_pulses);
				m_meters[ii].first_total_pulses_received = m_meters[ii].total_pulses;
			}

			if (s0_pulse != 0)
			{
				double pph = ((double)m_meters[ii].m_pulse_per_unit) / 1000; // Pulses per (watt) hour
				double ActualUsage = ((3600.0 / double(m_meters[ii].m_PacketsSinceLastPulseChange*s0_pulse_interval) / pph) * s0_pulse);
				m_meters[ii].m_PacketsSinceLastPulseChange = 0;

				m_meters[ii].m_last_values[m_meters[ii].m_value_buffer_write_pos] = ActualUsage;
				m_meters[ii].m_value_buffer_write_pos = (m_meters[ii].m_value_buffer_write_pos + 1) % 5;

				if (m_meters[ii].m_value_buffer_total < 5)
					m_meters[ii].m_value_buffer_total++;

				//Calculate Average
				double vTotal = 0;
				for (int iBuf = 0; iBuf < m_meters[ii].m_value_buffer_total; iBuf++)
					vTotal += m_meters[ii].m_last_values[iBuf];
				m_meters[ii].m_CurrentUsage = vTotal / double(m_meters[ii].m_value_buffer_total);
#ifdef _DEBUG
				_log.Log(LOG_STATUS, "S0 Meter: M%d, Watt: %.3f", ii + 1, m_meters[ii].m_CurrentUsage);
#endif
			}
			else
			{
				if (m_meters[ii].m_PacketsSinceLastPulseChange > 5 * 6)
				{
					//No pulses received for a long time, consider no usage
					m_meters[ii].m_PacketsSinceLastPulseChange = 0;
					m_meters[ii].m_last_values[0] = 0;
					m_meters[ii].m_value_buffer_total = 0;
					m_meters[ii].m_value_buffer_write_pos = 0;
					m_meters[ii].m_CurrentUsage = 0;
				}
			}

			if ((m_meters[ii].total_pulses != 0) || (m_meters[ii].m_firstTime))
			{
				m_meters[ii].m_firstTime = false;
				if (m_meters[ii].first_total_pulses_received == 0)
					m_meters[ii].first_total_pulses_received = m_meters[ii].total_pulses;

				double counter_value = m_meters[ii].m_counter_start + (((double)(m_meters[ii].total_pulses - m_meters[ii].first_total_pulses_received) / ((double)m_meters[ii].m_pulse_per_unit)));
				m_meters[ii].m_current_counter = counter_value;
				SendMeter(ii + 1, m_meters[ii].m_CurrentUsage / 1000.0f, counter_value);
			}

			roffset += 3;
		}
	}
	else if(results[0] == "EID")
	{
		roffset = 2;
		for (int ii = 0; ii < totmeters; ii++)
		{
			double s0_counter = atof(results[roffset + 1].c_str());
			if (s0_counter != 0)
			{
				if (m_meters[ii].m_firstTime)
				{
					m_meters[ii].m_current_counter = s0_counter;
					m_meters[ii].m_firstTime = false;
				}
				if (s0_counter < m_meters[ii].m_current_counter)
				{
					//counter has looped
					m_meters[ii].m_counter_start += m_meters[ii].m_current_counter;
				}
				m_meters[ii].m_current_counter = s0_counter;
				m_meters[ii].m_CurrentUsage = atof(results[roffset + 2].c_str());

				double counter_value = m_meters[ii].m_counter_start + s0_counter;
				SendMeter(ii + 1, m_meters[ii].m_CurrentUsage / 1000.0f, m_meters[ii].m_current_counter);
			}

			roffset += 3;
		}
	}
}

