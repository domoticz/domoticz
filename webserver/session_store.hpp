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
	std::string remote_host;
	std::string auth_token;
	std::string username;
	time_t expires;
} WebEmStoredSession;

/**
 * Gives access to the user session store.
 */
class session_store {
public:
  virtual ~session_store() = default;
  ;

  /**
   * Retrieve user session from store
   */
  virtual WebEmStoredSession GetSession(const std::string &sessionId) = 0;

  /**
   * Save user session into store
   */
  virtual void StoreSession(const WebEmStoredSession &session) = 0;

  /**
   * Remove user session from store
   */
  virtual void RemoveSession(const std::string &sessionId) = 0;

  /**
   * Remove expired user sessions from store
   */
  virtual void CleanSessions() = 0;
};

typedef session_store* session_store_impl_ptr;

} // namespace server
} // namespace http
#endif /* WEBSERVER_SESSION_STORE_HPP_ */
