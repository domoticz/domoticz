#pragma once

class HTTPClient
{
	// give MainWorker acces to the protected Cleanup() function
	friend class MainWorker;

      public:
	enum _eHTTPmethod
	{
		HTTP_METHOD_GET,
		HTTP_METHOD_POST,
		HTTP_METHOD_PUT,
		HTTP_METHOD_DELETE,
		HTTP_METHOD_PATCH
	};

      protected:
	// Cleanup function, should be called before application closed
	static void Cleanup();

      public:
	/************************************************************************
	 *									*
	 * Configuration functions						*
	 *									*
	 * CAUTION!								*
	 * Because these settings are global they may affect other classes	*
	 * that use HTTPClient. Please add a debug line to your class if you	*
	 * access any of these functions.					*
	 *									*
	 ************************************************************************/

	static void SetConnectionTimeout(long timeout);
	static void SetTimeout(long timeout);
	static void SetUserAgent(const std::string &useragent);
	static void SetSecurityOptions(bool verifypeer, bool verifyhost);

	/************************************************************************
	 *									*
	 * simple access methods						*
	 *   - use for unprotected sites					*
	 *									*
	 ************************************************************************/

	static bool GET(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned = false, const bool bStartNewSession = false);
	static bool GETSingleLine(const std::string &url, std::string &response, bool bIgnoreNoDataReturned = false);
	static bool GETBinaryToFile(const std::string &url, const std::string &outputfile);

	/************************************************************************
	 *									*
	 * methods with optional headers					*
	 *   - use for sites that require additional header data		*
	 *     e.g. authorization token, charset definition, etc.		*
	 *									*
	 ************************************************************************/

	static bool GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned = false, const bool bStartNewSession = false);
	static bool POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, bool bFollowRedirect = true,
			 bool bIgnoreNoDataReturned = false);
	static bool PUT(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, bool bIgnoreNoDataReturned = false);
	static bool Delete(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, bool bIgnoreNoDataReturned = false);

	/************************************************************************
	 *									*
	 * methods with access to the return headers				*
	 *   - use if you require access to the HTTP return codes for		*
	 *     handling specific errors						*
	 *									*
	 ************************************************************************/

	static bool GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bIgnoreNoDataReturned = false, const bool bStartNewSession = false);
	static bool POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData,
			 bool bFollowRedirect = true, bool bIgnoreNoDataReturned = false);
	static bool PUT(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData,
			bool bIgnoreNoDataReturned = false);
	static bool Delete(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData,
			   bool bIgnoreNoDataReturned = false);
	static bool Patch(const std::string& url, const std::string& putdata, const std::vector<std::string>& ExtraHeaders, std::string& response, std::vector<std::string>& vHeaderData,
		bool bIgnoreNoDataReturned = false);

	/************************************************************************
	 *									*
	 * binary methods without access to return header data			*
	 *   - use if you require access to the return content even if there	*
	 *     was an error							*
	 *									*
	 ************************************************************************/

	static bool GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const long TimeOut = -1, const bool bStartNewSession = false);
	static bool GETBinarySingleLine(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, long TimeOut = -1);
	static bool POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, bool bFollowRedirect = true,
			       long TimeOut = -1);
	static bool PUTBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, long TimeOut = -1);
	static bool DeleteBinary(const std::string& url, const std::string& putdata, const std::vector<std::string>& ExtraHeaders, std::vector<unsigned char>& response, long TimeOut = -1);
	static bool PatchBinary(const std::string& url, const std::string& putdata, const std::vector<std::string>& ExtraHeaders, std::vector<unsigned char>& response, long TimeOut = -1);

	/************************************************************************
	 *									*
	 * binary methods with access to return header data			*
	 *									*
	 ************************************************************************/

	static bool GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, std::vector<std::string> &vHeaderData, const long TimeOut = -1, const bool bStartNewSession = false);
	static bool POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response,
			       std::vector<std::string> &vHeaderData, bool bFollowRedirect = true, long TimeOut = -1);
	static bool PUTBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response,
			      std::vector<std::string> &vHeaderData, long TimeOut = -1);
	static bool DeleteBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response,
				 std::vector<std::string> &vHeaderData, long TimeOut = -1);
	static bool PatchBinary(const std::string& url, const std::string& putdata, const std::vector<std::string>& ExtraHeaders, std::vector<unsigned char>& response,
		std::vector<std::string>& vHeaderData, long TimeOut = -1);

      private:
	static void SetGlobalOptions(void *curlobj);
	static bool CheckIfGlobalInitDone();
	static void LogError(long response_code);

      private:
	static bool m_bCurlGlobalInitialized;
	static bool m_bVerifyHost;
	static bool m_bVerifyPeer;
	static long m_iConnectionTimeout;
	static long m_iTimeout;
	static std::string m_sUserAgent;
};
