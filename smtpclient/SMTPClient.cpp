#include "stdafx.h"
#include "SMTPClient.h"
#include <curl/curl.h>
#include "../main/Helper.h"
#include <string.h>

#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../webserver/Base64.h"

// Part of the Message Construction is taken from jwSMTP library
// http://johnwiggins.net
// smtplib@johnwiggins.net

struct smtp_upload_status {
	size_t bytes_read;
	char *pDataBytes;
	size_t sDataLength;
};

static size_t smtp_payload_reader(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct smtp_upload_status *upload_ctx = (struct smtp_upload_status *)userp;

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}
	size_t realsize = size * nmemb;
	if (realsize+upload_ctx->bytes_read>upload_ctx->sDataLength)
		realsize=upload_ctx->sDataLength-upload_ctx->bytes_read;
	if (realsize==0)
		return 0;
	memcpy(ptr,upload_ctx->pDataBytes+upload_ctx->bytes_read,realsize);
	upload_ctx->bytes_read+=realsize;
	return realsize;
}

SMTPClient::SMTPClient()
{
	m_Port=25;
}

SMTPClient::~SMTPClient()
{

}

void SMTPClient::SetFrom(const std::string &From)
{
	m_From="<"+From+">";
}

void SMTPClient::SetTo(const std::string &To)
{
	std::vector<std::string> results;
	StringSplit(To,";",results);

	std::vector<std::string>::const_iterator itt;
	for (itt=results.begin(); itt!=results.end(); ++itt)
	{
		std::string sTo="<"+(*itt)+">";
		m_Recipients.push_back(sTo);
	}
}

void SMTPClient::SetSubject(const std::string &Subject)
{
	m_Subject=Subject;
}

void SMTPClient::SetServer(const std::string &Server, const int Port)
{
	m_Server=Server;
	m_Port=Port;
}

void SMTPClient::SetCredentials(const std::string &Username, const std::string &Password)
{
	m_Username=Username;
	m_Password=Password;
}

void SMTPClient::AddAttachment(const std::string &adata, const std::string &atype)
{
	std::pair<std::string, std::string> tattachment;
	tattachment.first = adata;
	tattachment.second = atype;
	m_Attachments.push_back(tattachment);
}

void SMTPClient::SetPlainBody(const std::string &body)
{
	m_PlainBody=body;
	m_HTMLBody="";
}

void SMTPClient::SetHTMLBody(const std::string &body)
{
	m_HTMLBody=body;
	m_PlainBody="";
}

bool SMTPClient::SendEmail()
{
	if (m_From.size()==0)
		return false;
	if (m_Recipients.empty())
		return false;
	if (m_Server.size()==0)
		return false;

	const std::string rmessage=MakeMessage();

	CURLcode ret;
	struct curl_slist *slist1;

	smtp_upload_status smtp_ctx;
	smtp_ctx.bytes_read=0;

	slist1 = NULL;


	std::vector<std::string>::const_iterator itt;
	for (itt=m_Recipients.begin(); itt!=m_Recipients.end(); ++itt)
	{
		slist1 = curl_slist_append(slist1, (*itt).c_str());
	}

	std::stringstream sstr;
	if (m_Port != 465)
		sstr << "smtp://";
	else
		sstr << "smtps://"; //SSL connection
	sstr << m_Server << ":" << m_Port;
	std::string szURL=sstr.str();//"smtp://"+MailServer;

	try
	{
		CURL *curl;
		curl = curl_easy_init();

		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)177);
		curl_easy_setopt(curl, CURLOPT_URL, szURL.c_str());
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		if (m_Username!="")
		{
			//std::string szUserPassword=MailUsername+":"+MailPassword;
			//curl_easy_setopt(curl, CURLOPT_USERPWD, szUserPassword.c_str());
			curl_easy_setopt(curl, CURLOPT_USERNAME, m_Username.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, m_Password.c_str());
		}
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "domoticz/7.26.0");
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);//CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSLVERSION, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

		//curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_From.c_str());
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, slist1);

		smtp_ctx.pDataBytes=new char[rmessage.size()];
		if (smtp_ctx.pDataBytes==NULL)
		{
			_log.Log(LOG_ERROR,"SMTP Mailer: Out of Memory!");

			curl_easy_cleanup(curl);
			curl_slist_free_all(slist1);
			return false;
		}
		smtp_ctx.sDataLength=rmessage.size();
		memcpy(smtp_ctx.pDataBytes,rmessage.c_str(),smtp_ctx.sDataLength);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_payload_reader);
		curl_easy_setopt(curl, CURLOPT_READDATA, &smtp_ctx);

		ret = curl_easy_perform(curl);

		curl_easy_cleanup(curl);
		curl_slist_free_all(slist1);
		delete [] smtp_ctx.pDataBytes;

		if (ret!=CURLE_OK)
		{
			_log.Log(LOG_ERROR,"SMTP Mailer: Error sending Email to: %s !",m_Recipients[0].c_str());
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR,"SMTP Mailer: Error sending Email to: %s !",m_Recipients[0].c_str());
		return false;
	}
	return true;
}

void MakeBoundry(char *pszBoundry)
{
	char* p = pszBoundry;
	while (*p) {
		if (*p == '0')
			*p = '0' + rand() % 10;
		p++;
	}
}

const std::string SMTPClient::MakeMessage()
{
	std::string ret;

	int ii;
	std::vector<std::string>::const_iterator itt;
	char szBoundary[40];
	char szBoundaryMixed[40];
	sprintf(szBoundary, "--------------000000000000000000000000");
	sprintf(szBoundaryMixed, "--------------000000000000000000000000");
	MakeBoundry(szBoundary);
	MakeBoundry(szBoundaryMixed);

	//From
	ret = "From: " + m_From + "\r\n";

	//To (first one, rest in Cc)
	ii = 0;
	for (itt = m_Recipients.begin(); itt != m_Recipients.end(); ++itt)
	{
		if (ii == 0)
			ret += "To: " + *itt;
		else {
			if (ii == 1)
			{
				ret += "\r\nCc: ";
			}
			ret += *itt;
			if (itt + 1 < m_Recipients.end())
				ret += ", ";
		}
		ii++;
	}
	ret += "\r\n";

	///////////////////////////////////////////////////////////////////////////
	// add the subject
	ret += "Subject: " + m_Subject + "\r\n";
	///////////////////////////////////////////////////////////////////////////
	// add the current time.
	// format is
	//     Date: 05 Jan 93 21:22:07
	//     Date: 05 Jan 93 21:22:07 -0500
	//     Date: 27 Oct 81 15:01:01 PST        (RFC 821 example)
	time_t t;
	time(&t);
	char timestring[128] = "";
	struct tm timeinfo;
	localtime_r(&t, &timeinfo);

	if (strftime(timestring, sizeof(timestring), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", &timeinfo)) {
		ret += timestring;
	}
#ifdef _WIN32
	//fallback method for Windows when %z (time zone offset) fails
	else if (strftime(timestring, sizeof(timestring), "Date: %a, %d %b %Y %H:%M:%S", &timeinfo)) {
		TIME_ZONE_INFORMATION tzinfo;
		if (GetTimeZoneInformation(&tzinfo) != TIME_ZONE_ID_INVALID)
			sprintf(timestring + strlen(timestring), " %c%02i%02i", (tzinfo.Bias > 0 ? '-' : '+'), (int)-tzinfo.Bias / 60, (int)-tzinfo.Bias % 60);
		ret += timestring;
		ret += "\r\n";
	}
#endif
	ret += "MIME-Version: 1.0\r\n";
	bool bHaveSendMimeFormat = false;
	if (!m_Attachments.empty())
	{
		// we have attachments
		ret += "Content-Type: multipart/mixed;\r\n"
			"\tboundary=\"" + std::string(szBoundaryMixed) + "\"\r\n\r\n";
		ret += "This is a multipart message in MIME format.\r\n";
		bHaveSendMimeFormat = true;
	}

	if ((!m_PlainBody.empty()) || (!m_HTMLBody.empty()))
	{
		ret += "Content-Type: multipart/alternative;\r\n"
			"\tboundary=\"" + std::string(szBoundary) + "\"\r\n\r\n";
		if (!bHaveSendMimeFormat)
			ret += "This is a multipart message in MIME format.\r\n";

		if (!m_PlainBody.empty()) {
			ret += "--" + std::string(szBoundary) + "\r\n";
			ret += "Content-type: text/plain; charset=utf-8; format=flowed\r\n"
				"Content-transfer-encoding: 8bit\r\n\r\n";
			ret += m_PlainBody;
			ret += "\r\n";
		}
		if (!m_HTMLBody.empty())
		{
			ret += "--" + std::string(szBoundary) + "\r\n";
			ret += "Content-type: text/html; charset=utf-8\r\n"
				"Content-Transfer-Encoding: 8bit\r\n\r\n";
			ret += m_HTMLBody;
			ret += "\r\n";
		}
		ret += "--" + std::string(szBoundary) + "--\r\n";
	}
	else
	{
		//no text
		ret += "--" + std::string(szBoundaryMixed) + "\r\n";
		ret += "Content-type: text/plain; charset=utf-8; format=flowed\r\n"
			"Content-transfer-encoding: 7bit\r\n\r\n";
		ret += "\r\n";
	}

	if (!m_Attachments.empty())
	{
		ret += "\r\n";
		// now add each attachment.
		std::vector<std::pair<std::string, std::string> >::const_iterator it1;
		for (it1 = m_Attachments.begin(); it1 != m_Attachments.end(); ++it1)
		{
			ret += "--" + std::string(szBoundaryMixed) + "\r\n";
			if (it1->second.length() > 3)
			{ // long enough for an extension
				std::string typ(it1->second.substr(it1->second.length() - 4, 4));
				if (typ == ".gif") { // gif format presumably
					ret += "Content-Type: image/gif;\r\n";
				}
				else if (typ == ".jpg" || typ == "jpeg") { // j-peg format presumably
					ret += "Content-Type: image/jpg;\r\n";
				}
				else if (typ == ".txt") { // text format presumably
					ret += "Content-Type: plain/txt;\r\n";
				}
				else if (typ == ".bmp") { // windows bitmap format presumably
					ret += "Content-Type: image/bmp;\r\n";
				}
				else if (typ == ".htm" || typ == "html") { // hypertext format presumably
					ret += "Content-Type: plain/htm;\r\n";
				}
				else if (typ == ".png") { // portable network graphic format presumably
					ret += "Content-Type: image/png;\r\n";
				}
				else if (typ == ".exe") { // application
					ret += "Content-Type: application/X-exectype-1;\r\n";
				}
				else { // add other types
					   // everything else
					ret += "Content-Type: application/X-other-1;\r\n";
				}
			}
			else {
				// default to don't know
				ret += "Content-Type: application/X-other-1;\r\n";
			}

			ret += "\tname=\"" + it1->second + "\"\r\n";
			ret += "Content-Transfer-Encoding: base64\r\n";
			ret += "Content-Disposition: attachment;\r\n filename=\"" + it1->second + "\"\r\n\r\n";

			ret.insert(ret.end(), it1->first.begin(), it1->first.end());
			ret += "\r\n";
		}
		ret += "--" + std::string(szBoundaryMixed) + "--\r\n";
	}

	// We do not have to end the message with "<CRLF>.<CRLF>" as libcurl does this for us, but ensure the last line is terminated
	// ret += "\r\n.\r\n";
	
	ret += "\r\n";

	return ret;
}

