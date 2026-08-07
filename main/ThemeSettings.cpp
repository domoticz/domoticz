#include "stdafx.h"
#include "ThemeSettings.h"
#include "Helper.h"
#include "SQLHelper.h"
#include "json_helper.h"
#include <algorithm>
#include <cstdlib>

namespace
{
	constexpr auto sqlCreateThemeSettings = "CREATE TABLE IF NOT EXISTS [ThemeSettings]("
						"[Scope] INTEGER NOT NULL,"
						"[UserID] INTEGER NOT NULL DEFAULT 0,"
						"[ThemeName] TEXT NOT NULL,"
						"[Value] TEXT NOT NULL DEFAULT '{}',"
						"[LastUpdate] TEXT NOT NULL DEFAULT '',"
						"PRIMARY KEY([Scope],[UserID],[ThemeName])"
						");";
} // namespace

void CThemeSettings::CreateTable()
{
	m_sql.safe_exec_no_return(sqlCreateThemeSettings);
}

void CThemeSettings::MigrateFromPreferences()
{
	// Explode the legacy single-blob Preferences.ThemeSettings into per-theme instance
	// rows. The legacy Preferences row stays on disk (and is refreshed by every
	// instance-default write) so a downgrade still finds usable data; nothing reads it
	// anymore. Idempotent through DO NOTHING, so a re-run cannot clobber newer rows.
	std::string sThemeSettings;
	if (!m_sql.GetPreferencesVar("ThemeSettings", sThemeSettings) || sThemeSettings.empty())
		return;

	Json::Value jRoot;
	if (!ParseJSon(sThemeSettings, jRoot) || !jRoot.isObject())
		return;

	for (const auto &themeName : jRoot.getMemberNames())
	{
		// Only object values carry theme settings; a hand-edited scalar has no
		// sub-object to overlay and is left behind in the legacy row.
		if (!jRoot[themeName].isObject())
			continue;
		const std::string szTheme = JSonToRawString(jRoot[themeName]);
		m_sql.safe_query("INSERT INTO ThemeSettings (Scope, UserID, ThemeName, Value, LastUpdate) "
				 "VALUES (%d, 0, '%q', '%q', strftime('%%Y-%%m-%%d %%H:%%M:%%f','now','localtime')) "
				 "ON CONFLICT(Scope, UserID, ThemeName) DO NOTHING",
				 static_cast<int>(eScope::Instance), themeName.c_str(), szTheme.c_str());
	}
}

bool CThemeSettings::IsValidThemeName(const std::string &themeName)
{
	if (themeName.empty() || themeName.size() > MAX_NAME_LENGTH)
		return false;
	return std::all_of(themeName.begin(), themeName.end(), [](const char c) {
		const auto uc = static_cast<unsigned char>(c);
		return (uc >= 0x20) && (uc != 0x7F);
	});
}

bool CThemeSettings::Get(const eScope scope, const unsigned long userID, const std::string &themeName, Json::Value &value, std::string &lastUpdate)
{
	auto result = m_sql.safe_query("SELECT Value, LastUpdate FROM ThemeSettings WHERE (Scope==%d) AND (UserID==%llu) AND (ThemeName=='%q')", static_cast<int>(scope), (unsigned long long)userID,
				       themeName.c_str());
	if (result.empty())
		return false;
	// A row that no longer parses (hand-edited database) reports as absent rather than
	// as a broken layer the caller would have to special-case.
	if (!ParseJSon(result[0][0], value))
		return false;
	lastUpdate = result[0][1];
	return true;
}

CThemeSettings::eResult CThemeSettings::Set(const eScope scope, const unsigned long userID, const std::string &themeName, const std::string &szValue, const std::string &expectedLastUpdate,
					    std::string &newLastUpdate)
{
	if (!IsValidThemeName(themeName))
		return eResult::InvalidTheme;
	if (szValue.empty())
		return eResult::MissingValue;
	// Reject on length before parsing
	if (szValue.size() > MAX_REQUEST_SIZE)
		return eResult::TooLarge;

	Json::Value jValue;
	bool bParsed = false;
	try
	{
		bParsed = ParseJSonStrict(szValue, jValue);
	}
	catch (const std::exception &)
	{
		// jsoncpp throws instead of returning false on deeply nested input
		bParsed = false;
	}
	if (!bParsed || !jValue.isObject())
		return eResult::InvalidJson;

	// The cap applies to the reserialized form, which is also what gets stored
	const std::string szStored = JSonToRawString(jValue);
	if (szStored.size() > MAX_STORED_SIZE)
		return eResult::TooLarge;

	// Soft per-scope cap: count existing rows, excluding the theme being written so an
	// update to an existing row is never blocked by the cap.
	const int themeCount = CountRows(scope, userID, themeName);
	if (themeCount < 0)
		return eResult::DBError;
	if (themeCount >= MAX_THEMES_PER_SCOPE)
		return eResult::TooManyThemes;

	// The concurrency token is generated here in C++ (rather than by SQL's strftime())
	// so the exact value written can be handed back to the caller without a second,
	// non-atomic read-back that could race with another session's interleaving write.
	const std::string szNewLastUpdate = TimeToString(nullptr, TF_DateTimeMs);

	// Single atomic upsert. If the row exists and the caller's concurrency token does
	// not match its LastUpdate, the DO UPDATE WHERE clause fails and changes()==0,
	// which reports as a conflict. A fresh INSERT always succeeds (changes()==1).
	const int changes = m_sql.safe_exec_changes("INSERT INTO ThemeSettings (Scope, UserID, ThemeName, Value, LastUpdate) "
						    "VALUES (%d, %llu, '%q', '%q', '%q') "
						    "ON CONFLICT(Scope, UserID, ThemeName) DO UPDATE "
						    "SET Value = excluded.Value, LastUpdate = excluded.LastUpdate "
						    "WHERE ThemeSettings.LastUpdate == '%q'",
						    static_cast<int>(scope), (unsigned long long)userID, themeName.c_str(), szStored.c_str(), szNewLastUpdate.c_str(), expectedLastUpdate.c_str());
	if (changes < 0)
		return eResult::DBError;
	if (changes == 0)
		return eResult::Conflict;

	newLastUpdate = szNewLastUpdate;
	return eResult::Ok;
}

CThemeSettings::eResult CThemeSettings::Reset(const eScope scope, const unsigned long userID, const std::string &themeName)
{
	if (!IsValidThemeName(themeName))
		return eResult::InvalidTheme;
	// Unconditional and idempotent: no concurrency token, and deleting a row that is
	// not there is not an error.
	const int changes =
		m_sql.safe_exec_changes("DELETE FROM ThemeSettings WHERE (Scope==%d) AND (UserID==%llu) AND (ThemeName=='%q')", static_cast<int>(scope), (unsigned long long)userID, themeName.c_str());
	return (changes < 0) ? eResult::DBError : eResult::Ok;
}

CThemeSettings::eResult CThemeSettings::DeleteForUser(const unsigned long userID)
{
	const int changes = m_sql.safe_exec_changes("DELETE FROM ThemeSettings WHERE (Scope==%d) AND (UserID==%llu)", static_cast<int>(eScope::User), (unsigned long long)userID);
	return (changes < 0) ? eResult::DBError : eResult::Ok;
}

int CThemeSettings::CountRows(const eScope scope, const unsigned long userID, const std::string &excludeThemeName)
{
	// Soft cap: this count-then-insert is not atomic with the upsert in Set(), so two
	// concurrent requests for two new theme names can both pass the check and land one
	// row over the cap. Acceptable for a per-user quota, not a security boundary.
	auto result = m_sql.safe_query("SELECT COUNT(*) FROM ThemeSettings WHERE (Scope==%d) AND (UserID==%llu) AND (ThemeName<>'%q')", static_cast<int>(scope), (unsigned long long)userID,
				       excludeThemeName.c_str());
	if (result.empty())
		return -1;
	return atoi(result[0][0].c_str());
}

bool CThemeSettings::GetMerged(const bool haveUser, const unsigned long userID, Json::Value &merged)
{
	// Instance defaults overlaid by the user's rows. Per theme name, a user row
	// replaces the whole instance sub-object (shallow, one level: themes treat their
	// sub-object as atomic).
	merged = Json::Value(Json::objectValue);
	auto result = m_sql.safe_query("SELECT ThemeName, Value FROM ThemeSettings WHERE (Scope==%d)", static_cast<int>(eScope::Instance));
	for (const auto &sd : result)
	{
		Json::Value jValue;
		if (ParseJSon(sd[1], jValue) && jValue.isObject())
			merged[sd[0]] = jValue;
	}
	if (haveUser)
	{
		result = m_sql.safe_query("SELECT ThemeName, Value FROM ThemeSettings WHERE (Scope==%d) AND (UserID==%llu)", static_cast<int>(eScope::User), (unsigned long long)userID);
		for (const auto &sd : result)
		{
			Json::Value jValue;
			if (ParseJSon(sd[1], jValue) && jValue.isObject())
				merged[sd[0]] = jValue;
		}
	}
	return !merged.empty();
}

void CThemeSettings::MirrorDefaults()
{
	// Keep the legacy Preferences.ThemeSettings blob equal to the current instance rows
	// so a downgrade to a pre-181 database finds the current instance defaults instead
	// of a snapshot from migration day. Nothing in the new code reads it.
	Json::Value jRoot(Json::objectValue);
	auto result = m_sql.safe_query("SELECT ThemeName, Value FROM ThemeSettings WHERE (Scope==%d)", static_cast<int>(eScope::Instance));
	for (const auto &sd : result)
	{
		Json::Value jValue;
		if (ParseJSon(sd[1], jValue) && jValue.isObject())
			jRoot[sd[0]] = jValue;
	}
	m_sql.UpdatePreferencesVar("ThemeSettings", JSonToRawString(jRoot));
}

const char *CThemeSettings::ErrorCode(const eResult res)
{
	switch (res)
	{
		case eResult::InvalidTheme:
			return "invalid_theme";
		case eResult::MissingValue:
			return "missing_value";
		case eResult::InvalidJson:
			return "invalid_json";
		case eResult::TooLarge:
			return "too_large";
		case eResult::TooManyThemes:
			return "too_many_themes";
		case eResult::Conflict:
			return "conflict";
		default:
			return "db_error";
	}
}

const char *CThemeSettings::ErrorMessage(const eResult res)
{
	switch (res)
	{
		case eResult::InvalidTheme:
			return "Missing or invalid theme parameter";
		case eResult::MissingValue:
			return "Missing value parameter";
		case eResult::InvalidJson:
			return "value must be a JSON object";
		case eResult::TooLarge:
			return "Theme settings exceed the 16 KB limit";
		case eResult::TooManyThemes:
			return "Theme settings limit reached; reset settings for themes you no longer use";
		case eResult::Conflict:
			return "Theme settings were changed by another session; reload and retry";
		default:
			return "Failed to store theme settings";
	}
}
