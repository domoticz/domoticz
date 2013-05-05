#include "stdafx.h"
#include "SMTPClient.h"
#if defined WIN32
#include "../curl/curl.h"
#else
#include <curl/curl.h>
#endif

#include "../main/Logger.h"
#include "../webserver/Base64.h"

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

bool SMTPClient::SendEmail(
	const std::string MailFrom,
	const std::string MailTo,
	const std::string MailServer,
	const int MailPort,
	const std::string MailUsername,
	const std::string MailPassword,
	const std::string MailSubject,
	const std::string MailBody,
	const bool bIsHTML)
{
	CURLcode ret;
	CURL *curl;
	struct curl_slist *slist1;

	smtp_upload_status smtp_ctx;
	smtp_ctx.bytes_read=0;

	std::string sFrom="<"+MailFrom+">";
	std::string sTo="<"+MailTo+">";

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, sTo.c_str());

	std::string szURL="smtp://"+MailServer+"/domoticz";

	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)177);
	curl_easy_setopt(curl, CURLOPT_URL, szURL.c_str());
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	if (MailUsername!="")
	{
		//std::string szUserPassword=MailUsername+":"+MailPassword;
		//curl_easy_setopt(curl, CURLOPT_USERPWD, szUserPassword.c_str());
		curl_easy_setopt(curl, CURLOPT_USERNAME, MailUsername.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, MailPassword.c_str());
	}
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "domoticz/7.26.0");
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);//CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_SSLVERSION, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);

	//curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sFrom.c_str());
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, slist1);

	//Create our full message
	const std::string boundary("bounds=_NextP_0056wi_0_8_ty789432_tp");
	std::string szMessage="";
	///////////////////////////////////////////////////////////////////////////
	// add the current time.
	// format is
	//     Date: 05 Jan 93 21:22:07
	//     Date: 05 Jan 93 21:22:07 -0500
	//     Date: 27 Oct 81 15:01:01 PST        (RFC 821 example)
	time_t t;
	time(&t);
	char timestring[128] = "";
	if(strftime(timestring, 127, "Date: %d %b %y %H:%M:%S %Z", localtime(&t))) { // got the date
		szMessage = timestring;
		szMessage += "\n";
	}
	szMessage+=
		"To: " + sTo + "\n" +
		"From: " + sFrom + "(Domoticz)\n" +
		"Subject: " + MailSubject + "\n";
		//"Content-Type: text/html; charset=\"us-ascii\"\n" +
		//"Content-Transfer-Encoding: quoted-printable\n" +
	if (bIsHTML)
	{
		szMessage+=
			"Mime-version: 1.0\n"
			"Content-Type: multipart/mixed;\n"
			"\tboundary=\"" + boundary + "\"\n";

		szMessage+= "This is a MIME encapsulated message\n\n";
		szMessage+= "--" + boundary + "\n";

		const std::string innerboundary("inner_jfd_0078hj_0_8_part_tp");
		szMessage+=
			"Content-Type: multipart/alternative;\n"
			"\tboundary=\"" + innerboundary + "\"\n";

		// need the inner boundary starter.
		szMessage += "\n\n--" + innerboundary + "\n";

		// plain text message first.
		szMessage +=
			"Content-type: text/plain; charset=iso-8859-1\n"
			"Content-transfer-encoding: 7BIT\n\n";

		szMessage += "\n\n--" + innerboundary + "\n";
		///////////////////////////////////
		// Add html message here!
		szMessage +=
			"Content-type: text/html; charset=UTF-8\n"
			"Content-Transfer-Encoding: base64\n\n";
		std::string szHTML=base64_encode((const unsigned char*)MailBody.c_str(),MailBody.size());
		szMessage +=szHTML;
		szMessage+= "\n\n--" + innerboundary + "--\n";
		szMessage+= "\n--" + boundary + "--\n";
	}
	else
	{
		//Plain Text
		szMessage+=MailBody;
	}
	// end the data in the message.
	szMessage+= "\n";

	smtp_ctx.pDataBytes=new char[szMessage.size()];
	if (smtp_ctx.pDataBytes==NULL)
	{
		_log.Log(LOG_ERROR,"SMTP Mailer: Out of Memory!");

		curl_easy_cleanup(curl);
		curl_slist_free_all(slist1);
		return false;
	}
	smtp_ctx.sDataLength=szMessage.size();
	memcpy(smtp_ctx.pDataBytes,szMessage.c_str(),smtp_ctx.sDataLength);

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_payload_reader);
	curl_easy_setopt(curl, CURLOPT_READDATA, &smtp_ctx);

	ret = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_slist_free_all(slist1);
	delete [] smtp_ctx.pDataBytes;

	if (ret!=CURLE_OK)
	{
		_log.Log(LOG_ERROR,"SMTP Mailer: Error sending Email to: %s !",MailTo.c_str());
		return false;
	}
	return true;
}

