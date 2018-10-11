#include "stdafx.h"
#include "../../json/value.h"
#include "DeconzNode.h"

DeconzNode::DeconzNode(Json::Value node)
{
	name = node["name"].asString();
	type = node["type"].asString();
	modelId = node["modelid"].asString();	
	swVersion = node["swversion"].asString();
	uniqueId = node["uniqueid"].asString();

	if (node["manufacturer"].isObject())
	{
		manufacturer = node["manufacturer"].asString();
	} else
	{
		manufacturer = node["manufacturername"].asString();
	}
}

DeconzNode::DeconzNode() : name(""), type(""), modelId(""), swVersion(""), uniqueId(""), manufacturer("")
{	
}
