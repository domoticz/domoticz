#ifndef PROTOCOLX10TEST_H
#define PROTOCOLX10TEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ProtocolX10Test : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ProtocolX10Test);
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

#endif //PROTOCOLX10TEST_H
