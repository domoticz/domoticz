#include "StringsTest.h"
#include "Strings.h"

void StringsTest :: setUp (void)
{
}

void StringsTest :: tearDown (void)
{
}

void StringsTest :: formatfTest (void) {
	CPPUNIT_ASSERT_EQUAL(std::string("42"), TelldusCore::formatf("%u", 42));
	CPPUNIT_ASSERT_EQUAL(std::string("2A"), TelldusCore::formatf("%X", 42));
	CPPUNIT_ASSERT_EQUAL(std::string("42"), TelldusCore::formatf("%s", "42"));
}
