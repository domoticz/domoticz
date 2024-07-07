#include "stdafx.h"
#include "lsignal.h"

namespace lsignal
{
	connection_data::connection_data()
	{

	}

	connection_data::~connection_data()
	{

	}

	connection_cleaner::connection_cleaner()
	{

	}

	connection_cleaner::~connection_cleaner()
	{

	}

	connection::connection()
	{

	}
	
	connection::connection(std::shared_ptr<connection_data>&& data)
		: _data(std::move(data))
	{
	}

	connection::~connection()
	{
	}

	bool connection::is_locked() const
	{
		return _data->locked;
	}

	void connection::set_lock(const bool lock)
	{
		_data->locked = lock;
	}

	void connection::disconnect()
	{
		if (_data)
		{
			//connection fully cleared after next signal call or signal delete
			_data->deleted = true;
			_data.reset();
		}
	}

	slot::slot()
	{
	}

	slot::~slot()
	{
		disconnect();
	}

	void slot::disconnect()
	{
		decltype(_cleaners) cleaners = _cleaners;

		for (auto iter = cleaners.cbegin(); iter != cleaners.cend(); ++iter)
		{
			const connection_cleaner& cleaner = *iter;
			cleaner.data->deleted = true;
		}

		_cleaners.clear();
	}
}//namespace lsignal