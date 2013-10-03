#ifndef SERVICETESTS_H
#define SERVICETESTS_H

#include "ProtocolEverflourishTest.h"
#include "ProtocolHastaTest.h"
#include "ProtocolNexaTest.h"
#include "ProtocolOregonTest.h"
#include "ProtocolSartanoTest.h"
#include "ProtocolX10Test.h"

namespace ServiceTests {
	inline void setup() {
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolEverflourishTest);
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolHastaTest);
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolNexaTest);
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolOregonTest);
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolSartanoTest);
		CPPUNIT_TEST_SUITE_REGISTRATION (ProtocolX10Test);
	}
}
#endif // SERVICETESTS_H
