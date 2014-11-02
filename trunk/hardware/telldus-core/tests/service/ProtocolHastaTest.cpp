#include "ProtocolHastaTest.h"
#include "service/ProtocolHasta.h"

class ProtocolHastaTest::PrivateData {
public:
	ProtocolHasta *protocol;
};

void ProtocolHastaTest :: setUp (void) {
	d = new PrivateData;
	d->protocol = new ProtocolHasta();
}

void ProtocolHastaTest :: tearDown (void) {
	delete d->protocol;
	delete d;
}

void ProtocolHastaTest :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Hasta Version 1 26380 1 DOWN",
		std::string("class:command;protocol:hasta;model:selflearning;house:26380;unit:1;method:down;"),
		d->protocol->decodeData(ControllerMessage("protocol:hasta;model:selflearning;data:0xC671100;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Hasta Version 1 26380 1 UP",
		std::string("class:command;protocol:hasta;model:selflearning;house:26380;unit:1;method:up;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:selflearning;data:0xC670100;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Hasta Version 2 19337 15 DOWN",
		std::string("class:command;protocol:hasta;model:selflearningv2;house:19337;unit:15;method:down;"),
		d->protocol->decodeData(ControllerMessage("protocol:hasta;model:selflearningv2;data:0x4B891F01;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Hasta Version 2 19337 15 UP",
		std::string("class:command;protocol:hasta;model:selflearningv2;house:19337;unit:15;method:up;"),
		d->protocol->decodeData(ControllerMessage("protocol:hasta;model:selflearningv2;data:0x4B89CF01;"))
	);
}
