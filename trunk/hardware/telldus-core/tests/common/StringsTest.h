#ifndef STRINGSTEST_H
#define STRINGSTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class StringsTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (StringsTest);
	CPPUNIT_TEST (formatfTest);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp (void);
	void tearDown (void);

protected:
	void formatfTest(void);
};

#endif //STRINGSTEST_H
