#pragma once

#include <string>

namespace Json
{
	class Value;
} // namespace Json

// Theme settings are stored in two layers: instance-wide defaults written by an admin
// (Scope::Instance) and a per-user overlay written by any authenticated user
// (Scope::User). getsettings serves the merge of the two, a user row replacing the whole
// sub-object of the theme it names. Everything here is table logic; the web layer only
// authenticates, passes parameters in and maps the result back to JSON.
class CThemeSettings
{
      public:
	// Reported to themes as "ThemeSettingsAPI"; bump when the command surface changes
	static constexpr int API_VERSION = 1;

	// Requests are rejected on length before parsing; the stored cap is applied to the
	// reserialized JSON, which is what actually goes into the row
	static constexpr size_t MAX_REQUEST_SIZE = 64 * 1024;
	static constexpr size_t MAX_STORED_SIZE = 16 * 1024;
	static constexpr size_t MAX_NAME_LENGTH = 64;
	static constexpr int MAX_THEMES_PER_SCOPE = 32;

	enum class eScope
	{
		Instance = 0,
		User = 1,
	};

	enum class eResult
	{
		Ok,
		InvalidTheme,
		MissingValue,
		InvalidJson,
		TooLarge,
		TooManyThemes,
		Conflict,
		DBError,
	};

	// Schema and migration, called from CSQLHelper::OpenDatabase()
	static void CreateTable();
	static void MigrateFromPreferences();

	static bool IsValidThemeName(const std::string &themeName);

	static bool Get(eScope scope, unsigned long userID, const std::string &themeName, Json::Value &value, std::string &lastUpdate);
	static eResult Set(eScope scope, unsigned long userID, const std::string &themeName, const std::string &szValue, const std::string &expectedLastUpdate, std::string &newLastUpdate);
	static eResult Reset(eScope scope, unsigned long userID, const std::string &themeName);
	// Removes every user-scope row of one user at once, backing both the API's
	// reset=all and user deletion. Deliberately has no instance-scope counterpart:
	// wiping every theme's defaults in a single call has no undo and no use case.
	static eResult DeleteForUser(unsigned long userID);

	static bool GetMerged(bool haveUser, unsigned long userID, Json::Value &merged);
	static void MirrorDefaults();

	// Single place where an eResult becomes the JSON API's error/message pair
	static const char *ErrorCode(eResult res);
	static const char *ErrorMessage(eResult res);

      private:
	static int CountRows(eScope scope, unsigned long userID, const std::string &excludeThemeName);
};
