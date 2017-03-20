/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ITGWSwitchIntertechno.h
 * Author: alexiade
 *
 * Created on March 18, 2017, 9:01 AM
 */

#ifndef ITGWSWITCHINTERTECHNO_H
#define ITGWSWITCHINTERTECHNO_H

class ITGWSwitchIntertechno {
public:
  ITGWSwitchIntertechno();
  ITGWSwitchIntertechno(const ITGWSwitchIntertechno& orig);
  virtual ~ITGWSwitchIntertechno();
  std::string GetEncodedAddress(const char* ID);
  std::string GetOnCommandString(const char* ID);
  std::string GetOffCommandString(const char* ID);
  std::string GetSpeed();
  std::string GetHeader();
private:

};

#endif /* ITGWSWITCHINTERTECHNO_H */

