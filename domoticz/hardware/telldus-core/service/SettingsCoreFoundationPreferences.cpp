//
// C++ Implementation: telldussettingsconfuse
//
// Description:
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <stdlib.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include "common/Strings.h"
#include "service/Settings.h"
#include "client/telldus-core.h"

class privateVars {
public:
	CFStringRef app_ID;
	CFStringRef userName;
	CFStringRef hostName;
};

class Settings::PrivateData {
public:
	CFStringRef app_ID;
	CFStringRef userName;
	CFStringRef hostName;
};

/*
* Constructor
*/
Settings::Settings(void) {
	d = new PrivateData();
	d->app_ID = CFSTR( "com.telldus.core" );
	d->userName = kCFPreferencesAnyUser;
	d->hostName = kCFPreferencesCurrentHost;
}

/*
* Destructor
*/
Settings::~Settings(void) {
	delete d;
}

/*
* Return a setting
*/
std::wstring Settings::getSetting(const std::wstring &strName) const {
	return L"";
}

/*
* Return the number of stored devices
*/
int Settings::getNumberOfNodes(Node type) const {
	CFArrayRef cfarray = CFPreferencesCopyKeyList( d->app_ID, d->userName, d->hostName );
	if (!cfarray) {
		return 0;
	}
	CFIndex size = CFArrayGetCount( cfarray );
	int nodes = 0;
	for (CFIndex k = 0; k < size; ++k) {
		CFStringRef key = (CFStringRef) CFArrayGetValueAtIndex(cfarray, k);
		if (!CFStringHasSuffix( key, CFSTR(".name") )) {
			continue;
		}
		if (type == Device && CFStringHasPrefix( key, CFSTR("devices.") )) {
			++nodes;
		} else if (type == Controller && CFStringHasPrefix( key, CFSTR("controllers.") )) {
			++nodes;
		}
	}
	CFRelease(cfarray);
	return nodes;
}

int Settings::getNodeId(Node type, int intNodeIndex) const {
	CFArrayRef cfarray = CFPreferencesCopyKeyList( d->app_ID, d->userName, d->hostName );
	if (!cfarray) {
		return 0;
	}
	CFIndex size = CFArrayGetCount( cfarray );
	int index = 0;
	int id = 0;
	for (CFIndex k = 0; k < size; ++k) {
		CFStringRef key = (CFStringRef) CFArrayGetValueAtIndex(cfarray, k);
		if (!CFStringHasSuffix( key, CFSTR(".name") )) {
			continue;
		}
		if ( type == Device && !CFStringHasPrefix(key, CFSTR("devices.")) ) {
			continue;
		}
		if ( type == Controller && !CFStringHasPrefix(key, CFSTR("controllers.")) ) {
			continue;
		}
		if (index == intNodeIndex) {
			CFArrayRef split = CFStringCreateArrayBySeparatingStrings( 0, key, CFSTR(".") );
			if ( !split ) {
				continue;
			}
			if (CFArrayGetCount( split ) != 3 ) {
				CFRelease( split );
				continue;
			}

			// This code crashes!
			// CFNumberRef cfid = (CFNumberRef) CFArrayGetValueAtIndex( split, 1 );
			// if (cfid)
			// 	CFNumberGetValue( cfid, kCFNumberIntType, &id);

			CFStringRef cfid = (CFStringRef) CFArrayGetValueAtIndex( split, 1 );
			char *cp = NULL;
			CFIndex size = CFStringGetMaximumSizeForEncoding( CFStringGetLength( cfid ), kCFStringEncodingUTF8) + 1;
			cp = reinterpret_cast<char *>(malloc(size));
			CFStringGetCString( cfid, cp, size, kCFStringEncodingUTF8 );
			char *newcp = reinterpret_cast<char *>(realloc( cp, strlen(cp) + 1));
			if (newcp != NULL) {
				cp = newcp;
				id = atoi(cp);
			} else {
				// Should not happen
				id = 0;
			}
			free(cp);

			CFRelease(split);
			break;
		}
		index++;
	}
	CFRelease(cfarray);
	return id;
}

/*
* Add a new node
*/
int Settings::addNode(Node type) {
	int id = getNextNodeId(type);
	setStringSetting( type, id, L"name", L"", false );  // Create a empty name so the node has an entry
	if (type == Device) {
		// Is there a reason we do this?
		setStringSetting( type, id, L"model", L"", false );
	}
	return id;
}

/*
* Get next available node id
*/
int Settings::getNextNodeId(Node type) const {
	int id = 0, max = 0;
	int numberOfNodes = getNumberOfNodes(type);
	for( int i = 0; i < numberOfNodes; i++) {
		id = getNodeId( type, i );
		if (id > max) {
			max = id;
		}
	}
	max++;
	return max;
}

/*
* Remove a device
*/
int Settings::removeNode(Node type, int intNodeId) {
	int ret = TELLSTICK_ERROR_DEVICE_NOT_FOUND;
	CFStringRef filterKey = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d."), getNodeString(type).c_str(), intNodeId);  // The key to search for

	CFArrayRef cfarray = CFPreferencesCopyKeyList( d->app_ID, d->userName, d->hostName );
	if (!cfarray) {
		CFRelease(filterKey);
		return TELLSTICK_ERROR_UNKNOWN;
	}
	CFIndex size = CFArrayGetCount( cfarray );
	for (CFIndex k = 0; k < size; ++k) {
		CFStringRef key = (CFStringRef) CFArrayGetValueAtIndex(cfarray, k);
		if (CFStringHasPrefix( key, filterKey ) ) {
			CFPreferencesSetValue( key, NULL, d->app_ID, d->userName, d->hostName );  // Remove the key
			ret = TELLSTICK_SUCCESS;
		}
	}

	CFPreferencesSynchronize( d->app_ID, d->userName, d->hostName );
	CFRelease(cfarray);
	CFRelease(filterKey);
	return ret;
}

std::wstring Settings::getStringSetting(Node type, int intNodeId, const std::wstring &wname, bool parameter) const {
	std::string name(TelldusCore::wideToString(wname));
	CFStringRef cfname = CFStringCreateWithCString( 0, name.c_str(), kCFStringEncodingUTF8 );

	CFStringRef key;
	if (parameter) {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.parameters.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	} else {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	}

	CFStringRef value;

	value = (CFStringRef)CFPreferencesCopyValue(key, d->app_ID, d->userName, d->hostName);
	if (!value) {
		CFRelease(key);
		CFRelease(cfname);
		return L"";
	}

	std::wstring retval;
	char *cp = NULL;
	CFIndex size = CFStringGetMaximumSizeForEncoding( CFStringGetLength( value ), kCFStringEncodingUTF8) + 1;
	cp = reinterpret_cast<char *>(malloc(size));
	CFStringGetCString( value, cp, size, kCFStringEncodingUTF8 );
	char *newcp = reinterpret_cast<char *>(realloc( cp, strlen(cp) + 1));
	if (newcp != NULL) {
		cp = newcp;
		retval = TelldusCore::charToWstring(cp);
	} else {
		// Should not happen
		retval = L"";
	}
	free(cp);

	CFRelease(value);
	CFRelease(key);
	CFRelease(cfname);
	return retval;
}

int Settings::setStringSetting(Node type, int intNodeId, const std::wstring &wname, const std::wstring &wvalue, bool parameter) {
	std::string name(TelldusCore::wideToString(wname));
	std::string value(TelldusCore::wideToString(wvalue));
	CFStringRef cfname = CFStringCreateWithCString( 0, name.c_str(), kCFStringEncodingUTF8 );
	CFStringRef cfvalue = CFStringCreateWithCString( 0, value.c_str(), kCFStringEncodingUTF8 );

	CFStringRef key;
	if (parameter) {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.parameters.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	} else {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	}

	CFPreferencesSetValue( key, cfvalue, d->app_ID, d->userName, d->hostName );
	CFPreferencesSynchronize( d->app_ID, d->userName, d->hostName );
	CFRelease(key);
	CFRelease(cfvalue);
	CFRelease(cfname);
	return TELLSTICK_SUCCESS;
}

int Settings::getIntSetting(Node type, int intNodeId, const std::wstring &wname, bool parameter) const {
	int retval = 0;
	std::string name(TelldusCore::wideToString(wname));
	CFStringRef cfname = CFStringCreateWithCString( 0, name.c_str(), kCFStringEncodingUTF8 );
	CFNumberRef cfvalue;

	CFStringRef key;
	if (parameter) {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.parameters.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	} else {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	}

	cfvalue = (CFNumberRef)CFPreferencesCopyValue(key, d->app_ID, d->userName, d->hostName);

	// If the preference exists, use it.
	if (cfvalue) {
		try {
			// Numbers come out of preferences as CFNumber objects.
			if (!CFNumberGetValue(cfvalue, kCFNumberIntType, &retval)) {
				retval = 0;
			}
			CFRelease(cfvalue);
		} catch (std::exception e) {
			retval = 0;
		}
	}

	CFRelease(key);
	CFRelease(cfname);
	return retval;
}

int Settings::setIntSetting(Node type, int intNodeId, const std::wstring &wname, int value, bool parameter) {
	std::string name(TelldusCore::wideToString(wname));
	CFStringRef cfname = CFStringCreateWithCString( 0, name.c_str(), kCFStringEncodingUTF8 );
	CFNumberRef cfvalue = CFNumberCreate(NULL, kCFNumberIntType, &value);

	CFStringRef key;
	if (parameter) {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.parameters.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	} else {
		key = CFStringCreateWithFormat(0, NULL, CFSTR("%ss.%d.%@"), getNodeString(type).c_str(), intNodeId, cfname);
	}

	CFPreferencesSetValue( key, cfvalue, d->app_ID, d->userName, d->hostName );
	CFPreferencesSynchronize( d->app_ID, d->userName, d->hostName );
	CFRelease(key);
	CFRelease(cfvalue);
	CFRelease(cfname);
	return TELLSTICK_SUCCESS;
}
