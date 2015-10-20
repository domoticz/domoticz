/*
 * session_store.hpp
 *
 *  Created on: 12 oct. 2015
 *      Author: gaudryc
 */

#ifndef WEBSERVER_SESSION_STORE_HPP_
#define WEBSERVER_SESSION_STORE_HPP_

namespace http {
namespace server {

typedef struct _tWebEmStoredSession {
	std::string id;
	std::string auth_token;
	std::string username;
	time_t expires;
} WebEmStoredSession;

class session_store {
public:
	virtual const WebEmStoredSession GetSession(const std::string & sessionId)=0;
	virtual void StoreSession(const WebEmStoredSession & session)=0;
	virtual void RemoveSession(const std::string & sessionId)=0;
};

}
}
#endif /* WEBSERVER_SESSION_STORE_HPP_ */
