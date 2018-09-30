#pragma once

#ifndef _STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F
#define _STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F

#include <future>

//borrowed and modified from Varun (https://thispointer.com/c11-how-to-stop-or-terminate-a-thread/)

class StoppableTask
{
public:
	template<typename R>
	bool is_ready(std::future<R> const& f)
	{
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}
	StoppableTask() :
		m_futureObj(m_exitSignal.get_future())
	{
	}
	StoppableTask(StoppableTask && obj) :
		m_exitSignal(std::move(obj.m_exitSignal)), m_futureObj(std::move(obj.m_futureObj))
	{
	}
	StoppableTask & operator=(StoppableTask && obj)
	{
		m_exitSignal = std::move(obj.m_exitSignal);
		m_futureObj = std::move(obj.m_futureObj);
		return *this;
	}
	//Checks if thread is requested to stop
	bool IsStopRequested(const int timeMS)
	{
		// checks if value in future object is available
		return (m_futureObj.wait_for(std::chrono::milliseconds(timeMS)) != std::future_status::timeout);
	}
	// Request the thread to stop by setting value in promise object
	void RequestStop()
	{
		if (!is_ready(m_futureObj))
			m_exitSignal.set_value();
	}
	void RequestStart()
	{
		if (is_ready(m_futureObj))
		{
			m_exitSignal = std::promise<void>();
			m_futureObj = m_exitSignal.get_future();
		}
	}
private:
	std::promise<void> m_exitSignal;
	std::future<void> m_futureObj;
};

#endif //_STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F
