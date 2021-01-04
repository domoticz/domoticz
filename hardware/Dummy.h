#pragma once

#include "DomoticzHardware.h"

class CDummy : public CDomoticzHardwareBase
{
      public:
	explicit CDummy(int ID);
	~CDummy() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
};
