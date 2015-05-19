#include "ProtocolOregonTest.h"
#include "service/ProtocolOregon.h"

class ProtocolOregonTest::PrivateData {
public:
};

void ProtocolOregonTest :: setUp (void) {
	d = new PrivateData;
}

void ProtocolOregonTest :: tearDown (void) {
	delete d;
}

void ProtocolOregonTest :: decodeDataTest (void) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 77.3",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:77.3;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177307700E4;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 74.7",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:74.7;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177707410A4;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 77.7",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:77.7;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:217770774054;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 66.5",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:66.5;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177506600E4;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 122.5",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:122.5;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177502291A3;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 120.1",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:120.1;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177102031B3;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 120.6",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:120.6;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:217760208193;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 23, temp: 202.7",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:23;temp:202.7;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:2177702A2D3;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: 202.7",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:202.7;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:21777002A2D3;"))
	);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Oregon, id: 119, temp: -23.1",
		std::string("class:sensor;protocol:oregon;model:EA4C;id:119;temp:-23.1;"),
		ProtocolOregon::decodeData(ControllerMessage("class:sensor;protocol:oregon;model:0xEA4C;data:21771023D8B3;"))
	);
}
