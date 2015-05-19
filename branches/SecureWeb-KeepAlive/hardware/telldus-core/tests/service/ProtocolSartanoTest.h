#ifndef PROTOCOLSARTANOTEST_H
#define PROTOCOLSARTANOTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ProtocolSartanoTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ProtocolSartanoTest);
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

#endif //PROTOCOLSARTANOTEST_H
