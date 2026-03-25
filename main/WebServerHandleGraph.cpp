/*
 * WebServerHandleGraph.cpp
 *
 *  Created on: 7 August 2023
 *
 * This file is NOT a separate class but is part of 'main/WebServer.cpp'
 * It contains the 'HandleGraph' Cmd that is part of the WebServer class, but for sourcecode management
 * reasons separated out into its own file. The definitions are still in 'main/Webserver.h'
*/

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerHelper.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <iostream>
#include <fstream>
#include <json/json.h>

#include "mainworker.h"
#include "Helper.h"
#include "EventSystem.h"
#include "HTMLSanitizer.h"
#include "json_helper.h"
#include "Logger.h"
#include "SQLHelper.h"
#include "WebServerHandleGraphInternals.h"

namespace http
{
	namespace server
	{
		void CWebServer::Cmd_HandleGraph(WebEmSession& session, const request& req, Json::Value& root)
		{
			// "stats" is a special pseudo-range that redirects to kWh stats; handle
			// it before the device lookup performed by BuildGraphContext.
			if (request::findValue(&req, "range") == "stats")
			{
				Cmd_GetkWhStats(session, req, root);
				return;
			}

			GraphContext ctx;
			if (!BuildGraphContext(req, m_sql, ctx))
				return;

			_log.Debug(DEBUG_WEBSERVER, "CWebServer::Cmd_HandleGraph() : dType:%02X  dSubType:%02X  metertype:%d",
				ctx.dType, ctx.dSubType, int(ctx.metertype));

			if (ctx.sensor == "counter")
				Cmd_GetCosts(session, req, root);

			if (ctx.srange == "hour")
			{
				HandleGraphHour(ctx, req, root, m_sql);
				return;
			}
			else if (ctx.srange == "day")
			{
				HandleGraphDay(ctx, req, root, m_sql, *this);
				return;
			}
			else if (ctx.srange == "week")
			{
				HandleGraphWeek(ctx, req, root, m_sql);
				return;
			}
			else if (ctx.srange == "month" || ctx.srange == "year" || !ctx.sgroupby.empty())
			{
				HandleGraphMonthYear(ctx, req, root, m_sql, *this);
				return;
			}
			else
			{
				std::string dateStart, dateEnd;
				if (ParseCustomRange(ctx.srange, dateStart, dateEnd))
				{
					HandleGraphCustomRange(ctx, req, root, m_sql, *this);
					return;
				}
			}
		}

	} // namespace server
} // namespace http
