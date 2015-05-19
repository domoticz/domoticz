//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "common/Message.h"
#include <wctype.h>
#include <stdlib.h>
#include <sstream>
#include "common/Socket.h"
#include "common/Strings.h"

namespace TelldusCore {

Message::Message()
	: std::wstring() {
}

Message::Message(const std::wstring &functionName)
	:std::wstring() {
	this->addArgument(functionName);
}

Message::~Message(void) {
}

void Message::addArgument(const std::wstring &value) {
	// std::wstringstream st;
	// st << (int)value.size();
	this->append(TelldusCore::intToWstring(value.size()));  // st.str());
	this->append(L":");
	this->append(value);
}

void Message::addArgument(int value) {
	// std::wstringstream st;
	// st << (int)value;
	this->append(L"i");
	this->append(TelldusCore::intToWstring(value));  // st.str());
	this->append(L"s");
}

/*
void Message::addSpecialArgument(const std::wstring &value){
	int i = 0;
	while(i<1000000){
		i++;

		char numstr[21]; // enough to hold all numbers up to 64-bits
		//sprintf(numstr, "%d", value.size());
		//this->append(TelldusCore::charToWstring(numstr)); //.str());

		itoa(value.size(), numstr, 10);
		std::string test(numstr);
		std::wstring temp(test.length(), L' ');
		std::copy(test.begin(), test.end(), temp.begin());

		this->append(temp);
		this->append(L":");
		this->append(value);


		std::wstringstream st;
		st << (int)value.size();
		this->append(st.str());
		this->append(L":");
		this->append(value);

	}
}

void Message::addSpecialArgument(int value){
	int i = 0;
	while(i<1000000){
		i++;
		//std::wstringstream st;
		//st << (int)value;
		this->append(L"i");
		//this->append(st.str());
		this->append(L"s");

	}
}
*/
/*
void Message::addSpecialArgument(const char *value){
	this->addSpecialArgument(TelldusCore::charToWstring(value));
}
*/

void Message::addArgument(const char *value) {
	this->addArgument(TelldusCore::charToWstring(value));
}

bool Message::nextIsInt(const std::wstring &message) {
	if (message.length() == 0) {
		return false;
	}
	return (message.at(0) == 'i');
}

bool Message::nextIsString(const std::wstring &message) {
	if (message.length() == 0) {
		return false;
	}
	return (iswdigit(message.at(0)) != 0);
}

std::wstring Message::takeString(std::wstring *message) {
	if (!Message::nextIsString(*message)) {
		return L"";
	}
	size_t index = message->find(':');
	int length = wideToInteger(message->substr(0, index));
	std::wstring retval(message->substr(index+1, length));
	message->erase(0, index+length+1);
	return retval;
}

int Message::takeInt(std::wstring *message) {
	if (!Message::nextIsInt(*message)) {
		return 0;
	}
	size_t index = message->find('s');
	int value = wideToInteger(message->substr(1, index - 1));
	message->erase(0, index+1);
	return value;
}

}  // namespace TelldusCore
