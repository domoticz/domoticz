#include "stdafx.h"
#include "UrlEncode.h"

#include <stdlib.h>
#include <cmath>
#include <string>

#include <algorithm>

// HEX Values array
char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
// UNSAFE String
std::string CURLEncode::csUnsafeString= "\"<>%\\^[]`+$,@:;/!#?=&";

// PURPOSE OF THIS FUNCTION IS TO CONVERT A GIVEN CHAR TO URL HEX FORM
std::string CURLEncode::convert(char val) 
{
	std::string csRet;
	csRet += "%";
	csRet += decToHex(val, 16);	
	return  csRet;
}

// THIS IS A HELPER FUNCTION.
// PURPOSE OF THIS FUNCTION IS TO GENERATE A HEX REPRESENTATION OF GIVEN CHARACTER
std::string CURLEncode::decToHex(char num, int radix)
{	
	int temp=0;	
	std::string csTmp;
	int num_char;
	num_char = (int) num;

	// ISO-8859-1 
	// IF THE IF LOOP IS COMMENTED, THE CODE WILL FAIL TO GENERATE A 
	// PROPER URL ENCODE FOR THE CHARACTERS WHOSE RANGE IN 127-255(DECIMAL)
	if (num_char < 0)
		num_char = 256 + num_char;

	while (num_char >= radix)
	{
		temp = num_char % radix;
		num_char = (int)floor((float)num_char / radix);
		csTmp = hexVals[temp];
	}
	if (num_char>16)
		num_char = 0; //should not be nececery
	csTmp += hexVals[num_char];

	if(csTmp.size() < 2)
	{
		csTmp += '0';
	}

	std::string strdecToHex(csTmp);
	// Reverse the String
	std::reverse(strdecToHex.begin(),strdecToHex.end());

	return strdecToHex;
}

// PURPOSE OF THIS FUNCTION IS TO CHECK TO SEE IF A CHAR IS URL UNSAFE.
// TRUE = UNSAFE, FALSE = SAFE
bool CURLEncode::isUnsafe(char compareChar)
{
	bool bcharfound = false;
	char tmpsafeChar;
	int m_strLen = 0;

	m_strLen = csUnsafeString.size();
	for(int ichar_pos = 0; ichar_pos < m_strLen ;ichar_pos++)
	{
		tmpsafeChar = csUnsafeString[ichar_pos]; 
		if(tmpsafeChar == compareChar)
		{ 
			bcharfound = true;
			break;
		} 
	}
	int char_ascii_value = 0;
	//char_ascii_value = __toascii(compareChar);
	char_ascii_value = (int) compareChar;

	if(bcharfound == false &&  char_ascii_value > 32 && char_ascii_value < 123)
	{
		return false;
	}
	// found no unsafe chars, return false		
	else
	{
		return true;
	}

	return true;
}
// PURPOSE OF THIS FUNCTION IS TO CONVERT A STRING 
// TO URL ENCODE FORM.
std::string CURLEncode::URLEncode(std::string pcsEncode)
{	
	int ichar_pos;
	std::string csEncode;
	std::string csEncoded;	
	int m_length;

	csEncode = pcsEncode;
	m_length = csEncode.size();

	for(ichar_pos = 0; ichar_pos < m_length; ichar_pos++)
	{
		char ch = csEncode[ichar_pos];
		if (ch < ' ') 
		{
			ch = ch;
		}		
		if(!isUnsafe(ch))
		{
			// Safe Character				
			csEncoded += ch;
		}
		else
		{
			// get Hex Value of the Character
			csEncoded += convert(ch);
		}
	}

	return csEncoded;
}

std::string CURLEncode::URLDecode(const std::string SRC)
{
	std::string ret;
	char ch;
	int ii;
	size_t len=SRC.length();
	for (size_t i=0; i<len; i++) {
		if (int(SRC[i])==37) {
			if ( i+2 >= len )
				return SRC;
			int iret=sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
			if (iret < 1)
				return "";
			ch=static_cast<char>(ii);
			ret+=ch;
			i=i+2;
		} else {
			ret+=SRC[i];
		}
	}
	return (ret);
}

