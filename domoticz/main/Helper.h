#pragma once

void StringSplit(std::string str, std::string delim, std::vector<std::string> &results);
std::string stdreplace(
	std::string result, 
	const std::string& replaceWhat, 
	const std::string& replaceWithWhat);

std::vector<std::string> GetSerialPorts(bool &bUseDirectPath);
