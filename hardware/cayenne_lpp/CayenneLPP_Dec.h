// CayenneLPP Decoder
//
// Decodes a CayenneLPP binary buffer to JSON format
//
// https://os.mbed.com/teams/myDevicesIoT/code/Cayenne-LPP
// https://github.com/open-source-parsers/jsoncpp
//
// Copyright (c) 2018 Robbert E. Peters. All rights reserved.  
// Licensed under the MIT License. See LICENSE file in the project root for full license information.  
//
#pragma once
#include "CayenneLPP.h"

namespace Json
{
	class Value;
} // namespace Json

class CayenneLPPDec
{
public:
	static bool ParseLPP(const uint8_t *pBuffer, size_t Len, Json::Value &root);
};

