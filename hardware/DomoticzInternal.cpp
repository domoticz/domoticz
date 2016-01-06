/*
 * DomoticzInternal.cpp
 *
 *  Created on: 5 janv. 2016
 *      Author: chrisgaudry
 */

#include "DomoticzInternal.h"

DomoticzInternal::DomoticzInternal(const int ID) {
	m_HwdID = ID;
	m_bSkipReceiveCheck = true;
	m_bIsStarted = false;
}

DomoticzInternal::~DomoticzInternal() {

}

