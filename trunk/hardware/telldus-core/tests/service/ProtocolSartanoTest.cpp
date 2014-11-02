#include "ProtocolSartanoTest.h"
#include "service/ProtocolSartano.h"

class ProtocolSartanoTest::PrivateData {
public:
	ProtocolSartano *protocol;
};

void ProtocolSartanoTest :: setUp (void) {
	d = new PrivateData;
	d->protocol = new ProtocolSartano();
}

void ProtocolSartanoTest :: tearDown (void) {
	delete d->protocol;
	delete d;
}

void ProtocolSartanoTest :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Sartano 0101010101 ON",
		std::string("class:command;protocol:sartano;model:codeswitch;code:0101010101;method:turnon;"),
		d->protocol->decodeData(ControllerMessage("protocol:arctech;model:codeswitch;data:0x955;"))
	);
}
