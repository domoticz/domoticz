/*
 * session.hpp
 *
 *  Created on: 15 oct. 2015
 *      Author: gaudryc
 */

#ifndef WEBSERVER_HTTP_SESSION_HPP_
#define WEBSERVER_HTTP_SESSION_HPP_

#include <string>
#include <map>

namespace http {
namespace server {

struct http_session {
  std::string id;
  std::map<std::string, std::string> parameters;
};

} // namespace server
} // namespace http

#endif /* WEBSERVER_HTTP_SESSION_HPP_ */
