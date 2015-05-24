#pragma once

void StringSplit(std::string str, const std::string &delim, std::vector<std::string> &results);
std::string stdreplace(
	std::string result, 
	const std::string& replaceWhat, 
	const std::string& replaceWithWhat);
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

int mkdir_deep(const char *szDirName, int secattr);

double ConvertToCelsius(const double Fahrenheit);
double ConvertToFahrenheit(const double Celsius);
double ConvertTemperature(const double tValue, const unsigned char tSign);

std::vector<std::string> ExecuteCommandAndReturn(const std::string &szCommand);

std::string GenerateMD5Hash(const std::string &InputString, const std::string &Salt="");

void hue2rgb(const float hue, int &outR, int &outG, int &outB, const double maxValue = 100.0);
bool is_number(const std::string& s);
