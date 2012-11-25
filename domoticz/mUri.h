#include <string>
#include <algorithm>    // find

struct Uri
{
public:
	std::string QueryString, Path, Protocol, Host, Port;

	static Uri Parse(const std::string &uri)
	{
		Uri result;

		result.Port="80";

		typedef std::string::const_iterator iterator_t;

		if (uri.length() == 0)
			return result;

		iterator_t uriEnd = uri.end();

		// get query start
		iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');

		// protocol
		iterator_t protocolStart = uri.begin();
		iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

		if (protocolEnd != uriEnd)
		{
			std::string prot = &*(protocolEnd);
			if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
			{
				result.Protocol = std::string(protocolStart, protocolEnd);
				protocolEnd += 3;   //      ://
			}
			else
				protocolEnd = uri.begin();  // no protocol
		}
		else
			protocolEnd = uri.begin();  // no protocol

		// host
		iterator_t hostStart = protocolEnd;
		iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

		iterator_t hostEnd = std::find(protocolEnd, 
			(pathStart != uriEnd) ? pathStart : queryStart,
			':');  // check for port

		result.Host = std::string(hostStart, hostEnd);

		// port
		if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
		{
			hostEnd++;
			iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
			result.Port = std::string(hostEnd, portEnd);
		}

		// path
		if (pathStart != uriEnd)
			result.Path = std::string(pathStart, queryStart);

		// query
		if (queryStart != uriEnd)
			result.QueryString = std::string(queryStart, uri.end());

		return result;

	}   // Parse
};  // uri
