#include "ProtocolEverflourishTest.h"
#include "service/ProtocolEverflourish.h"

class ProtocolEverflourishTest::PrivateData {
public:
	ProtocolEverflourish *protocol;
};

void ProtocolEverflourishTest :: setUp (void) {
	d = new PrivateData;
	d->protocol = new ProtocolEverflourish();
}

void ProtocolEverflourishTest :: tearDown (void) {
	delete d->protocol;
	delete d;
}

void ProtocolEverflourishTest :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Everflourish 4242:3 ON",
		std::string("class:command;protocol:everflourish;model:selflearning;house:4242;unit:3;method:turnon;"),
		d->protocol->decodeData(ControllerMessage("protocol:everflourish;data:0x424A6F;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Everflourish 5353:4 OFF",
		std::string("class:command;protocol:everflourish;model:selflearning;house:5353;unit:4;method:turnoff;"),
		d->protocol->decodeData(ControllerMessage("protocol:everflourish;data:0x53A7E0;"))
	);
}
