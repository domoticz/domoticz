#pragma once
#include <iosfwd>
#include <vector>

class HTTPClient
{
public:
	//GET functions
	static bool GET(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned = false);
	static bool GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned = false);
	static bool GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const int TimeOut = -1);

	static bool GETBinaryToFile(const std::string &url, const std::string &outputfile);

	static bool GETSingleLine(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned = false);
	static bool GETBinarySingleLine(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const int TimeOut = -1);

	//POST functions, postdata looks like: "name=john&age=123&country=this"
	static bool POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bFollowRedirect=true, const bool bIgnoreNoDataReturned = false);
	static bool POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const bool bFollowRedirect = true);

	//PUT functions, postdata looks like: "name=john&age=123&country=this"
	static bool PUT(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned = false);
	static bool PUTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response);

	//DELETE functions, postdata looks like: "name=john&age=123&country=this"
	static bool Delete(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned = false);
	static bool DeleteBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response);

	//Cleanup function, should be called before application closed
	static void Cleanup();

	//Configuration functions
	static void SetConnectionTimeout(const long timeout);
	static void SetTimeout(const long timeout);
	static void SetUserAgent(const std::string &useragent);
	static void SetSecurityOptions(const bool verifypeer, const bool verifyhost);
private:
	static void SetGlobalOptions(void *curlobj);
	static bool CheckIfGlobalInitDone();
	static void LogError(void *curlobj);
	//our static variables
	static bool m_bCurlGlobalInitialized;
	static bool m_bVerifyHost;
	static bool m_bVerifyPeer;
	static long m_iConnectionTimeout;
	static long m_iTimeout;
	static std::string m_sUserAgent;
};

