#pragma once

#include <string>
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/mainworker.h"

namespace mcp
{
	void McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesTemplatesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getLogging(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getFloorplan(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setThermostatSetpoint(const Json::Value& jsonRequest, Json::Value& jsonRPCRep);
	bool getStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool searchDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool setSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setDimmerLevel(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool controlBlinds(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setColorBrightness(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setColorTemperature(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getScenes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool switchScene(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getRooms(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getRoomDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSceneDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getHardware(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSystemSettings(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSunTimes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getCameras(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getFloorplans(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getUsers(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device);

	bool validRPC(const std::string &sInput, Json::Value &jsonRequest, std::string &sError);

	bool getAllDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool renameDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool deleteDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool createVirtualSensor(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateDeviceValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getDeviceHistory(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getUserVariables(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool addUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool deleteUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool addLogMessage(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool sendNotification(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getEvents(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool createEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
}
