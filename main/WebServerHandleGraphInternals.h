#pragma once

/*
 * WebServerHandleGraphInternals.h
 *
 * Forward declarations for the per-range graph handler free functions
 * extracted from CWebServer::Cmd_HandleGraph.
 *
 * All functions live in the http::server namespace and are defined in their
 * own translation units (WebServerHandleGraphHour.cpp, etc.).
 *
 * NOTE: This header must only be included from translation units that also
 *       pull in WebServer.h (for CWebServer) and json/json.h.
 */

#include "WebServerHandleGraphUtils.h"

// Forward declarations – avoid pulling in heavy headers here.
namespace Json { class Value; }
class CSQLHelper;
namespace http { namespace server { class CWebServer; } }

namespace http
{
namespace server
{

// ---------------------------------------------------------------------------
void HandleGraphHour(const GraphContext& ctx, const request& req,
                     Json::Value& root, CSQLHelper& sql);

// ---------------------------------------------------------------------------
void HandleGraphDay(const GraphContext& ctx, const request& req,
                    Json::Value& root, CSQLHelper& sql,
                    CWebServer& webserver);

// ---------------------------------------------------------------------------
void HandleGraphWeek(const GraphContext& ctx, const request& req,
                     Json::Value& root, CSQLHelper& sql);

// ---------------------------------------------------------------------------
void HandleGraphMonthYear(const GraphContext& ctx, const request& req,
                           Json::Value& root, CSQLHelper& sql,
                           CWebServer& webserver);

// ---------------------------------------------------------------------------
void HandleGraphCustomRange(const GraphContext& ctx, const request& req,
                             Json::Value& root, CSQLHelper& sql,
                             CWebServer& webserver);

} // namespace server
} // namespace http
