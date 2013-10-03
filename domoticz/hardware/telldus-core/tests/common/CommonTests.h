#ifndef COMMONTESTS_H
#define COMMONTESTS_H

#include "StringsTest.h"

namespace CommonTests {
	inline void setup() {
		CPPUNIT_TEST_SUITE_REGISTRATION (StringsTest);
	}
}
#endif // COMMONTESTS_H
