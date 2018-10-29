#pragma once

#if defined WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif

// Function declarations taken from telldus-core.h

typedef void (WINAPI *TDDeviceEvent)(int deviceId, int method, const char *data, int callbackId, void *context);
typedef void (WINAPI *TDRawDeviceEvent)(const char *data, int controllerId, int callbackId, void *context);
typedef void (WINAPI *TDSensorEvent)(const char *protocol, const char *model, int id, int dataType, const char *value, int timestamp, int callbackId, void *context);

#ifdef __cplusplus
extern "C" {
#endif
struct TelldusFunctions {
    void (WINAPI *Init)(void);
	int (WINAPI *RegisterDeviceEvent)(TDDeviceEvent eventFunction, void *context);
	int (WINAPI *RegisterRawDeviceEvent)(TDRawDeviceEvent eventFunction, void *context);
	int (WINAPI *RegisterSensorEvent)(TDSensorEvent eventFunction, void *context);
	int (WINAPI *UnregisterCallback)(int callbackId);
	void (WINAPI *Close)(void);
	void (WINAPI *ReleaseString)(char *thestring);

	int (WINAPI *TurnOn)(int intDeviceId);
	int (WINAPI *TurnOff)(int intDeviceId);
	int (WINAPI *Dim)(int intDeviceId, unsigned char level);
	int (WINAPI *Methods)(int id, int methodsSupported);

	int (WINAPI *GetNumberOfDevices)();
	int (WINAPI *GetDeviceId)(int intDeviceIndex);

	char * (WINAPI *GetName)(int intDeviceId);
};
#ifdef __cplusplus
} // extern "C"
#endif

// Device methods
#define TELLSTICK_TURNON	1
#define TELLSTICK_TURNOFF	2
#define TELLSTICK_BELL		4
#define TELLSTICK_TOGGLE	8
#define TELLSTICK_DIM		16
#define TELLSTICK_LEARN		32
#define TELLSTICK_EXECUTE	64
#define TELLSTICK_UP		128
#define TELLSTICK_DOWN		256
#define TELLSTICK_STOP		512

// Sensor value types
#define TELLSTICK_TEMPERATURE	1
#define TELLSTICK_HUMIDITY		2
#define TELLSTICK_RAINRATE		4
#define TELLSTICK_RAINTOTAL		8
#define TELLSTICK_WINDDIRECTION	16
#define TELLSTICK_WINDAVERAGE	32
#define TELLSTICK_WINDGUST		64

