#include "ProtocolNexaTest.h"
#include "service/ProtocolNexa.h"

class ProtocolNexaTest::PrivateData {
public:
	ProtocolNexa *protocol;
};

void ProtocolNexaTest :: setUp (void) {
	d = new PrivateData;
	d->protocol = new ProtocolNexa();
}

void ProtocolNexaTest :: tearDown (void) {
	delete d->protocol;
	delete d;
}

void ProtocolNexaTest :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Arctech Codeswitch A1 ON",
		std::string("class:command;protocol:arctech;model:codeswitch;house:A;unit:1;method:turnon;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:codeswitch;data:0xE00;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Arctech Codeswitch A1 OFF",
		std::string("class:command;protocol:arctech;model:codeswitch;house:A;unit:1;method:turnoff;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:codeswitch;data:0x600;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Arctech Selflearning 1329110 1 ON",
		std::string("class:command;protocol:arctech;model:selflearning;house:1329110;unit:1;group:0;method:turnon;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:selflearning;data:0x511F590;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Arctech Selflearning 1329110 1 OFF",
		std::string("class:command;protocol:arctech;model:selflearning;house:1329110;unit:1;group:0;method:turnoff;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:selflearning;data:0x511F580;"))
	);
}
