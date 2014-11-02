#include "../client/telldus-core.h"
#include <stdlib.h>
#include <errno.h>
#include <argp.h>
#include <string>

const char *argp_program_version = "tdadmin " VERSION ;
const char *argp_program_bug_address = "<info.tech@telldus.com>";

static char args_doc[] = "COMMAND ACTION";

static char doc[] = "TellStick admin tool -- a command line utility to edit devices and controllers for Telldus TellStick";

const int VID = 1;
const int PID = 2;
const int SERIAL = 3;

static struct argp_option options[] = {
	{0,0,0,0,
		"COMMAND: controller, ACTION: connect/disconnect\n"
		"Tells the daemon to add or remove a TellStick (duo)"
	},
	{"vid",VID,"VID",0, "The vendor id (1781)" },
	{"pid",PID,"PID",0,"The product id (0c30 or 0c31)" },
	{"serial",SERIAL,"SERIAL",0,"The usb serial number" },
	{ 0 }
};

static std::string command, action;

int vid, pid;
static std::string serial;

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
	switch (key) {
		case PID:
			pid = strtol(arg, NULL, 16);
			break;
		case SERIAL:
			serial = arg;
			break;
		case VID:
			vid = strtol(arg, NULL, 16);
			break;

		case ARGP_KEY_NO_ARGS:
			argp_usage (state);

		case ARGP_KEY_ARG:
			if (state->next == state->argc) {
				argp_usage (state);
			}
			command = arg;
			action = state->argv[state->next];
			state->next = state->argc;

			break;

	default:
		return ARGP_ERR_UNKNOWN;
	 }
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void handle_controller(void) {
	if (vid == 0 || pid == 0) {
		fprintf(stderr, "Missing parameter vid or pid\n");
	}
	if (action.compare("connect") == 0) {
		tdConnectTellStickController(vid,pid,serial.c_str());

	} else if (action.compare("disconnect") == 0) {
		tdDisconnectTellStickController(vid,pid,serial.c_str());
	}
}

int main(int argc, char **argv) {

	argp_parse (&argp, argc, argv, 0, 0, 0);

	if (command.compare("controller") == 0) {
		handle_controller();
	}

	return 0;
}
