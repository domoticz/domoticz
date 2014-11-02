#ifndef PROTOCOLOREGONTEST_H
#define PROTOCOLOREGONTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ProtocolOregonTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ProtocolOregonTest);
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

#endif // PROTOCOLOREGONTEST_H
