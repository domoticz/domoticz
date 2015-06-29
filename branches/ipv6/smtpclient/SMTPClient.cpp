#include "stdafx.h"
#include "SMTPClient.h"
#include <curl/curl.h>
#include "../main/Helper.h"
#include <sstream>

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
	if (m_Recipients.size()==0)
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

	sstr << "smtp://" << m_Server << ":" << m_Port << "/domoticz";
	std::string szURL=sstr.str();//"smtp://"+MailServer+"/domoticz";

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

const std::string SMTPClient::MakeMessage()
{
	std::string ret;

	int ii;
	std::vector<std::string>::const_iterator itt;

	//From
	ret =	"From: " + m_From + "\r\n" +
			"Reply-To: " + m_From + "\r\n";

	//To (first one, rest in BCC)
	ii=0;
	for (itt=m_Recipients.begin(); itt!=m_Recipients.end(); ++itt)
	{
		if (ii==0)
		{
			ret += "To: " + *itt;
			ret+="\r\n";
		}
		else {
			ret += "Cc: " + *itt;
			if (itt+1<m_Recipients.end())
				ret+=", ";
			ret+="\r\n";
		}
		ii++;
	}

	const std::string boundary("bounds=_NextP_0056wi_0_8_ty789432_tp");
	bool MIME=(m_Attachments.size() || m_HTMLBody.size());

	if(MIME)
	{	
		// we have attachments (or HTML mail)
		// use MIME 1.0
		ret+="MIME-Version: 1.0\r\n"
			"Content-Type: multipart/mixed;\r\n"
			"\tboundary=\"" + boundary + "\"\r\n";
	}

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

	if (strftime(timestring, 127, "Date: %a, %d %b %Y %H:%M:%S %Z", &timeinfo)) { // got the date
		ret += timestring;
		ret += "\r\n";
	}

	///////////////////////////////////////////////////////////////////////////
	// add the subject
	ret+= "Subject: " + m_Subject + "\r\n\r\n";

	///////////////////////////////////////////////////////////////////////////
	//
	// everything else added is the body of the email message.
	//
	///////////////////////////////////////////////////////////////////////////

	if(MIME) 
	{
		ret += "This is a MIME encapsulated message\r\n\r\n";
		ret += "--" + boundary + "\r\n";
		if(!m_HTMLBody.size()) {
			// plain text message first.
			ret +=	"Content-type: text/plain; charset=iso-8859-1\r\n"
					"Content-transfer-encoding: 7BIT\r\n\r\n";

			ret += m_PlainBody;
			ret += "\r\n\r\n--" + boundary + "\r\n";
		}
		else {
			// make it multipart/alternative as we have html
			const std::string innerboundary("inner_jfd_0078hj_0_8_part_tp");
			ret +=	"Content-Type: multipart/alternative;\r\n"
					"\tboundary=\"" + innerboundary + "\"\r\n";

			// need the inner boundary starter.
			ret += "\r\n\r\n--" + innerboundary + "\r\n";

			// plain text message first.
			ret += "Content-type: text/plain; charset=iso-8859-1\r\n"
					 "Content-transfer-encoding: 7BIT\r\n\r\n";
			if (m_PlainBody.size())
			{
				ret+=m_PlainBody;
			}
			ret += "\r\n\r\n--" + innerboundary + "\r\n";
			///////////////////////////////////
			// Add html message here!
			ret +=	"Content-type: text/html; charset=UTF-8\r\n"
					"Content-Transfer-Encoding: base64\r\n\r\n";

			std::string szHTML=base64_encode((const unsigned char*)m_HTMLBody.c_str(),m_HTMLBody.size());
			ret +=szHTML;
			ret += "\r\n\r\n--" + innerboundary + "--\r\n";

			// end the boundaries if there are no attachments
			if(!m_Attachments.size())
				ret += "\r\n--" + boundary + "--\r\n";
			else
				ret += "\r\n--" + boundary + "\r\n";
			///////////////////////////////////
		}

		// now add each attachment.
		std::vector<std::pair<std::string, std::string> >::const_iterator it1;
		for(it1 = m_Attachments.begin(); it1!=m_Attachments.end(); ++ it1)
		{
			if(it1->second.length() > 3)
			{ // long enough for an extension
				std::string typ(it1->second.substr(it1->second.length()-4, 4));
				if(typ == ".gif") { // gif format presumably
					ret+= "Content-Type: image/gif;\r\n";
				}
				else if(typ == ".jpg" || typ == "jpeg") { // j-peg format presumably
					ret+= "Content-Type: image/jpg;\r\n";
				}
				else if(typ == ".txt") { // text format presumably
					ret+= "Content-Type: plain/txt;\r\n";
				}
				else if(typ == ".bmp") { // windows bitmap format presumably
					ret+= "Content-Type: image/bmp;\r\n";
				}
				else if(typ == ".htm" || typ == "html") { // hypertext format presumably
					ret+= "Content-Type: plain/htm;\r\n";
				}
				else if(typ == ".png") { // portable network graphic format presumably
					ret+= "Content-Type: image/png;\r\n";
				}
				else if(typ == ".exe") { // application
					ret+= "Content-Type: application/X-exectype-1;\r\n";
				}
				else { // add other types
					// everything else
					ret+= "Content-Type: application/X-other-1;\r\n";
				}
			}
			else {
				// default to don't know
				ret+= "Content-Type: application/X-other-1;\r\n";
			}

			ret+= "\tname=\"" + it1->second + "\"\r\n";
			ret+= "Content-Transfer-Encoding: base64\r\n";
			ret+= "Content-Disposition: attachment; filename=\"" + it1->second + "\"\r\n\r\n";

			ret.insert(ret.end(), it1->first.begin(), it1->first.end());

			// terminate the message with the boundary + "--"
			if((it1 + 1) == m_Attachments.end())
				ret += "\r\n\r\n--" + boundary + "--\r\n";
			else
				ret += "\r\n\r\n--" + boundary + "\r\n";
		}
	}
	else // just a plain text message only
		ret+=m_PlainBody;

	// end the data in the message.
	ret += "\r\n.\r\n";

	return ret;
}

