#include "ProtocolX10Test.h"
#include "service/ProtocolX10.h"

class ProtocolX10Test::PrivateData {
public:
	ProtocolX10 *protocol;
};

void ProtocolX10Test :: setUp (void) {
	d = new PrivateData;
	d->protocol = new ProtocolX10();
}

void ProtocolX10Test :: tearDown (void) {
	delete d->protocol;
	delete d;
}

void ProtocolX10Test :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"X10 A1 ON",
		std::string("class:command;protocol:x10;model:codeswitch;house:A;unit:1;method:turnon;"),
		d->protocol->decodeData(ControllerMessage("protocol:x10;data:0x609F00FF;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"X10 E11 OFF",
		std::string("class:command;protocol:x10;model:codeswitch;house:E;unit:11;method:turnoff;"),
		d->protocol->decodeData(ControllerMessage("protocol:x10;data:0x847B28D7;"))
	);
}
