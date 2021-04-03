#pragma once

#include <string>
#include <map>

enum _eTimeFormat
{
	TF_Time = 0,	// 0
	TF_Date,		// 1
	TF_DateTime,	// 2
	TF_DateTimeMs	// 3
};

unsigned int Crc32(unsigned int crc, const unsigned char* buf, size_t size);
void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results);
uint64_t hexstrtoui64(const std::string &str);
std::string ToHexString(const uint8_t *pSource, size_t length);
std::vector<char> HexToBytes(const std::string& hex);
void stdreplace(
	std::string &inoutstring,
	const std::string& replaceWhat,
	const std::string& replaceWithWhat);
void stdupper(std::string& inoutstring);
void stdlower(std::string& inoutstring);
void stdupper(std::wstring& inoutstring);
void stdlower(std::wstring& inoutstring);

template< typename T > inline
std::string int_to_hex(T i)
{
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') << std::setw(sizeof(T) * 2)
		<< std::hex << i;
	return stream.str();
}

bool file_exist (const char *filename);
std::vector<std::string> GetSerialPorts(bool &bUseDirectPath);
double CalculateAltitudeFromPressure(double pressure);
float pressureSeaLevelFromAltitude(float altitude, float atmospheric, float temp);
float pressureToAltitude(float seaLevel, float atmospheric, float temp);

double deg2rad(double deg);
double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d);
std::string &stdstring_ltrim(std::string &s);
std::string &stdstring_rtrim(std::string &s);
std::string &stdstring_trim(std::string &s);
double CalculateDewPoint(double temp, int humidity);
uint32_t IPToUInt(const std::string &ip);
bool isInt(const std::string &s);

void sleep_seconds(long seconds);
void sleep_milliseconds(long milliseconds);

int createdir(const char *szDirName, int secattr);
int mkdir_deep(const char *szDirName, int secattr);

int RemoveDir(const std::string &dirnames, std::string &errorPath);

double ConvertToCelsius(double Fahrenheit);
double ConvertToFahrenheit(double Celsius);
double ConvertTemperature(double tValue, unsigned char tSign);

std::vector<std::string> ExecuteCommandAndReturn(const std::string &szCommand, int &returncode);

std::string TimeToString(const time_t *ltime, _eTimeFormat format);
std::string GenerateMD5Hash(const std::string &InputString, const std::string &Salt="");

void hsb2rgb(float hue, float saturation, float vlue, int &outR, int &outG, int &outB, double maxValue = 100.0);
void rgb2hsb(int r, int g, int b, float hsbvals[3]);

bool is_number(const std::string& s);
void padLeft(std::string &str, size_t num, char paddingChar = '0');

bool IsLightOrSwitch(int devType, int subType);

int MStoBeaufort(float ms);

void FixFolderEnding(std::string &folder);

struct dirent;
bool dirent_is_directory(const std::string &dir, struct dirent *ent);
bool dirent_is_file(const std::string &dir, struct dirent *ent);
void DirectoryListing(std::vector<std::string>& entries, const std::string &dir, bool bInclDirs, bool bInclFiles);

std::string GenerateUserAgent();
std::string MakeHtml(const std::string &txt);
std::string SafeHtml(const std::string &txt);

#if defined WIN32
	int gettimeofday(timeval * tp, void * tzp);
#endif
int getclock(struct timeval *tv);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

bool IsArgumentSecure(const std::string &arg);
uint32_t SystemUptime();
int GenerateRandomNumber(int range);
int GetDirFilesRecursive(const std::string &DirPath, std::map<std::string, int> &_Files);

int SetThreadName(const std::thread::native_handle_type &thread, const char *name);

#if !defined(WIN32)
bool IsDebuggerPresent();
#endif

std::string GenerateUUID();
double round_digits(double dIn, int totDigits);
