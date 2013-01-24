
#if !defined( mynetwork_h )
#define mynetwork_h

#include <vector>

//  I_HTTPRequest will run until it's received all available data 
//  from the query. Reading will return 0 bytes when at the 
//  end, even if the query isn't yet complete. Test for completeness 
//  with complete(). 
class I_HTTPRequest {
  public:
	  struct _tHTTPVecBuffer {
		  unsigned char *pData;
		  unsigned short usLength;
	  } HTTPVecBuffer;
    virtual void dispose() = 0;
    virtual bool complete() = 0;
    virtual int read( void * ptr, size_t data ) = 0;
	virtual bool readDataInVecBuffer()=0;
	virtual bool getBuffer(unsigned char *&pDest, unsigned long &ulLength)=0;
	std::vector<_tHTTPVecBuffer> m_buffers;
};

//  This request will NOT deal with user names and passwords. You 
//  have been warned!
I_HTTPRequest * NewHTTPRequest( char const * url );

#endif

