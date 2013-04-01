#pragma once
#include <string>
#include <vector>

class HTTPClient
{
public:
	//GET functions
	static bool GET(const std::string url, std::string &response);
	static bool GETBinary(const std::string url, std::vector<unsigned char> &response);

	static bool GETBinaryToFile(const std::string url, const std::string outputfile);

	//POST functions, postdata looks like: "name=john&age=123&country=this"
	static bool POST(const std::string url, const std::string postdata, std::string &response);
	static bool POSTBinary(const std::string url, const std::string postdata, std::vector<unsigned char> &response);

	//Cleanup function, should be called before application closed
	static void Cleanup();

	//Configuration functions
	static void SetConnectionTimeout(const long timeout);
	static void SetTimeout(const long timeout);
	static void SetUserAgent(const std::string useragent);
private:
	static void SetGlobalOptions(void *curlobj);
	static bool CheckIfGlobalInitDone();
	//our static variables
	static bool	m_bCurlGlobalInitialized;
	static long	m_iConnectionTimeout;
	static long	m_iTimeout;
	static std::string m_sUserAgent;
};

