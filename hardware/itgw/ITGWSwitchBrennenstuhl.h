/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ITGWSwitchBrennenstuhl.h
 * Author: alexiade
 *
 * Created on March 18, 2017, 1:49 PM
 */

#ifndef ITGWSWITCHBRENNENSTUHL_H
#define ITGWSWITCHBRENNENSTUHL_H

class ITGWSwitchBrennenstuhl {
public:
  ITGWSwitchBrennenstuhl();
  ITGWSwitchBrennenstuhl(const ITGWSwitchBrennenstuhl& orig);
  virtual ~ITGWSwitchBrennenstuhl();
  std::string GetEncodedAddress(const char* ID);
  std::string GetOnCommandString(const char* ID);
  std::string GetOffCommandString(const char* ID);
  std::string GetSpeed();
  std::string GetHeader();
private:

};

#endif /* ITGWSWITCHBRENNENSTUHL_H */

