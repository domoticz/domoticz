//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <Windows.h>
#include <sstream>
#include <string>
#include <vector>

#include "../client/telldus-core.h"
#include "common/common.h"
#include "common/Strings.h"
#include "service/Settings.h"

const int intMaxRegValueLength = 1000;

class Settings::PrivateData {
public:
	HKEY rootKey;
	std::wstring strRegPath;
	std::wstring getNodePath(Settings::Node type);
};

std::wstring Settings::PrivateData::getNodePath(Settings::Node type) {
	if (type == Settings::Device) {
		return L"SOFTWARE\\Telldus\\Devices\\";
	} else if (type == Settings::Controller) {
		return L"SOFTWARE\\Telldus\\Controllers\\";
	}
	return L"";
}

/*
* Constructor
*/
Settings::Settings(void) {
	d = new PrivateData();
	d->strRegPath = L"SOFTWARE\\Telldus\\";
	d->rootKey = HKEY_LOCAL_MACHINE;
}

/*
* Destructor
*/
Settings::~Settings(void) {
	delete d;
}

/*
* Return the number of stored devices
*/
int Settings::getNumberOfNodes(Node type) const {
	TelldusCore::MutexLocker locker(&mutex);

	int intNumberOfNodes = 0;
	HKEY hk;

	LONG lnExists = RegOpenKeyEx(d->rootKey, d->getNodePath(type).c_str(), 0, KEY_QUERY_VALUE, &hk);

	if(lnExists == ERROR_SUCCESS) {
		std::wstring strNumSubKeys;
		DWORD dNumSubKeys;
		RegQueryInfoKey(hk, NULL, NULL, NULL, &dNumSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		intNumberOfNodes = static_cast<int>(dNumSubKeys);

		RegCloseKey(hk);
	}
	return intNumberOfNodes;
}


int Settings::getNodeId(Node type, int intNodeIndex) const {
	TelldusCore::MutexLocker locker(&mutex);

	int intReturn = -1;
	HKEY hk;

	LONG lnExists = RegOpenKeyEx(d->rootKey, d->getNodePath(type).c_str(), 0, KEY_READ, &hk);

	if(lnExists == ERROR_SUCCESS) {
		wchar_t* Buff = new wchar_t[intMaxRegValueLength];
		DWORD size = intMaxRegValueLength;
		if (RegEnumKeyEx(hk, intNodeIndex, (LPWSTR)Buff, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			intReturn = _wtoi(Buff);
		}

		delete[] Buff;
		RegCloseKey(hk);
	}
	return intReturn;
}

/*
* Add a new node
*/
int Settings::addNode(Node type) {
	TelldusCore::MutexLocker locker(&mutex);

	int intNodeId = -1;
	HKEY hk;

	DWORD dwDisp;
	intNodeId = getNextNodeId(type);

	std::wstring strCompleteRegPath = d->getNodePath(type);
	strCompleteRegPath.append(TelldusCore::intToWstring(intNodeId));

	if (RegCreateKeyEx(d->rootKey, strCompleteRegPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &dwDisp)) {
		// fail
		intNodeId = -1;
	}

	RegCloseKey(hk);
	return intNodeId;
}

/*
* Get next available device id
*/
int Settings::getNextNodeId(Node type) const {
	// Private, no locks needed
	int intReturn = -1;
	HKEY hk;
	DWORD dwDisp;

	LONG lnExists = RegCreateKeyEx(d->rootKey, d->getNodePath(type).c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &dwDisp);	 // create or open if already created

	if(lnExists == ERROR_SUCCESS) {
		DWORD dwLength = sizeof(DWORD);
		DWORD nResult(0);

		LONG lngStatus = RegQueryValueEx(hk, L"LastUsedId", NULL, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwLength);

		if(lngStatus == ERROR_SUCCESS) {
			intReturn = nResult + 1;
		} else {
			intReturn = 1;
		}
		DWORD dwVal = intReturn;
		RegSetValueEx (hk, L"LastUsedId", 0L, REG_DWORD, (CONST BYTE*) &dwVal, sizeof(DWORD));
	}
	RegCloseKey(hk);
	return intReturn;
}

/*
* Remove a device
*/
int Settings::removeNode(Node type, int intNodeId) {
	TelldusCore::MutexLocker locker(&mutex);

	std::wstring strCompleteRegPath = d->getNodePath(type);
	strCompleteRegPath.append(TelldusCore::intToWstring(intNodeId));

	LONG lngSuccess = RegDeleteKey(d->rootKey, strCompleteRegPath.c_str());

	if(lngSuccess == ERROR_SUCCESS) {
		// one of the deletions succeeded
		return TELLSTICK_SUCCESS;
	}

	return TELLSTICK_ERROR_UNKNOWN;
}

std::wstring Settings::getSetting(const std::wstring &strName) const {
	std::wstring strReturn;
	HKEY hk;

	std::wstring strCompleteRegPath = d->strRegPath;
	LONG lnExists = RegOpenKeyEx(d->rootKey, strCompleteRegPath.c_str(), 0, KEY_QUERY_VALUE, &hk);

	if(lnExists == ERROR_SUCCESS) {
		wchar_t* Buff = new wchar_t[intMaxRegValueLength];
		DWORD dwLength = sizeof(wchar_t)*intMaxRegValueLength;
		LONG lngStatus = RegQueryValueEx(hk, strName.c_str(), NULL, NULL, (LPBYTE)Buff, &dwLength);

		if(lngStatus == ERROR_MORE_DATA) {
			// The buffer is to small, recreate it
			delete[] Buff;
			Buff = new wchar_t[dwLength];
			lngStatus = RegQueryValueEx(hk, strName.c_str(), NULL, NULL, (LPBYTE)Buff, &dwLength);
		}
		if (lngStatus == ERROR_SUCCESS) {
			strReturn = Buff;
		}
		delete[] Buff;
	}
	RegCloseKey(hk);
	return strReturn;
}

std::wstring Settings::getStringSetting(Node type, int intNodeId, const std::wstring &name, bool parameter) const {
	std::wstring strReturn;
	HKEY hk;

	std::wstring strCompleteRegPath = d->getNodePath(type);
	strCompleteRegPath.append(TelldusCore::intToWstring(intNodeId));
	LONG lnExists = RegOpenKeyEx(d->rootKey, strCompleteRegPath.c_str(), 0, KEY_QUERY_VALUE, &hk);

	if(lnExists == ERROR_SUCCESS) {
		wchar_t* Buff = new wchar_t[intMaxRegValueLength];
		DWORD dwLength = sizeof(wchar_t)*intMaxRegValueLength;
		LONG lngStatus = RegQueryValueEx(hk, name.c_str(), NULL, NULL, (LPBYTE)Buff, &dwLength);

		if(lngStatus == ERROR_MORE_DATA) {
			// The buffer is to small, recreate it
			delete[] Buff;
			Buff = new wchar_t[dwLength];
			lngStatus = RegQueryValueEx(hk, name.c_str(), NULL, NULL, (LPBYTE)Buff, &dwLength);
		}
		if (lngStatus == ERROR_SUCCESS) {
			strReturn = Buff;
		}
		delete[] Buff;
	}
	RegCloseKey(hk);
	return strReturn;
}

int Settings::setStringSetting(Node type, int intNodeId, const std::wstring &name, const std::wstring &value, bool parameter) {
	HKEY hk;
	int ret = TELLSTICK_SUCCESS;

	std::wstring strNodeId = TelldusCore::intToWstring(intNodeId);
	std::wstring strCompleteRegPath = d->getNodePath(type);
	strCompleteRegPath.append(strNodeId);
	LONG lnExists = RegOpenKeyEx(d->rootKey, strCompleteRegPath.c_str(), 0, KEY_WRITE, &hk);

	if (lnExists == ERROR_SUCCESS) {
		int length = static_cast<int>(value.length()) * sizeof(wchar_t);
		RegSetValueEx(hk, name.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), length+1);
	} else {
		ret = TELLSTICK_ERROR_UNKNOWN;
	}
	RegCloseKey(hk);

	return ret;
}

int Settings::getIntSetting(Node type, int intNodeId, const std::wstring &name, bool parameter) const {
	int intReturn = 0;

	std::wstring strSetting = getStringSetting(type, intNodeId, name, parameter);
	if (strSetting.length()) {
		intReturn = static_cast<int>(strSetting[0]);  // TODO(micke): do real conversion instead
	}

	return intReturn;
}

int Settings::setIntSetting(Node type, int intNodeId, const std::wstring &name, int value, bool parameter) {
	int intReturn = TELLSTICK_ERROR_UNKNOWN;
	HKEY hk;

	std::wstring strCompleteRegPath =  d->getNodePath(type);
	strCompleteRegPath.append(TelldusCore::intToWstring(intNodeId));
	LONG lnExists = RegOpenKeyEx(d->rootKey, strCompleteRegPath.c_str(), 0, KEY_WRITE, &hk);
	if (lnExists == ERROR_SUCCESS) {
		DWORD dwVal = value;
		lnExists = RegSetValueEx (hk, name.c_str(), 0L, REG_DWORD, (CONST BYTE*) &dwVal, sizeof(DWORD));
		if (lnExists == ERROR_SUCCESS) {
			intReturn = TELLSTICK_SUCCESS;
		}
	}
	RegCloseKey(hk);
	return intReturn;
}
