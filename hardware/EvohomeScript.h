/**
 * Class for running external Evohome script(s)
 *
 *  Functions extracted from the original code by fullTalgoRythm
 *  Original header and copyright notice below
 */

/**
 * Evohome class for HGI80 gateway, evohome control, monitoring and integration with Domoticz
 *
 *  Copyright 2014 - fullTalgoRythm https://github.com/fullTalgoRythm/Domoticz-evohome
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 * 
 * based in part on https://github.com/mouse256/evomon
 * and details available at http://www.domoticaforum.eu/viewtopic.php?f=7&t=5806&start=90#p72564
 */


#pragma once

#include "EvohomeBase.h"


class CEvohomeScript : public CEvohomeBase
{
public:
	explicit CEvohomeScript(const int ID);
	~CEvohomeScript(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	void RunScript(const char *pdata, const unsigned char length);
};

