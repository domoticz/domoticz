/*
 * File:   ITGWSwitchIntertechno.cpp
 * Author: alexiade
 *
 * Created on March 18, 2017, 9:01 AM
 */
#include "stdafx.h"
#include "ITGWSwitchIntertechno.h"

#define B_LOW "4,12,4,12,"
#define B_FLOAT "4,12,12,4,"
#define ON "4,12,12,4,4,12,12,4,"
#define OFF "4,12,12,4,4,12,4,12,"


ITGWSwitchIntertechno::ITGWSwitchIntertechno() {
}

ITGWSwitchIntertechno::ITGWSwitchIntertechno(const ITGWSwitchIntertechno& orig) {
}

ITGWSwitchIntertechno::~ITGWSwitchIntertechno() {
}

std::string ITGWSwitchIntertechno::GetEncodedAddress(const char* ID){
	std::string res = "";
  std::string _id = ID;
  std::string unit = _id.substr(2,2);
  std::string house = _id.substr(0,2);
  std::string enc_unit = "";
  std::string enc_house = "";
  std::string h = B_FLOAT;
  std::string l = B_LOW;
  //res = house+unit;
  switch (atoi(house.c_str()) - 40) {
            case 1:
               enc_house =  l + l + l + l;
                break;
            case 2:
               enc_house =  h + l + l + l;
                break;
            case 3:
               enc_house =  l + h + l + l;
                break;
            case 4:
               enc_house =  h + h + l + l;
                break;
            case 5:
               enc_house =  l + l + h + l;
                break;
            case 6:
               enc_house =  h + l + h + l;
                break;
            case 7:
               enc_house =  l + h + h + l;
                break;
            case 8:
               enc_house =  h + h + h + l;
                break;
            case 9:
               enc_house =  l + l + l + h;
                break;
            case 10:
               enc_house =  h + l + l + h;
                break;
            case 11:
               enc_house =  l + h + l + h;
                break;
            case 12:
               enc_house =  h + h + l + h;
                break;
            case 13:
               enc_house =  l + l + h + h;
                break;
            case 14:
               enc_house =  h + l + h + h;
                break;
            case 15:
               enc_house =  l + h + h + h;
                break;
            case 16:
               enc_house =  h + h + h + h;
                break;
  }

  switch (atoi(unit.c_str())) {
            case 1:
               enc_unit =  l + l + l + l;
                break;
            case 2:
               enc_unit =  h + l + l + l;
                break;
            case 3:
               enc_unit =  l + h + l + l;
                break;
            case 4:
               enc_unit =  h + h + l + l;
                break;
            case 5:
               enc_unit =  l + l + h + l;
                break;
            case 6:
               enc_unit =  h + l + h + l;
                break;
            case 7:
               enc_unit =  l + h + h + l;
                break;
            case 8:
               enc_unit =  h + h + h + l;
                break;
            case 9:
               enc_unit =  l + l + l + h;
                break;
            case 10:
               enc_unit =  h + l + l + h;
                break;
            case 11:
               enc_unit =  l + h + l + h;
                break;
            case 12:
               enc_unit =  h + h + l + h;
                break;
            case 13:
               enc_unit =  l + l + h + h;
                break;
            case 14:
               enc_unit =  h + l + h + h;
                break;
            case 15:
               enc_unit =  l + h + h + h;
                break;
            case 16:
               enc_unit =  h + h + h + h;
                break;
  }
  return enc_house + enc_unit + l + h;
}

std::string ITGWSwitchIntertechno::GetOnCommandString(const char* ID){
  std::string encAddress = GetEncodedAddress(ID);
//  _log.Log(LOG_ERROR, "ITGW: intertechno encoded address: %s", encAddress.c_str());
  		std::stringstream sstr;
		  sstr << encAddress;
      sstr << ON;

      return sstr.str();
}

std::string ITGWSwitchIntertechno::GetOffCommandString(const char* ID){
   std::string encAddress = GetEncodedAddress(ID);
   std::stringstream sstr;
	 sstr << encAddress;
   sstr << OFF;

  return sstr.str();
}

std::string ITGWSwitchIntertechno::GetSpeed(){
  const std::string speed = "125,";
  return speed;
}

std::string ITGWSwitchIntertechno::GetHeader(){
  const std::string head = "0,0,6,11125,89,26,0,";
  return head;
}