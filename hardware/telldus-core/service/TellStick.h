//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_TELLSTICK_H_
#define TELLDUS_CORE_SERVICE_TELLSTICK_H_

#include <list>
#include <string>
#include "service/Controller.h"
#include "common/Thread.h"

class TellStickDescriptor {
public:
	std::string serial;
	int vid, pid;
};

class TellStick : public Controller, public TelldusCore::Thread {
public:
	TellStick(int controllerId, TelldusCore::EventRef event, TelldusCore::EventRef updateEvent, const TellStickDescriptor &d);
	virtual ~TellStick();

	virtual int pid() const;
	virtual int vid() const;
	virtual std::string serial() const;

	bool isOpen() const;
	bool isSameAsDescriptor(const TellStickDescriptor &d) const;
	virtual int reset();
	virtual int send( const std::string &message );
	bool stillConnected() const;

	static std::list<TellStickDescriptor> findAll();

	static std::string createTPacket( const std::string & );
	static std::string convertSToT(  unsigned char t0, unsigned char t1, unsigned char t2, unsigned char t3, const std::string &data );

protected:
	void processData( const std::string &data );
	void run();
	void setBaud( int baud );
	void stop();

private:
	static std::list<TellStickDescriptor> findAllByVIDPID( int vid, int pid );

	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_TELLSTICK_H_
