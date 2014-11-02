#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <sstream>
#include "../client/telldus-core.h"

#ifdef _WINDOWS
#define strcasecmp _stricmp
#define DEGREE " "
#else
#define DEGREE "°"
#endif

const int SUPPORTED_METHODS =
	TELLSTICK_TURNON |
	TELLSTICK_TURNOFF |
	TELLSTICK_BELL |
	TELLSTICK_DIM;

const int DATA_LENGTH = 20;

void print_usage( char *name ) {
	printf("Usage: %s [ options ]\n", name);
	printf("\n");
	printf("Options:\n");
	printf("         -[bdefhlnrv] [ --list ] [ --help ]\n");
	printf("                      [ --list-sensors ] [ --list-devices ]\n");
	printf("                      [ --on device ] [ --off device ] [ --bell device ]\n");
	printf("                      [ --learn device ]\n");
	printf("                      [ --dimlevel level --dim device ]\n");
	printf("                      [ --raw input ]\n");
	printf("\n");
	printf("       --list (-l short option)\n");
	printf("             List currently configured devices and all discovered sensors.\n");
	printf("\n");
	printf("       --list-sensors\n");
	printf("       --list-devices\n");
	printf("             Alternative devices/sensors listing:\n");
	printf("             Shows devices and/or sensors using key=value format (with tabs as\n");
	printf("             separators, one device/sensor per line, no header lines.)\n");
	printf("\n");
	printf("       --help (-h short option)\n");
	printf("             Shows this screen.\n");
	printf("\n");
	printf("       --on device (-n short option)\n");
	printf("             Turns on device. 'device' could either be an integer of the\n");
	printf("             device-id, or the name of the device.\n");
	printf("             Both device-id and name is outputed with the --list option\n");
	printf("\n");
	printf("       --off device (-f short option)\n");
	printf("             Turns off device. 'device' could either be an integer of the\n");
	printf("             device-id, or the name of the device.\n");
	printf("             Both device-id and name is outputed with the --list option\n");
	printf("\n");
	printf("       --dim device (-d short option)\n");
	printf("             Dims device. 'device' could either be an integer of the device-id,\n");
	printf("             or the name of the device.\n");
	printf("             Both device-id and name is outputed with the --list option\n");
	printf("             Note: The dimlevel parameter must be set before using this option.\n");
	printf("\n");
	printf("       --dimlevel level (-v short option)\n");
	printf("             Set dim level. 'level' should an integer, 0-255.\n");
	printf("             Note: This parameter must be set before using dim.\n");
	printf("\n");
	printf("       --bell device (-b short option)\n");
	printf("             Sends bell command to devices supporting this. 'device' could\n");
	printf("             either be an integer of the device-id, or the name of the device.\n");
	printf("             Both device-id and name is outputed with the --list option\n");
	printf("\n");
	printf("       --learn device (-e short option)\n");
	printf("             Sends a special learn command to devices supporting this. This is normaly\n");
	printf("             devices of 'selflearning' type. 'device' could either be an integer\n");
	printf("             of the device-id, or the name of the device.\n");
	printf("             Both device-id and name is outputed with the --list option\n");
	printf("\n");
	printf("       --raw input (-r short option)\n");
	printf("             This command sends a raw command to TellStick.\n");
	printf("             input can be either - or a filename. If input is - the data is\n");
	printf("             taken from stdin, otherwise the data is taken from the supplied filename.\n");
	printf("\n");
	printf("             Example to turn on an ArcTech codeswitch A1:\n");
	printf("             echo 'S$k$k$k$k$k$k$k$k$k$k$k$k$k$k$k$k$k$k$kk$$kk$$kk$$}+' | tdtool --raw -\n");
	printf("\n");
	printf("Report bugs to <info.tech@telldus.se>\n");
}

void print_version() {
	printf("tdtool " VERSION "\n");
	printf("\n");
	printf("Copyright (C) 2011 Telldus Technologies AB\n");
	printf("\n");
	printf("Written by Micke Prag <micke.prag@telldus.se>\n");
}

void print_device( int index ) {
	tdInit();
	int intId = tdGetDeviceId(index);
	char *name = tdGetName(intId);
	printf("%i\t%s\t", intId, name);
	tdReleaseString(name);
	int lastSentCommand = tdLastSentCommand(intId, SUPPORTED_METHODS);
	char *level = 0;
	switch(lastSentCommand) {
		case TELLSTICK_TURNON:
			printf("ON");
			break;
		case TELLSTICK_TURNOFF:
			printf("OFF");
			break;
		case TELLSTICK_DIM:
			level = tdLastSentValue(intId);
			printf("DIMMED:%s", level);
			tdReleaseString(level);
			break;
		default:
			printf("Unknown state");
	}
	printf("\n");
}

int list_devices() {
	tdInit();
	int intNum = tdGetNumberOfDevices();
	if (intNum < 0) {
		char *errorString = tdGetErrorString(intNum);
		fprintf(stderr, "Error fetching devices: %s\n", errorString);
		tdReleaseString(errorString);
		return intNum;
	}
	printf("Number of devices: %i\n", intNum);
	int i = 0;
	while (i < intNum) {
		print_device( i );
		i++;
	}

	char protocol[DATA_LENGTH], model[DATA_LENGTH];
	int sensorId = 0, dataTypes = 0;

	int sensorStatus = tdSensor(protocol, DATA_LENGTH, model, DATA_LENGTH, &sensorId, &dataTypes);
	if(sensorStatus == 0){
		printf("\n\nSENSORS:\n\n%-20s\t%-20s\t%-5s\t%-5s\t%-8s\t%-20s\t%-20s\t%-20s\n", "PROTOCOL", "MODEL", "ID", "TEMP", "HUMIDITY", "RAIN", "WIND", "LAST UPDATED");
	}
	while(sensorStatus == 0){
		char tempvalue[DATA_LENGTH];
		tempvalue[0] = 0;
		char humidityvalue[DATA_LENGTH];
		humidityvalue[0] = 0;
		char windvalue[DATA_LENGTH];
		windvalue[0] = 0;
		char winddirectionvalue[DATA_LENGTH];
		winddirectionvalue[0] = 0;
		char windaveragevalue[DATA_LENGTH];
		windaveragevalue[0] = 0;
		char windgustvalue[DATA_LENGTH];
		windgustvalue[0] = 0;
		char rainvalue[DATA_LENGTH];
		rainvalue[0] = 0;
		char raintotvalue[DATA_LENGTH];
		raintotvalue[0] = 0;
		char rainratevalue[DATA_LENGTH];
		rainratevalue[0] = 0;
		char timeBuf[80];
		timeBuf[0] = 0;
		time_t timestamp = 0;

		if (dataTypes & TELLSTICK_TEMPERATURE) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_TEMPERATURE, tempvalue, DATA_LENGTH, (int *)&timestamp);
			strcat(tempvalue, DEGREE);
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_HUMIDITY) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_HUMIDITY, humidityvalue, DATA_LENGTH, (int *)&timestamp);
			strcat(humidityvalue, "%");
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_RAINRATE) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_RAINRATE, rainratevalue, DATA_LENGTH, (int *)&timestamp);
			strcat(rainratevalue, " mm/h, ");
			strcat(rainvalue, rainratevalue);
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_RAINTOTAL) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_RAINTOTAL, raintotvalue, DATA_LENGTH, (int *)&timestamp);
			//TODO detta blir väl fel, kan väl hamna i andra ordningar, eller hur?
			strcat(raintotvalue, " mm");
			strcat(rainvalue, raintotvalue);
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_WINDDIRECTION) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDDIRECTION, winddirectionvalue, DATA_LENGTH, (int *)&timestamp);
			//TODO or use charToInteger in common?
			std::stringstream inputstream;
			inputstream << winddirectionvalue;
			int direction;
			inputstream >> direction;
			direction = direction / 22.5;
			std::string directionabbrev = "N";
			switch (direction) {
				case 1:
					directionabbrev = "NNE";
					break;
				case 2:
					directionabbrev = "NE";
					break;
				case 3:
					directionabbrev = "ENE";
					break;
				case 4:
					directionabbrev = "E";
					break;
				case 5:
					directionabbrev = "ESE";
					break;
				case 6:
					directionabbrev = "SE";
					break;
				case 7:
					directionabbrev = "SSE";
					break;
				case 8:
					directionabbrev = "S";
					break;
				case 9:
					directionabbrev = "SSW";
					break;
				case 10:
					directionabbrev = "SW";
					break;
				case 11:
					directionabbrev = "WSW";
					break;
				case 12:
					directionabbrev = "W";
					break;
				case 13:
					directionabbrev = "WNW";
					break;
				case 14:
					directionabbrev = "NW";
					break;
				case 15:
					directionabbrev = "NNW";
					break;
			}
			strcat(windvalue, directionabbrev.c_str());
			strcat(windvalue, ", ");
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_WINDAVERAGE) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDAVERAGE, windaveragevalue, DATA_LENGTH, (int *)&timestamp);
			strcat(windaveragevalue, " m/s (");
			strcat(windvalue, windaveragevalue);
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		if (dataTypes & TELLSTICK_WINDGUST) {
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDGUST, windgustvalue, DATA_LENGTH, (int *)&timestamp);
			strcat(windgustvalue, " m/s) ");
			strcat(windvalue, windgustvalue);
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
		}

		printf("%-20s\t%-20s\t%-5i\t%-5s\t%-8s\t%-20s\t%-20s\t%-20s\n", protocol, model, sensorId, tempvalue, humidityvalue, rainvalue, windvalue, timeBuf);

		sensorStatus = tdSensor(protocol, DATA_LENGTH, model, DATA_LENGTH, &sensorId, &dataTypes);
	}
	printf("\n");
	if(sensorStatus != TELLSTICK_ERROR_DEVICE_NOT_FOUND){
		char *errorString = tdGetErrorString(sensorStatus);
		fprintf(stderr, "Error fetching sensors: %s\n", errorString);
		tdReleaseString(errorString);
		return sensorStatus;
	}
	return TELLSTICK_SUCCESS;
}

/* list sensors using key=value format, one sensor/line, no header lines
 * and no degree or percent signs attached to the numbers - just
 * plain values. */
int list_kv_sensors() {
	char protocol[DATA_LENGTH], model[DATA_LENGTH];

	tdInit();
	int sensorId = 0, dataTypes = 0;
	time_t now = 0;
	int sensorStatus;

	time(&now);
	while(1) {
		sensorStatus = tdSensor(protocol, DATA_LENGTH, model, DATA_LENGTH, &sensorId, &dataTypes);
		if (sensorStatus != 0) break;

		printf("type=sensor\tprotocol=%s\tmodel=%s\tid=%d",
			protocol, model, sensorId);

		time_t timestamp = 0;

		if (dataTypes & TELLSTICK_TEMPERATURE) {
			char tempvalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_TEMPERATURE, tempvalue, DATA_LENGTH, (int *)&timestamp);
			printf("\ttemperature=%s", tempvalue);
		}

		if (dataTypes & TELLSTICK_HUMIDITY) {
			char humidityvalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_HUMIDITY, humidityvalue, DATA_LENGTH, (int *)&timestamp);
			printf("\thumidity=%s", humidityvalue);
		}

		if (dataTypes & TELLSTICK_WINDDIRECTION) {
			char winddirectionvalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDDIRECTION, winddirectionvalue, DATA_LENGTH, (int *)&timestamp);
			printf("\twinddirection=%s", winddirectionvalue);
		}

		if (dataTypes & TELLSTICK_WINDAVERAGE) {
			char windaveragevalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDAVERAGE, windaveragevalue, DATA_LENGTH, (int *)&timestamp);
			printf("\twindaverage=%s", windaveragevalue);
		}

		if (dataTypes & TELLSTICK_WINDGUST) {
			char windgustvalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_WINDGUST, windgustvalue, DATA_LENGTH, (int *)&timestamp);
			printf("\twindgust=%s", windgustvalue);
		}

		if (dataTypes & TELLSTICK_RAINRATE) {
			char rainratevalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_RAINRATE, rainratevalue, DATA_LENGTH, (int *)&timestamp);
			printf("\trainrate=%s", rainratevalue);
		}

		if (dataTypes & TELLSTICK_RAINTOTAL) {
			char raintotalvalue[DATA_LENGTH];
			tdSensorValue(protocol, model, sensorId, TELLSTICK_RAINTOTAL, raintotalvalue, DATA_LENGTH, (int *)&timestamp);
			printf("\traintotal=%s", raintotalvalue);
		}

		if (dataTypes & (TELLSTICK_TEMPERATURE | TELLSTICK_HUMIDITY)) {
			/* timestamp has been set, print time & age */
			/* (age is more useful on e.g. embedded systems
			 * which may not have real-time clock chips =>
			 * time is useful only as a relative value) */
			char timeBuf[80];
			strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
			printf("\ttime=%s\tage=%d", timeBuf, (int)(now - timestamp));
		}
		printf("\n");

	}
	if(sensorStatus != TELLSTICK_ERROR_DEVICE_NOT_FOUND){
		char *errorString = tdGetErrorString(sensorStatus);
		fprintf(stderr, "Error fetching sensors: %s\n", errorString);
		tdReleaseString(errorString);
		return sensorStatus;
	}
	return TELLSTICK_SUCCESS;
}

/* list devices using key=value format, one device/line, no header lines */
int list_kv_devices() {
	tdInit();
	int intNum = tdGetNumberOfDevices();
	if (intNum < 0) {
		char *errorString = tdGetErrorString(intNum);
		fprintf(stderr, "Error fetching devices: %s\n", errorString);
		tdReleaseString(errorString);
		return intNum;
	}
	int index = 0;
	while (index < intNum) {
		tdInit();
		int intId = tdGetDeviceId(index);
		char *name = tdGetName(intId);
		printf("type=device\tid=%i\tname=%s", intId, name);
		tdReleaseString(name);

		int lastSentCommand = tdLastSentCommand(intId, SUPPORTED_METHODS);
		char *level = 0;
		switch(lastSentCommand) {
			case TELLSTICK_TURNON:
				printf("\tlastsentcommand=ON");
				break;
			case TELLSTICK_TURNOFF:
				printf("\tlastsentcommand=OFF");
				break;
			case TELLSTICK_DIM:
				level = tdLastSentValue(intId);
				printf("\tlastsentcommand=DIMMED\tdimlevel=%s", level);
				tdReleaseString(level);
				break;
			/* default: state is unknown, print nothing. */
		}
		printf("\n");
		index++;
	}
	return TELLSTICK_SUCCESS;
}


int find_device( char *device ) {
	tdInit();
	int deviceId = atoi(device);
	if (deviceId == 0) { //Try to find the id from the name
		int intNum = tdGetNumberOfDevices();
		int index = 0;
		while (index < intNum) {
			int id = tdGetDeviceId(index);
			char *name = tdGetName( id );
			if (strcasecmp(name, device) == 0) {
				deviceId = id;
 				tdReleaseString(name);
				break;
			}
			tdReleaseString(name);
			index++;
		}
	}
	return deviceId;
}

int switch_device( bool turnOn, char *device ) {
	tdInit();
	int deviceId = find_device( device );
	if (deviceId == 0) {
		printf("Device '%s', not found!\n", device);
		return TELLSTICK_ERROR_DEVICE_NOT_FOUND;
	}

	char *name = tdGetName( deviceId );
	int deviceType = tdGetDeviceType( deviceId );
	printf("Turning %s %s %i, %s",
					(turnOn ? "on" : "off"),
					(deviceType == TELLSTICK_TYPE_DEVICE ? "device" : "group"),
					deviceId,
					name);
	tdReleaseString(name);

	int retval = (turnOn ? tdTurnOn( deviceId ) : tdTurnOff( deviceId ));
	char *errorString = tdGetErrorString(retval);
	
	printf(" - %s\n", errorString);
	tdReleaseString(errorString);
	return retval;
}

int dim_device( char *device, int level ) {
	tdInit();
	int deviceId = find_device( device );
	if (deviceId == 0) {
		printf("Device '%s', not found!\n", device);
		return TELLSTICK_ERROR_DEVICE_NOT_FOUND;
	}
	if (level < 0 || level > 255) {
		printf("Level %i out of range!\n", level);
		return TELLSTICK_ERROR_SYNTAX;
	}

	char *name = tdGetName( deviceId );
	int retval = tdDim( deviceId, (unsigned char)level );
	char *errorString = tdGetErrorString(retval);
	printf("Dimming device: %i %s to %i - %s\n", deviceId, name, level, errorString);
	tdReleaseString(name);
	tdReleaseString(errorString);
	return retval;
}

int bell_device( char *device ) {
	tdInit();
	int deviceId = find_device( device );
	if (deviceId == 0) {
		printf("Device '%s', not found!\n", device);
		return TELLSTICK_ERROR_DEVICE_NOT_FOUND;
	}

	char *name = tdGetName( deviceId );
	int retval = tdBell( deviceId );
	char *errorString = tdGetErrorString(retval);
	printf("Sending bell to: %i %s - %s\n", deviceId, name, errorString);
	tdReleaseString(name);
	tdReleaseString(errorString);
	return retval;
}

int learn_device( char *device ) {
	tdInit();
	int deviceId = find_device( device );
	if (deviceId == 0) {
		printf("Device '%s', not found!\n", device);
		return TELLSTICK_ERROR_DEVICE_NOT_FOUND;
	}

	char *name = tdGetName( deviceId );
	int retval = tdLearn( deviceId );
	char *errorString = tdGetErrorString(retval);
	printf("Learning device: %i %s - %s\n", deviceId, name, errorString);
	tdReleaseString(name);
	tdReleaseString(errorString);
	return retval;
}

int send_raw_command( char *command ) {
	tdInit();
	const int MAX_LENGTH = 100;
	char msg[MAX_LENGTH];
	
	if (strcmp(command, "-") == 0) {
		fgets(msg, MAX_LENGTH, stdin);
	} else {
		FILE *fd;
		
		fd = fopen(command, "r");
		if (fd == NULL) {
			printf("Error opening file %s\n", command);
			return TELLSTICK_ERROR_UNKNOWN;
		}
		fgets(msg, MAX_LENGTH, fd);
		fclose(fd);
	}
	
	int retval = tdSendRawCommand( msg, 0 );	
	char *errorString = tdGetErrorString(retval);
	printf("Sending raw command: %s\n", errorString);
	tdReleaseString(errorString);
	return retval;
}

#define LIST_KV_SENSORS 1
#define LIST_KV_DEVICES 2

int main(int argc, char **argv)
{
	int optch, longindex;
	static char optstring[] = "ln:f:d:b:v:e:r:hi";
	static struct option long_opts[] = {
		{ "list", 0, 0, 'l' },
		{ "list-sensors", 0, 0, LIST_KV_SENSORS },
		{ "list-devices", 0, 0, LIST_KV_DEVICES },
		{ "on", 1, 0, 'n' },
		{ "off", 1, 0, 'f' },
		{ "dim", 1, 0, 'd' },
		{ "bell", 1, 0, 'b' },
		{ "dimlevel", 1, 0, 'v' },
		{ "learn", 1, 0, 'e' },
		{ "raw", 1, 0, 'r' },
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 'i'},
		{ 0, 0, 0, 0}
	};
	int level = -1;

	if (argc < 2) {
		print_usage( argv[0] );
		return -TELLSTICK_ERROR_SYNTAX;
	}

	int returnSuccess = 0;
	while ( (optch = getopt_long(argc,argv,optstring,long_opts,&longindex)) != -1 ){
		int success = 0;
		switch (optch) {
			case 'b' :
				success = bell_device( &optarg[0] );
				break;
			case 'd' :
				if (level >= 0) {
					success = dim_device( &optarg[0], level );
					break;
				}
				printf("Dim level missing or incorrect value.\n");
				success = TELLSTICK_ERROR_SYNTAX;
				break;
			case 'f' :
				success = switch_device(false, &optarg[0]);
				break;
			case 'h' :
				print_usage( argv[0] );
				success = TELLSTICK_SUCCESS;
				break;
			case 'i' :
				print_version( );
				success = TELLSTICK_SUCCESS;
				break;
			case 'l' :
				success = list_devices();
				break;
			case LIST_KV_SENSORS:
				success = list_kv_sensors();
				break;
			case LIST_KV_DEVICES:
				success = list_kv_devices();
				break;
			case 'n' :
				success = switch_device(true, &optarg[0]);
				break;
			case 'e' :
				success = learn_device(&optarg[0]);
				break;
			case 'r' :
				success = send_raw_command(&optarg[0]);
				break;
			case 'v' :
				level = atoi( &optarg[0] );
				break;
			default :
				print_usage( argv[0] );
				success = TELLSTICK_ERROR_SYNTAX;
		}
		if(success != TELLSTICK_SUCCESS){
			returnSuccess = success;  //return last error message
		}
	}
	tdClose(); //Cleaning up
	return -returnSuccess;
}
