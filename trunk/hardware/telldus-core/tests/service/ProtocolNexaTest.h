#ifndef PROTOCOLNEXATEST_H
#define PROTOCOLNEXATEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ProtocolNexaTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ProtocolNexaTest);
	CPPUNIT_TEST (decodeDataTest);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp (void);
	void tearDown (void);

protected:
	void decodeDataTest(void);

private:
	class PrivateData;
	PrivateData *d;
};

#endif //PROTOCOLNEXATEST_H
