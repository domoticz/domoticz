#pragma once
#include <string>

namespace Json {
	class Value;
}

class DeconzNode
{
public:
	DeconzNode(Json::Value node);
	DeconzNode();

protected:

public:
	std::string name;
	std::string modelId;
	std::string swVersion;
	std::string type;
	std::string manufacturer;
	std::string uniqueId;	
};

