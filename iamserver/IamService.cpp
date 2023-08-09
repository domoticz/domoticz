/*
 * IamService.cpp
 *
 *  Created on: 23 Januari 2023
 *      Author: kiddigital
 * 
 * It contains routines that are part of the WebServer class, but for sourcecode management
 * reasons separated out into its own file so it is easier to maintain the IAM related functions
 * of the WebServer. The definitions of the methods here are still in 'main/Webserver.h'
 *  
*/

#include "stdafx.h"
#include <iostream>
#include <json/json.h>
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../httpclient/UrlEncode.h"
#include "../main/WebServer.h"
#include "../webserver/Base64.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace http
{
	namespace server
	{

		void CWebServer::GetOauth2AuthCode(WebEmSession &session, const request &req, reply &rep)
		{
			bool bAuthenticated = false;
			bool bAuthorized = false;

			std::string code;
			std::string error = "unknown_error";

			std::string state = request::findValue(&req, "state");
			std::string redirect_uri = CURLEncode::URLDecode(request::findValue(&req, "redirect_uri"));
			std::string response_type = request::findValue(&req, "response_type");
			std::string client_id = request::findValue(&req, "client_id");
			std::string scope = request::findValue(&req, "scope");
			std::string code_challenge = request::findValue(&req, "code_challenge");
			std::string code_challenge_method = request::findValue(&req, "code_challenge_method");

			if (!redirect_uri.empty() && redirect_uri.substr(0,8) == "https://")	// Absolute and (TLS)safe redirect URI expected
			{
				if (req.method == "GET" || req.method == "POST")
				{
					if(!response_type.empty() && (response_type.compare("code") == 0 )) // || response_type.compare("token") == 0))
					{
						if(scope.find_first_of("openid") == std::string::npos)
						{
							_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Missing 'openid' in scope (%s)! Not an OpenID Connect request, maybe just OAuth2?", scope.c_str());
						}
						int iClient = -1;
						int iUser = -1;
						if (!client_id.empty())
						{
							iClient = FindClient(client_id.c_str());
							if (iClient != -1)
							{
								std::string Username;

								if(req.method != "POST")
								{	// So a GET request
									bAuthenticated = m_pWebEm->FindAuthenticatedUser(Username, req, rep);

									if (Username.empty())
									{
										_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: No Authenticated User, present Authentication Dialog!");
										PresentOauth2LoginDialog(rep, m_users[iClient].Username, "");
										m_failcount = 0;
										return;
									}
									iUser = FindUser(Username.c_str());
								}
								else
								{	// POST request, so maybe we have the data from the Login form
									std::string sConsent = request::findValue(&req, "consent");
									std::string sPWD = request::findValue(&req, "psw");
									std::string sTOTP = request::findValue(&req, "totp");
									Username = request::findValue(&req, "uname");
									iUser = FindUser(Username.c_str());
									bAuthenticated = (iUser != -1 ? (m_users[iUser].Password == GenerateMD5Hash(sPWD)) : false);
									if (!bAuthenticated)
									{
										m_failcount++;
										if (m_failcount < 3)
										{
											_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Validating User (%s) failed (%d)!", Username.c_str(), iUser);
											PresentOauth2LoginDialog(rep, m_users[iClient].Username, "Invalid Credentials!");
											return;
										}
										error = "User credentials do not match!";
									}
									else
									{
										// User/pass matches.. now check TOTP if required
										if (!m_users[iUser].Mfatoken.empty())
										{
											std::string sTotpKey = "";
											bAuthenticated = false;
											if(base32_decode(m_users[iUser].Mfatoken, sTotpKey))
											{
												if (VerifySHA1TOTP(sTOTP, sTotpKey))
												{
													bAuthenticated = true;
												}
												else
												{
													m_failcount++;
													if (m_failcount < 3)
													{
														_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Validating Time-based On-Time Passcode for User (%s) failed (%s)!", Username.c_str(), sTOTP.c_str());
														PresentOauth2LoginDialog(rep, m_users[iClient].Username, "Invalid TOTP code!");
														return;
													}
													error = "TOTP Verification for a user has failed!";
												}
											}
											else
											{
												error = "TOTP key is not valid base32 encoded!";
											}
										}
									}
								}

								if (iUser != -1 && bAuthenticated)
								{
									code = GenerateMD5Hash(base64_encode(GenerateUUID()));
									m_accesscodes[iUser].AuthCode = code;
									m_accesscodes[iUser].clientID = iClient;
									m_accesscodes[iUser].AuthTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
									m_accesscodes[iUser].ExpTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + (m_iamsettings.code_expire_seconds * 1000);
									m_accesscodes[iUser].RedirectUri = redirect_uri;
									m_accesscodes[iUser].Scope = scope;
									if (!(code_challenge.empty() || code_challenge_method.empty()) && code_challenge_method.compare("S256") == 0 )
									{
										m_accesscodes[iUser].CodeChallenge = code_challenge;
									}
									else
									{
										if (!code_challenge.empty())
										{
											_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: PKCE Code Challenge found, but challenge method (%s) not supported! Challenge ignored!", code_challenge_method.c_str());
										}
									}
									bAuthorized = true;
								}
								else
								{
									error = "Authentication for a user has failed!";
								}
							}
						}
						if (iUser == -1)
						{
							error = "unauthorized_client";
							_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Unauthorized/Unknown client_id (%s)!", client_id.c_str());
						}
					}
					else
					{
						error = "unsupported_response_type";
						_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Invalid/unsupported response_type (%s)!", response_type.c_str());
					}
				}
				else
				{
					error = "unsupported_request_method";
					_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Received invalid request method .%s.", req.method.c_str());
				}
			}
			else
			{
				error = "missing or wrong redirect_uri";
				_log.Debug(DEBUG_AUTH, "OAuth2 Auth Code: Wrong/Missing redirect_uri (%s)!", redirect_uri.c_str());
			}

			// Redirect the User back to origin using the redirect_uri
			std::stringstream result;
			if(redirect_uri.find("?") != std::string::npos)
				result << redirect_uri << "&";
			else
				result << redirect_uri << "?";

			if (!code.empty())
				result << "code=" << CURLEncode::URLEncode(code);
			else
				result << "error=" << CURLEncode::URLEncode(error);

			if (!state.empty())
				result << "&state=" << state;

			reply::add_header(&rep, "Location", result.str());
			rep.status = reply::moved_temporarily;
		}

		void CWebServer::PostOauth2AccessToken(WebEmSession &session, const request &req, reply &rep)
		{
			Json::Value root;
			std::string jwttoken;
			uint32_t exptime = m_iamsettings.token_expire_seconds;			// Token validity time (seconds)
			uint32_t refreshexptime = m_iamsettings.refresh_expire_seconds;	// Refresh token validity time (seconds)

			reply::add_header_content_type(&rep, "application/json;charset=UTF-8");
			rep.status = reply::bad_request;

			std::string state = request::findValue(&req, "state");
			std::string grant_type = request::findValue(&req, "grant_type");
			std::string client_id = request::findValue(&req, "client_id");
			std::string client_secret = request::findValue(&req, "client_secret");
			std::string auth_code = request::findValue(&req, "code");
			std::string scope = request::findValue(&req, "scope");
			std::string code_verifier = request::findValue(&req, "code_verifier");
			std::string redirect_uri = CURLEncode::URLDecode(request::findValue(&req, "redirect_uri"));

			if (!state.empty())
			{
				root["state"] = state;
			}

			if (req.method == "POST")
			{
				bool bValidGrantType = false;
				if(!grant_type.empty())
				{
					if (grant_type.compare("authorization_code") == 0 )
					{
						bValidGrantType = true;
						int iClient = -1;
						int iUser = -1;
						if (!client_id.empty())
						{
							iClient = FindClient(client_id.c_str());
							if (iClient != -1)
							{
								// Let's find the user for this client with the right auth_code, if any
								iUser = 0;
								for (const auto &ac : m_accesscodes)
								{
									if (ac.AuthCode.compare(auth_code.c_str()) == 0)
									{
										if (ac.clientID == iClient || (ac.clientID == -1 && iUser == iClient))
											break;
									}
									iUser++;
								}

								if ((iUser < static_cast<int>(m_accesscodes.size())) && (m_accesscodes[iUser].AuthCode.compare(auth_code.c_str()) == 0))
								{
									_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Found Authorization Code (%s) for user (%d)!", m_accesscodes[iUser].AuthCode.c_str(), iUser);

									std::string acRedirectUri = m_accesscodes[iUser].RedirectUri;
									std::string acScope = m_accesscodes[iUser].Scope;
									std::string CodeChallenge = m_accesscodes[iUser].CodeChallenge;
									uint64_t AuthTime = m_accesscodes[iUser].AuthTime;
									uint64_t CodeTime = m_accesscodes[iUser].ExpTime;
									uint64_t CurTime = (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

									m_accesscodes[iUser].AuthCode = "";	// Once used, make sure it cannot be used again
									m_accesscodes[iUser].RedirectUri = "";
									m_accesscodes[iUser].Scope = "";
									m_accesscodes[iUser].clientID = -1;
									m_accesscodes[iUser].AuthTime = 0;
									m_accesscodes[iUser].ExpTime = 0;
									m_accesscodes[iUser].CodeChallenge = "";

									// For so-called (registered) 'public' clients, it is not mandatory to send the client_secret as it is never a real secret with public clients
									// So in those cases, we use the registered secret
									if(m_users[iClient].ActiveTabs == 1 && client_secret.empty())
									{
										client_secret = m_users[iClient].Password;
									}

									std::string hashedsecret = client_secret;
									if (!(client_secret.length() == 32 && isHexRepresentation(client_secret)))	// 2 * MD5_DIGEST_LENGTH
									{
										hashedsecret = GenerateMD5Hash(client_secret);
									}

									if(hashedsecret == m_users[iClient].Password)		// The client_secrets should match (for public clients we just took the secret itself so will always match)
									{
										if(acRedirectUri.compare(redirect_uri.c_str()) == 0)
										{
											if (CodeTime > CurTime)
											{
												bool bPKCE = false;
												if(!(code_verifier.empty() || CodeChallenge.empty()))
												{
													// We have a code_challenge from the Auth request and now also a code_verifier.. let's see if they match
													_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Verifiying PKCE Code Challenge (%s) using provided verifyer!", CodeChallenge.c_str());
													if (CodeChallenge.compare(base64url_encode(sha256raw(code_verifier))) == 0)
													{
														bPKCE = true;
													}
												}
												if(code_verifier.empty() || bPKCE)
												{
													Json::Value jwtpayload;
													jwtpayload["auth_time"] = AuthTime;
													jwtpayload["preferred_username"] = m_users[iUser].Username;
													jwtpayload["name"] = m_users[iUser].Username;
													jwtpayload["roles"][0] = m_users[iUser].userrights;

													if (m_pWebEm->GenerateJwtToken(jwttoken, client_id, client_secret, m_users[iUser].Username, exptime, jwtpayload))
													{
														std::string username = std::to_string(iClient) + ";" + std::to_string(iUser);
														root["access_token"] = jwttoken;
														root["token_type"] = "Bearer";
														root["expires_in"] = exptime;
														root["refresh_token"] = GenerateOAuth2RefreshToken(username, refreshexptime);

														_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Succesfully generated a Refresh Token.");

														m_sql.safe_query("UPDATE Applications SET LastSeen=datetime('now') WHERE (Applicationname == '%s')", m_users[iClient].Username.c_str());
														_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Succesfully generated an Access Token.");
														rep.status = reply::ok;
													}
													else
													{
														root["error"] = "server_error";
														_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Something went wrong! Unable to generate Access Token!");
														iUser = -1;
													}
												}
												else
												{
													_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: PKCE Code verification failed!");
													iUser = -1;
												}
											}
											else
											{
												_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Authorization code has expired (%" PRIu64 ") (%" PRIu64 ")!", CodeTime, CurTime);
												iUser = -1;
											}
										}
										else
										{
											_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Redirect URI does not match (%s) (%s)!", acRedirectUri.c_str(), redirect_uri.c_str());
											iUser = -1;
										}
									}
									else
									{
										_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Client Secret does not match for client (%s)!", m_users[iClient].Username.c_str());
										iUser = -1;
									}
								}
								else
								{
									_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Cannot find valid access code (%s) for user!", auth_code.c_str());
									iUser = -1;
								}
							}
						}
						if(iUser == -1)
						{
							root["error"] = "unauthorized_client";
							_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Unauthorized/Unknown client_id (%s)!", client_id.c_str());
						}
					} // end 'authorization_code' grant_type
					if (grant_type.compare("password") == 0 )
					{
						bValidGrantType = true;

						// Maybe we should only allow this when in a safe 'local' network? So check if request comes from local networks?
						int iUser = -1;
						int iClient = -1;
						if (request::get_req_header(&req, "Authorization") != nullptr)
						{
							std::string auth_header = request::get_req_header(&req, "Authorization");
							// Basic Auth header
							size_t npos = auth_header.find("Basic ");
							if (npos != std::string::npos)
							{
								std::string decoded = base64_decode(auth_header.substr(6));
								npos = decoded.find(':');
								if (npos != std::string::npos)
								{
									std::string user = decoded.substr(0, npos);
									std::string passwd = decoded.substr(npos + 1);
									_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Found a Basic Auth Header for User (%s)", user.c_str());

									iUser = FindUser(user.c_str());
									if(iUser != -1)
									{
										if (GenerateMD5Hash(passwd).compare(m_users[iUser].Password) == 0)
										{
											iClient = FindClient(client_id.c_str());
											if (iClient != -1)
											{
												Json::Value jwtpayload;
												jwtpayload["preferred_username"] = m_users[iUser].Username;
												jwtpayload["name"] = m_users[iUser].Username;
												jwtpayload["roles"][0] = m_users[iUser].userrights;

												if (m_pWebEm->GenerateJwtToken(jwttoken, client_id, m_users[iClient].Password, user, exptime, jwtpayload))
												{
													root["access_token"] = jwttoken;
													root["token_type"] = "Bearer";
													root["expires_in"] = exptime;
													rep.status = reply::ok;
												}
												else
												{
													root["error"] = "server_error";
													_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Something went wrong! Unable to generate Access Token!");
													iUser = -1;
												}
											}
											else
											{
												_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Invalid client_id (%s)(%d) for user (%s) for Password grant flow!", client_id.c_str(), iClient, user.c_str());
												iUser = -1;
											}
										}
										else
										{
											_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Invalid credentials for user (%s) for Password grant flow!", user.c_str());
											iUser = -1;
										}
									}
									else
									{
										_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Could not find user (%s) for Password grant flow!", user.c_str());
									}
								}
							}
						}
						if (iUser == -1)
						{
							root["error"] = "invalid_client";
							rep.status = reply::unauthorized;
							_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Password grant flow failed!");
						}
					} // end 'password' grant_type
					if (grant_type.compare("refresh_token") == 0 )
					{
						bValidGrantType = true;
						std::string refresh_token = request::findValue(&req, "refresh_token");
						if (!refresh_token.empty())
						{
							std::string usernamefromtoken;
							if (ValidateOAuth2RefreshToken(refresh_token, usernamefromtoken))
							{
								std::vector<std::string> strarray;
								StringSplit(usernamefromtoken,";", strarray);
								if(strarray.size() == 2)
								{
									int iClient = std::atoi(strarray[0].c_str());
									int iUser = std::atoi(strarray[1].c_str());
									_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Found valid Refresh token (%s) (c:%d,u:%d)!", refresh_token.c_str(), iClient, iUser);

									Json::Value jwtpayload;
									jwtpayload["preferred_username"] = m_users[iUser].Username;
									jwtpayload["name"] = m_users[iUser].Username;
									jwtpayload["roles"][0] = m_users[iUser].userrights;

									if (m_pWebEm->GenerateJwtToken(jwttoken, m_users[iClient].Username, m_users[iClient].Password, m_users[iUser].Username, exptime, jwtpayload))
									{
										std::string username = std::to_string(iClient) + ";" + std::to_string(iUser);
										root["access_token"] = jwttoken;
										root["token_type"] = "Bearer";
										root["expires_in"] = exptime;
										root["refresh_token"] = GenerateOAuth2RefreshToken(username, refreshexptime);
										rep.status = reply::ok;
									}
									else
									{
										root["error"] = "server_error";
										_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Something went wrong! Unable to generate new Access Token from Resfreshtoken!");
									}
								}
								else
								{
									root["error"] = "access_denied";
									rep.status = reply::unauthorized;
									_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Refresh token (%s) does not contain valid reference to client/user (%s)!", refresh_token.c_str(), usernamefromtoken.c_str());
								}
							}
							else
							{
								root["error"] = "access_denied";
								rep.status = reply::unauthorized;
								_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Refresh token (%s) is not valid or expired!", refresh_token.c_str());
							}
							InvalidateOAuth2RefreshToken(refresh_token);
						}
						else
						{
							root["error"] = "invalid_request";
							rep.status = reply::bad_request;
							_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Unable to process refresh token request!");
						}
					} // end 'refresh_token' grant_type
				}
				if(!bValidGrantType)
				{
					root["error"] = "unsupported_grant_type";
					_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Invalid/unsupported grant_type (%s)!", grant_type.c_str());
				}
			}
			else
			{
				root["error"] = "invalid_request";
				_log.Debug(DEBUG_AUTH, "OAuth2 Access Token: Received invalid request method .%s.", req.method.c_str());
			}

			reply::set_content(&rep, root.toStyledString());
		}

		void CWebServer::GetOpenIDConfiguration(WebEmSession &session, const request &req, reply &rep)
		{
			Json::Value root, jaRTS(Json::arrayValue), jaTEASAVS(Json::arrayValue), jaGTS(Json::arrayValue), jaCCMS(Json::arrayValue), jaTEAMS(Json::arrayValue);

			reply::add_header_content_type(&rep, "application/json;charset=UTF-8");
			rep.status = reply::bad_request;

			std::string base_url = m_pWebEm->m_DigistRealm.substr(0, m_pWebEm->m_DigistRealm.size()-1);

			root["issuer"] = m_pWebEm->m_DigistRealm;
			root["authorization_endpoint"] = base_url + m_iamsettings.auth_url;
			root["token_endpoint"] = base_url + m_iamsettings.token_url;
			jaRTS.append("code");
			root["response_types_supported"] = jaRTS;
			jaTEASAVS.append("PS256");
			jaTEASAVS.append("RS256");
			jaTEASAVS.append("HS256");
			jaTEASAVS.append("HS384");
			jaTEASAVS.append("HS512");
			root["token_endpoint_auth_signing_alg_values_supported"] = jaTEASAVS;
			jaGTS.append("authorization_code");
			jaGTS.append("password");
			jaGTS.append("refresh_token");
			root["grant_types_supported"] = jaGTS;
			jaCCMS.append("S256");
			root["code_challenge_methods_supported"] = jaCCMS;
			jaTEAMS.append("client_secret_basic");
			root["token_endpoint_auth_methods_supported"] = jaTEAMS;

			rep.status = reply::ok;
			reply::set_content(&rep, root.toStyledString());
		}

        void CWebServer::PresentOauth2LoginDialog(reply &rep, const std::string &sApp, const std::string &sError)
        {
			std::string sTOTP = "";	// required

			rep = reply::stock_reply(reply::ok);

            reply::set_content(&rep, m_iamsettings.getAuthPageContent());

			stdreplace(rep.content, "###REPLACE_APP###", sApp);
			stdreplace(rep.content, "###REPLACE_ERROR###", sError);
			stdreplace(rep.content, "###REPLACE_2FATOTP###", sTOTP);

            if (!sError.empty())
                rep.status = reply::status_type::unauthorized;
            reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
            reply::add_header_content_type(&rep, "text/html");
            reply::add_header(&rep, "Cache-Control", "no-store");
            if (m_pWebEm->m_settings.is_secure())
                reply::add_security_headers(&rep);
        }

        std::string CWebServer::GenerateOAuth2RefreshToken(const std::string &username, const int refreshexptime)
        {
            std::string refreshtoken = base64url_encode(sha256raw(GenerateUUID()));
            WebEmStoredSession refreshsession;
            refreshsession.id = GenerateMD5Hash(refreshtoken);
            refreshsession.auth_token = refreshtoken;
            refreshsession.expires = mytime(nullptr) + refreshexptime;
            refreshsession.username = username;
            StoreSession(refreshsession);
            return refreshtoken;
        }

        bool CWebServer::ValidateOAuth2RefreshToken(const std::string &refreshtoken, std::string &username)
        {
            bool bOk = false;
            WebEmStoredSession refreshsession = GetSession(GenerateMD5Hash(refreshtoken));
            if	((!refreshsession.id.empty())
                && (refreshsession.auth_token.compare(refreshtoken) == 0)
                && (refreshsession.expires > mytime(nullptr))
                )
            {
                bOk = true;
                username = refreshsession.username;
            }

            return bOk;
        }

        void CWebServer::InvalidateOAuth2RefreshToken(const std::string &refreshtoken)
        {
            WebEmStoredSession refreshsession = GetSession(GenerateMD5Hash(refreshtoken));
            if	(!refreshsession.id.empty())
                RemoveSession(refreshsession.id);
        }

		bool CWebServer::VerifySHA1TOTP(const std::string &code, const std::string &key)
		{
			/*
			 * Time-based One-Time Password algorithm verification method
			 * Using SHA1 and 30 seconds time-intervals (RFC6238)
			 * Also checks big/litte-endian to ensure proper functioning on different systems
			 * Should work with (mobile) Authenticators like FreeOTP or Google's Authenticator
			 */
			if (code.size() != 6 || key.size() != 20) {
				return false;
			}

			unsigned long long intCounter = time(nullptr) / 30;
			unsigned long long endianness = 0xdeadbeef;
			if ((*(const uint8_t *)&endianness) == 0xef) {
				std::reverse((uint8_t*)&intCounter, (uint8_t*)&intCounter + sizeof(intCounter));
			}

			char md[20];
			unsigned int mdLen;
			HMAC(EVP_sha1(), key.c_str(), key.size(), (const unsigned char*)&intCounter, sizeof(intCounter), (unsigned char*)&md, &mdLen);

			int offset = md[19] & 0x0f;
			int bin_code = (md[offset] & 0x7f) << 24
				| (md[offset+1] & 0xff) << 16
				| (md[offset+2] & 0xff) << 8
				| (md[offset+3] & 0xff);
			bin_code = bin_code % 1000000;

			char calcCode[7];
			snprintf(calcCode, sizeof(calcCode), "%06d", bin_code);

			std::string sCalcCode(calcCode);

			_log.Debug(DEBUG_AUTH, "VerifySHA1TOTP: Code given .%s. -> calculated .%s. (%s)", code.c_str(), sCalcCode.c_str(), sha256hex(key).c_str());

			return (sCalcCode.compare(code) == 0);
		}

	} // namespace server
} // namespace http
