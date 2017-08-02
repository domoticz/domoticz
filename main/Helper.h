
#pragma once

void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results);
void stdreplace(
	std::string &inoutstring,
	const std::string& replaceWhat,
	const std::string& replaceWithWhat);
void stdupper(std::string &inoutstring);
bool file_exist (const char *filename);
std::vector<std::string> GetSerialPorts(bool &bUseDirectPath);
double CalculateAltitudeFromPressure(double pressure);
float pressureSeaLevelFromAltitude(float altitude, float atmospheric, float temp);
float pressureToAltitude(float seaLevel, float atmospheric, float temp);

std::string &stdstring_ltrim(std::string &s);
std::string &stdstring_rtrim(std::string &s);
std::string &stdstring_trim(std::string &s);
double CalculateDewPoint(double temp, int humidity);
uint32_t IPToUInt(const std::string &ip);
bool isInt(const std::string &s);

void sleep_seconds(const long seconds);
void sleep_milliseconds(const long milliseconds);

int createdir(const char *szDirName, int secattr);
int mkdir_deep(const char *szDirName, int secattr);

double ConvertToCelsius(const double Fahrenheit);
double ConvertToFahrenheit(const double Celsius);
double ConvertTemperature(const double tValue, const unsigned char tSign);

std::vector<std::string> ExecuteCommandAndReturn(const std::string &szCommand, int &returncode);

void DateAsciiTotmTime (std::string &sLastUpdate , struct tm &LastUpdateTime  );
void AsciiTime (struct tm &ltime , char * pLastUpdate );
std::string  GetCurrentAsciiTime ();
void AsciiTime ( time_t DateStart, char * DateStr );
time_t DateAsciiToTime_t ( std::string & DateStr );
std::string GenerateMD5Hash(const std::string &InputString, const std::string &Salt="");

void hue2rgb(const float hue, int &outR, int &outG, int &outB, const double maxValue = 100.0);
void rgb2hsb(const int r, const int g, const int b, float hsbvals[3]);

bool is_number(const std::string& s);
void padLeft(std::string &str, const size_t num, const char paddingChar = '0');

bool IsLightOrSwitch(const int devType, const int subType);

int MStoBeaufort(const float ms);

struct dirent;
bool dirent_is_directory(std::string dir, struct dirent *ent);
bool dirent_is_file(std::string dir, struct dirent *ent);
void DirectoryListing(std::vector<std::string>& entries, const std::string &dir, bool bInclDirs, bool bInclFiles);

std::string GenerateUserAgent();
std::string MakeHtml(const std::string &txt);

#if defined WIN32
	int gettimeofday(timeval * tp, void * tzp);
#endif
int getclock(struct timeval *tv);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

bool IsArgumentSecure(const std::string &arg);
uint32_t SystemUptime();

