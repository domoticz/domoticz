//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_CONTROLLER_H_
#define TELLDUS_CORE_SERVICE_CONTROLLER_H_

#include <string>
#include "common/Event.h"

class ControllerEventData : public TelldusCore::EventDataBase {
public:
	std::string msg;
	int controllerId;
};

class Controller {
public:
	virtual ~Controller();

	virtual int firmwareVersion() const;
	virtual int send( const std::string &message ) = 0;
	virtual int reset() = 0;

protected:
	Controller(int id, TelldusCore::EventRef event, TelldusCore::EventRef updateEvent);
	void publishData(const std::string &data) const;
	void decodePublishData(const std::string &data) const;
	void setFirmwareVersion(int version);

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_CONTROLLER_H_
