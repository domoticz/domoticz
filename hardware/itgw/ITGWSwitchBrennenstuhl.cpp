/*
 * File:   ITGWSwitchBrennenstuhl.cpp
 * Author: alexiade
 *
 * Created on March 18, 2017, 1:49 PM
 */
#include "stdafx.h"
#include "ITGWSwitchBrennenstuhl.h"


#define B_LOW "1,3,1,3,"
#define B_FLOAT "1,3,3,1,"
#define ON "1,3,3,1,1,3,3,1,"
#define OFF "1,3,3,1,1,3,1,3,"

/** Brennenstuhl basic 433MHz swiches support
 * Tested with assorted RCS-1000 devices.
 *
 * http://www.brennenstuhl.com/index.php?module=products_downloads&index%5Bproducts_downloads%5D%5Bcategory%5D=105
 */

ITGWSwitchBrennenstuhl::ITGWSwitchBrennenstuhl() {
}

ITGWSwitchBrennenstuhl::ITGWSwitchBrennenstuhl(const ITGWSwitchBrennenstuhl& orig) {
}

ITGWSwitchBrennenstuhl::~ITGWSwitchBrennenstuhl() {
}

std::string ITGWSwitchBrennenstuhl::GetEncodedAddress(const char* ID){
	std::string res = "";
  std::string _id = ID;
  std::string dipswitch_hex = _id.substr(1,3);

  char * pEnd;
  int dipswitch = (int) std::strtol(dipswitch_hex.c_str(), &pEnd, 16);
  std::string enc_addr = "";
  std::string h = B_FLOAT;
  std::string l = B_LOW;
  //res = house+unit;

  int i = 0;
  int shift = dipswitch;
  std::stringstream sstr;
  for(i = 0; i < 10; i++){
    const std::string &temp = sstr.str();
    sstr.seekp(0);
    if((shift % 2) == 1) {
      sstr << l;
    }
    else{
      sstr << h;
    }
    sstr << temp;
    shift = shift >> 1;
  }
  enc_addr =  sstr.str();
  return enc_addr;
}

std::string ITGWSwitchBrennenstuhl::GetOnCommandString(const char* ID){
  std::string encAddress = GetEncodedAddress(ID);
//  _log.Log(LOG_ERROR, "ITGW: intertechno encoded address: %s", encAddress.c_str());
  		std::stringstream sstr;
		  sstr << encAddress;
      sstr << ON;

      return sstr.str();
}

std::string ITGWSwitchBrennenstuhl::GetOffCommandString(const char* ID){
   std::string encAddress = GetEncodedAddress(ID);
   std::stringstream sstr;
	 sstr << encAddress;
   sstr << OFF;

  return sstr.str();
}

std::string ITGWSwitchBrennenstuhl::GetSpeed(){
  const std::string speed = "32,";
  return speed;
}

std::string ITGWSwitchBrennenstuhl::GetHeader(){
  const std::string speed = "0,0,10,11200,350,26,0,";
  return speed;
}