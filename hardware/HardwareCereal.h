#pragma once
#include "../main/RFXtrx.h"

#if 0
// usage example:

	_tTempBaro temperature;
	std::string serialized = domoticztypes::Serialize<_tTempBaro>(temperature);
	temperature = domoticztypes::Deserialize<_tTempBaro>(serialized);

#endif

namespace domoticztypes {
typedef unsigned char BYTE;

typedef struct {
	BYTE	packetlength;
	BYTE	packettype;
	BYTE	subtype;
	BYTE	seqnbr;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("packetlength", packetlength);
		ar & cereal::make_nvp("packettype", packettype);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("seqnbr", seqnbr);
	};
} _tR_HEADER;

typedef struct {
	BYTE	cmnd;
	BYTE	freqsel;
	BYTE	xmitpwr;
	BYTE	msg3;
	BYTE	msg4;
	BYTE	msg5;
	BYTE	msg6;
	BYTE	msg7;
	BYTE	msg8;
	BYTE	msg9;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("freqsel", freqsel);
		ar & cereal::make_nvp("xmitpwr", xmitpwr);
		ar & cereal::make_nvp("msg3", msg3);
		ar & cereal::make_nvp("msg4", msg4);
		ar & cereal::make_nvp("msg5", msg5);
		ar & cereal::make_nvp("msg6", msg6);
		ar & cereal::make_nvp("msg7", msg7);
		ar & cereal::make_nvp("msg8", msg8);
		ar & cereal::make_nvp("msg9", msg9);
	};
} _tR_ICMND;

typedef struct {	//response on a mode command from the application
	BYTE	cmnd;
	BYTE	msg1;	//receiver/transceiver type
	BYTE	msg2;	//firmware version

	BYTE	msg3;
	BYTE	msg4;
	BYTE	msg5;
	BYTE	msg6;
	BYTE	msg7;
	BYTE	msg8;
	BYTE	msg9;
	BYTE	msg10;
	BYTE	msg11;
	BYTE	msg12;
	BYTE	msg13;
	BYTE	msg14;
	BYTE	msg15;
	BYTE	msg16;
	template <class Archive>
	void serialize(Archive &ar) {	//response on a mode command from the application
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("msg1", msg1);	//receiver/transceiver type
		ar & cereal::make_nvp("msg2", msg2);	//firmware version
		ar & cereal::make_nvp("msg3", msg3);
		ar & cereal::make_nvp("msg4", msg4);
		ar & cereal::make_nvp("msg5", msg5);
		ar & cereal::make_nvp("msg6", msg6);
		ar & cereal::make_nvp("msg7", msg7);
		ar & cereal::make_nvp("msg8", msg8);
		ar & cereal::make_nvp("msg9", msg9);
		ar & cereal::make_nvp("msg10", msg10);
		ar & cereal::make_nvp("msg11", msg11);
		ar & cereal::make_nvp("msg12", msg12);
		ar & cereal::make_nvp("msg13", msg13);
		ar & cereal::make_nvp("msg14", msg14);
		ar & cereal::make_nvp("msg15", msg15);
		ar & cereal::make_nvp("msg16", msg16);
	};
} _tR_IRESPONSE;

typedef struct {	//response on a mode command from the application
	BYTE	cmnd;
	BYTE	msg1;	//receiver/transceiver type
	BYTE	msg2;	//firmware version

	BYTE	msg3;
	BYTE	msg4;
	BYTE	msg5;
	BYTE	msg6;
	BYTE	msg7;
	BYTE	msg8;
	BYTE	msg9;
	BYTE	msg10;
	BYTE	msg11;
	BYTE	msg12;
	BYTE	msg13;
	BYTE	msg14;
	BYTE	msg15;
	BYTE	msg16;
	template <class Archive>
	void serialize(Archive &ar) {	//response on a mode command from the application
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("msg1", msg1);	//receiver/transceiver type
		ar & cereal::make_nvp("msg2", msg2);	//firmware version
		ar & cereal::make_nvp("msg3", msg3);
		ar & cereal::make_nvp("msg4", msg4);
		ar & cereal::make_nvp("msg5", msg5);
		ar & cereal::make_nvp("msg6", msg6);
		ar & cereal::make_nvp("msg7", msg7);
		ar & cereal::make_nvp("msg8", msg8);
		ar & cereal::make_nvp("msg9", msg9);
		ar & cereal::make_nvp("msg10", msg10);
		ar & cereal::make_nvp("msg11", msg11);
		ar & cereal::make_nvp("msg12", msg12);
		ar & cereal::make_nvp("msg13", msg13);
		ar & cereal::make_nvp("msg14", msg14);
		ar & cereal::make_nvp("msg15", msg15);
		ar & cereal::make_nvp("msg16", msg16);
	};

} _tR_IRESPONSE868;

typedef struct {
	BYTE	msg1;
	BYTE	msg2;
	BYTE	msg3;
	BYTE	msg4;
	BYTE	msg5;
	BYTE	msg6;
	BYTE	msg7;
	BYTE	msg8;
	BYTE	msg9;
	BYTE	msg10;
	BYTE	msg11;
	BYTE	msg12;
	BYTE	msg13;
	BYTE	msg14;
	BYTE	msg15;
	BYTE	msg16;
	BYTE	msg17;
	BYTE	msg18;
	BYTE	msg19;
	BYTE	msg20;
	BYTE	msg21;
	BYTE	msg22;
	BYTE	msg23;
	BYTE	msg24;
	BYTE	msg25;
	BYTE	msg26;
	BYTE	msg27;
	BYTE	msg28;
	BYTE	msg29;
	BYTE	msg30;
	BYTE	msg31;
	BYTE	msg32;
	BYTE	msg33;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("msg1", msg1);
		ar & cereal::make_nvp("msg2", msg2);
		ar & cereal::make_nvp("msg3", msg3);
		ar & cereal::make_nvp("msg4", msg4);
		ar & cereal::make_nvp("msg5", msg5);
		ar & cereal::make_nvp("msg6", msg6);
		ar & cereal::make_nvp("msg7", msg7);
		ar & cereal::make_nvp("msg8", msg8);
		ar & cereal::make_nvp("msg9", msg9);
		ar & cereal::make_nvp("msg10", msg10);
		ar & cereal::make_nvp("msg11", msg11);
		ar & cereal::make_nvp("msg12", msg12);
		ar & cereal::make_nvp("msg13", msg13);
		ar & cereal::make_nvp("msg14", msg14);
		ar & cereal::make_nvp("msg15", msg15);
		ar & cereal::make_nvp("msg16", msg16);
		ar & cereal::make_nvp("msg17", msg17);
		ar & cereal::make_nvp("msg18", msg18);
		ar & cereal::make_nvp("msg19", msg19);
		ar & cereal::make_nvp("msg20", msg20);
		ar & cereal::make_nvp("msg21", msg21);
		ar & cereal::make_nvp("msg22", msg22);
		ar & cereal::make_nvp("msg23", msg23);
		ar & cereal::make_nvp("msg24", msg24);
		ar & cereal::make_nvp("msg25", msg25);
		ar & cereal::make_nvp("msg26", msg26);
		ar & cereal::make_nvp("msg27", msg27);
		ar & cereal::make_nvp("msg28", msg28);
		ar & cereal::make_nvp("msg29", msg29);
		ar & cereal::make_nvp("msg30", msg30);
		ar & cereal::make_nvp("msg31", msg31);
		ar & cereal::make_nvp("msg32", msg32);
		ar & cereal::make_nvp("msg33", msg33);
	};
} _tR_UNDECODED;

typedef struct {	//receiver/transmitter messages
	BYTE	msg;
	template <class Archive>
	void serialize(Archive &ar) {	//receiver/transmitter messages
		ar & cereal::make_nvp("msg", msg);
	};
} _tR_RXRESPONSE;

typedef struct {
	BYTE	housecode;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("housecode", housecode);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_LIGHTING1;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	level;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("level", level);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_LIGHTING2;

typedef struct {
	BYTE	system;
	BYTE	channel8_1;
	BYTE	channel10_9;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("system", system);
		ar & cereal::make_nvp("channel8_1", channel8_1);
		ar & cereal::make_nvp("channel10_9", channel10_9);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_LIGHTING3;

typedef struct {
	BYTE	cmd1;
	BYTE	cmd2;
	BYTE	cmd3;
	BYTE	pulseHigh;
	BYTE	pulseLow;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("cmd1", cmd1);
		ar & cereal::make_nvp("cmd2", cmd2);
		ar & cereal::make_nvp("cmd3", cmd3);
		ar & cereal::make_nvp("pulseHigh", pulseHigh);
		ar & cereal::make_nvp("pulseLow", pulseLow);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);

	};
} _tR_LIGHTING4;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	level;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("level", level);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);

	};
} _tR_LIGHTING5;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	groupcode;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	cmndseqnbr;
	BYTE	seqnbr2;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("groupcode", groupcode);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("cmndseqnbr", cmndseqnbr);
		ar & cereal::make_nvp("seqnbr2", seqnbr2);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_LIGHTING6;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	sound;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("sound", sound);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};

} _tR_CHIME;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_FAN;

typedef struct {
	BYTE	housecode;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("housecode", housecode);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler", filler);
	};
} _tR_CURTAIN1;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	unicode_id4;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("unicode_id4", unicode_id4);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_BLINDS1;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	rfu1;
	BYTE	rfu2;
	BYTE	rfu3;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("rfu1", rfu1);
		ar & cereal::make_nvp("rfu2", rfu2);
		ar & cereal::make_nvp("rfu3", rfu3);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_RFY;

typedef struct {
	BYTE id1;
	BYTE id2;
	BYTE id3;
	BYTE housecode;
	BYTE unitcode;
	BYTE cmnd;
	BYTE rfu1;
	BYTE rfu2;
	BYTE filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("housecode", housecode);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("rfu1", rfu1);
		ar & cereal::make_nvp("rfu2", rfu2);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};

} _tR_HOMECONFORT;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	groupcode;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	cmndtime;
	BYTE	devtype_filler;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("groupcode", groupcode);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("cmndtime", cmndtime);
		ar & cereal::make_nvp("devtype_filler", devtype_filler);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_FUNKBUS;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	status;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("status", status);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_SECURITY1;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	BYTE	id5;
	BYTE	id6;
	BYTE	id7;
	BYTE	id8;
	BYTE	id9;
	BYTE	id10;
	BYTE	id11;
	BYTE	id12;
	BYTE	id13;
	BYTE	id14;
	BYTE	id15;
	BYTE	id16;
	BYTE	id17;
	BYTE	id18;
	BYTE	id19;
	BYTE	id20;
	BYTE	id21;
	BYTE	id22;
	BYTE	id23;
	BYTE	id24;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("id5", id5);
		ar & cereal::make_nvp("id6", id6);
		ar & cereal::make_nvp("id7", id7);
		ar & cereal::make_nvp("id8", id8);
		ar & cereal::make_nvp("id9", id9);
		ar & cereal::make_nvp("id10", id10);
		ar & cereal::make_nvp("id11", id11);
		ar & cereal::make_nvp("id12", id12);
		ar & cereal::make_nvp("id13", id13);
		ar & cereal::make_nvp("id14", id14);
		ar & cereal::make_nvp("id15", id15);
		ar & cereal::make_nvp("id16", id16);
		ar & cereal::make_nvp("id17", id17);
		ar & cereal::make_nvp("id18", id18);
		ar & cereal::make_nvp("id19", id19);
		ar & cereal::make_nvp("id20", id20);
		ar & cereal::make_nvp("id21", id21);
		ar & cereal::make_nvp("id22", id22);
		ar & cereal::make_nvp("id23", id23);
		ar & cereal::make_nvp("id24", id24);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_SECURITY2;

typedef struct {
	BYTE	housecode;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("housecode", housecode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_CAMERA1;

typedef struct {
	BYTE	id;
	BYTE	cmnd;
	BYTE	toggle_cmndtype_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id", id);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("toggle_cmndtype_rssi", toggle_cmndtype_rssi);
	};
} _tR_REMOTE;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	temperature;
	BYTE	set_point;
	BYTE	status_filler_mode;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("temperature", temperature);
		ar & cereal::make_nvp("set_point", set_point);
		ar & cereal::make_nvp("status_filler_mode", status_filler_mode);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_THERMOSTAT1;

typedef struct {
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_THERMOSTAT2;

typedef struct {
	BYTE	unitcode1;
	BYTE	unitcode2;
	BYTE	unitcode3;
	BYTE	cmnd;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("unitcode1", unitcode1);
		ar & cereal::make_nvp("unitcode2", unitcode2);
		ar & cereal::make_nvp("unitcode3", unitcode3);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_THERMOSTAT3;

typedef struct {
	BYTE	unitcode1;
	BYTE	unitcode2;
	BYTE	unitcode3;
	BYTE	beep;
	BYTE	fan1_speed;
	BYTE	flame_power;
	BYTE	mode;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("unitcode1", unitcode1);
		ar & cereal::make_nvp("unitcode2", unitcode2);
		ar & cereal::make_nvp("unitcode3", unitcode3);
		ar & cereal::make_nvp("beep", beep);
		ar & cereal::make_nvp("fan1_speed", fan1_speed);
		ar & cereal::make_nvp("flame_power", flame_power);
		ar & cereal::make_nvp("mode", mode);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};

} _tR_THERMOSTAT4;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	BYTE	unitcode;
	BYTE	cmnd;
	BYTE	temperature;
	BYTE	tempPoint5;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("temperature", temperature);
		ar & cereal::make_nvp("tempPoint5", tempPoint5);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_RADIATOR1;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	sensor1h;
	BYTE	sensor1l;
	BYTE	sensor2h;
	BYTE	sensor2l;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("sensor1h", sensor1h);
		ar & cereal::make_nvp("sensor1l", sensor1l);
		ar & cereal::make_nvp("sensor2h", sensor2h);
		ar & cereal::make_nvp("sensor2l", sensor2l);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_BBQ;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	raintotal1;
	BYTE	raintotal2;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("raintotal1", raintotal1);
		ar & cereal::make_nvp("raintotal2", raintotal2);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_TEMP_RAIN;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};

} _tR_TEMP;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	humidity;
	BYTE	humidity_status;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("humidity", humidity);
		ar & cereal::make_nvp("humidity_status", humidity_status);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_HUM;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	humidity;
	BYTE	humidity_status;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("humidity", humidity);
		ar & cereal::make_nvp("humidity_status", humidity_status);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_TEMP_HUM;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	baro1;
	BYTE	baro2;
	BYTE	forecast;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("baro1", baro1);
		ar & cereal::make_nvp("baro2", baro2);
		ar & cereal::make_nvp("forecast", forecast);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_BARO;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	humidity;
	BYTE	humidity_status;
	BYTE	baroh;
	BYTE	barol;
	BYTE	forecast;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("humidity", humidity);
		ar & cereal::make_nvp("humidity_status", humidity_status);
		ar & cereal::make_nvp("baroh", baroh);
		ar & cereal::make_nvp("barol", barol);
		ar & cereal::make_nvp("forecast", forecast);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_TEMP_HUM_BARO;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	rainrateh;
	BYTE	rainratel;
	BYTE	raintotal1;
	BYTE	raintotal2;
	BYTE	raintotal3;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("rainrateh", rainrateh);
		ar & cereal::make_nvp("rainratel", rainratel);
		ar & cereal::make_nvp("raintotal1", raintotal1);
		ar & cereal::make_nvp("raintotal2", raintotal2);
		ar & cereal::make_nvp("raintotal3", raintotal3);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_RAIN;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	directionh;
	BYTE	directionl;
	BYTE	av_speedh;
	BYTE	av_speedl;
	BYTE	gusth;
	BYTE	gustl;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	chillh_chillsign;
	BYTE	chilll;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("directionh", directionh);
		ar & cereal::make_nvp("directionl", directionl);
		ar & cereal::make_nvp("av_speedh", av_speedh);
		ar & cereal::make_nvp("av_speedl", av_speedl);
		ar & cereal::make_nvp("gusth", gusth);
		ar & cereal::make_nvp("gustl", gustl);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("chillh_chillsign", chillh_chillsign);
		ar & cereal::make_nvp("chilll", chilll);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_WIND;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	uv;
	BYTE	temperatureh_tempsignl;
	BYTE	temperaturel;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("uv", uv);
		ar & cereal::make_nvp("temperatureh_tempsignl", temperatureh_tempsignl);
		ar & cereal::make_nvp("temperaturel", temperaturel);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_UV;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	yy;
	BYTE	mm;
	BYTE	dd;
	BYTE	dow;
	BYTE	hr;
	BYTE	min;
	BYTE	sec;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("yy", yy);
		ar & cereal::make_nvp("mm", mm);
		ar & cereal::make_nvp("dd", dd);
		ar & cereal::make_nvp("dow", dow);
		ar & cereal::make_nvp("hr", hr);
		ar & cereal::make_nvp("min", min);
		ar & cereal::make_nvp("sec", sec);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_DT;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	count;
	BYTE	ch1h;
	BYTE	ch1l;
	BYTE	ch2h;
	BYTE	ch2l;
	BYTE	ch3h;
	BYTE	ch3l;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("count", count);
		ar & cereal::make_nvp("ch1h", ch1h);
		ar & cereal::make_nvp("ch1l", ch1l);
		ar & cereal::make_nvp("ch2h", ch2h);
		ar & cereal::make_nvp("ch2l", ch2l);
		ar & cereal::make_nvp("ch3h", ch3h);
		ar & cereal::make_nvp("ch3l", ch3l);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_CURRENT;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	count;
	BYTE	instant1;
	BYTE	instant2;
	BYTE	instant3;
	BYTE	instant4;
	BYTE	total1;
	BYTE	total2;
	BYTE	total3;
	BYTE	total4;
	BYTE	total5;
	BYTE	total6;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("count", count);
		ar & cereal::make_nvp("instant1", instant1);
		ar & cereal::make_nvp("instant2", instant2);
		ar & cereal::make_nvp("instant3", instant3);
		ar & cereal::make_nvp("instant4", instant4);
		ar & cereal::make_nvp("total1", total1);
		ar & cereal::make_nvp("total2", total2);
		ar & cereal::make_nvp("total3", total3);
		ar & cereal::make_nvp("total4", total4);
		ar & cereal::make_nvp("total5", total5);
		ar & cereal::make_nvp("total6", total6);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_ENERGY;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	count;
	BYTE	ch1h;
	BYTE	ch1l;
	BYTE	ch2h;
	BYTE	ch2l;
	BYTE	ch3h;
	BYTE	ch3l;
	BYTE	total1;
	BYTE	total2;
	BYTE	total3;
	BYTE	total4;
	BYTE	total5;
	BYTE	total6;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("count", count);
		ar & cereal::make_nvp("ch1h", ch1h);
		ar & cereal::make_nvp("ch1l", ch1l);
		ar & cereal::make_nvp("ch2h", ch2h);
		ar & cereal::make_nvp("ch2l", ch2l);
		ar & cereal::make_nvp("ch3h", ch3h);
		ar & cereal::make_nvp("ch3l", ch3l);
		ar & cereal::make_nvp("total1", total1);
		ar & cereal::make_nvp("total2", total2);
		ar & cereal::make_nvp("total3", total3);
		ar & cereal::make_nvp("total4", total4);
		ar & cereal::make_nvp("total5", total5);
		ar & cereal::make_nvp("total6", total6);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_CURRENT_ENERGY;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	voltage;
	BYTE	currentH;
	BYTE	currentL;
	BYTE	powerH;
	BYTE	powerL;
	BYTE	energyH;
	BYTE	energyL;
	BYTE	pf;
	BYTE	freq;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("voltage", voltage);
		ar & cereal::make_nvp("currentH", currentH);
		ar & cereal::make_nvp("currentL", currentL);
		ar & cereal::make_nvp("powerH", powerH);
		ar & cereal::make_nvp("powerL", powerL);
		ar & cereal::make_nvp("energyH", energyH);
		ar & cereal::make_nvp("energyL", energyL);
		ar & cereal::make_nvp("pf", pf);
		ar & cereal::make_nvp("freq", freq);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_POWER;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	weighthigh;
	BYTE	weightlow;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("weighthigh", weighthigh);
		ar & cereal::make_nvp("weightlow", weightlow);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_WEIGHT;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	BYTE	id5;
	BYTE	contract_type;
	BYTE	counter1_0;
	BYTE	counter1_1;
	BYTE	counter1_2;
	BYTE	counter1_3;
	BYTE	counter2_0;
	BYTE	counter2_1;
	BYTE	counter2_2;
	BYTE	counter2_3;
	BYTE	power_H;
	BYTE	power_L;
	BYTE	state;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("id5", id5);
		ar & cereal::make_nvp("contract_type", contract_type);
		ar & cereal::make_nvp("counter1_0", counter1_0);
		ar & cereal::make_nvp("counter1_1", counter1_1);
		ar & cereal::make_nvp("counter1_2", counter1_2);
		ar & cereal::make_nvp("counter1_3", counter1_3);
		ar & cereal::make_nvp("counter2_0", counter2_0);
		ar & cereal::make_nvp("counter2_1", counter2_1);
		ar & cereal::make_nvp("counter2_2", counter2_2);
		ar & cereal::make_nvp("counter2_3", counter2_3);
		ar & cereal::make_nvp("power_H", power_H);
		ar & cereal::make_nvp("power_L", power_L);
		ar & cereal::make_nvp("state", state);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_TIC;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	BYTE	counter1_0;
	BYTE	counter1_1;
	BYTE	counter1_2;
	BYTE	counter1_3;
	BYTE	counter2_0;
	BYTE	counter2_1;
	BYTE	counter2_2;
	BYTE	counter2_3;
	BYTE	state;
	BYTE	batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("counter1_0", counter1_0);
		ar & cereal::make_nvp("counter1_1", counter1_1);
		ar & cereal::make_nvp("counter1_2", counter1_2);
		ar & cereal::make_nvp("counter1_3", counter1_3);
		ar & cereal::make_nvp("counter2_0", counter2_0);
		ar & cereal::make_nvp("counter2_1", counter2_1);
		ar & cereal::make_nvp("counter2_2", counter2_2);
		ar & cereal::make_nvp("counter2_3", counter2_3);
		ar & cereal::make_nvp("state", state);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_CEENCODER;

typedef struct {
	BYTE id1;
	BYTE id2;
	BYTE id3;
	BYTE id4;
	BYTE runidx_0;
	BYTE runidx_1;
	BYTE runidx_2;
	BYTE runidx_3;
	BYTE prodidx1_0;
	BYTE prodidx1_1;
	BYTE prodidx1_2;
	BYTE prodidx1_3;
	BYTE currentidx_rfu;
	BYTE av_voltage;
	BYTE power_H;
	BYTE power_L;
	BYTE state;
	BYTE batterylevel_filler;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("runidx_0", runidx_0);
		ar & cereal::make_nvp("runidx_1", runidx_1);
		ar & cereal::make_nvp("runidx_2", runidx_2);
		ar & cereal::make_nvp("runidx_3", runidx_3);
		ar & cereal::make_nvp("prodidx1_0", prodidx1_0);
		ar & cereal::make_nvp("prodidx1_1", prodidx1_1);
		ar & cereal::make_nvp("prodidx1_2", prodidx1_2);
		ar & cereal::make_nvp("prodidx1_3", prodidx1_3);
		ar & cereal::make_nvp("currentidx_rfu", currentidx_rfu);
		ar & cereal::make_nvp("av_voltage", av_voltage);
		ar & cereal::make_nvp("power_H", power_H);
		ar & cereal::make_nvp("power_L", power_L);
		ar & cereal::make_nvp("state", state);
		ar & cereal::make_nvp("batterylevel_filler", batterylevel_filler);
	};
} _tR_LINKY;

typedef struct {
	BYTE cmnd;
	BYTE baudrate;
	BYTE parity;
	BYTE databits;
	BYTE stopbits;
	BYTE polarity;
	BYTE filler1;
	BYTE filler2;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("baudrate", baudrate);
		ar & cereal::make_nvp("parity", parity);
		ar & cereal::make_nvp("databits", databits);
		ar & cereal::make_nvp("stopbits", stopbits);
		ar & cereal::make_nvp("polarity", polarity);
		ar & cereal::make_nvp("filler1", filler1);
		ar & cereal::make_nvp("filler2", filler2);
	};
} _tR_ASYNCPORT;

typedef struct {
	BYTE datachar[252];
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("datachar", datachar);
	};
} _tR_ASYNCDATA;

typedef struct {
	BYTE	id;
	BYTE	msg1;
	BYTE	msg2;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id", id);
		ar & cereal::make_nvp("msg1", msg1);
		ar & cereal::make_nvp("msg2", msg2);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_RFXSENSOR;

typedef struct {
	BYTE	id1;
	BYTE	id2;
	BYTE	count1;
	BYTE	count2;
	BYTE	count3;
	BYTE	count4;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("count1", count1);
		ar & cereal::make_nvp("count2", count2);
		ar & cereal::make_nvp("count3", count3);
		ar & cereal::make_nvp("count4", count4);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_RFXMETER;

typedef struct {
	BYTE	hc1;
	BYTE	hc2;
	BYTE	addr;
	BYTE	cmd1;
	BYTE	cmd2;
	BYTE	filler_rssi;
	template <class Archive>
	void serialize(Archive &ar) {
		ar & cereal::make_nvp("hc1", hc1);
		ar & cereal::make_nvp("hc2", hc2);
		ar & cereal::make_nvp("addr", addr);
		ar & cereal::make_nvp("cmd1", cmd1);
		ar & cereal::make_nvp("cmd2", cmd2);
		ar & cereal::make_nvp("filler_rssi", filler_rssi);
	};
} _tR_FS20;
/*
typedef struct {
BYTE	repeat;
typedef struct{
BYTE	uint_msb;
BYTE	uint_lsb;
} _tR_pulse[124];
template <class Archive>
void serialize (Archive &ar) {
ar & cereal::make_nvp("repeat", repeat);
template <class Archive>
void serialize (Archive &ar){
ar & cereal::make_nvp("uint_msb", uint_msb);
ar & cereal::make_nvp("uint_lsb", uint_lsb);
};
};
} _tR_RAW;
*/

typedef struct {
	_tR_HEADER HEADER;
	union {
		_tR_ICMND ICMND;
		_tR_IRESPONSE IRESPONSE;
		_tR_IRESPONSE868 IRESPONSE868;
		_tR_UNDECODED UNDECODED;
		_tR_RXRESPONSE RXRESPONSE;
		_tR_LIGHTING1 LIGHTING1;
		_tR_LIGHTING2 LIGHTING2;
		_tR_LIGHTING3 LIGHTING3;
		_tR_LIGHTING4 LIGHTING4;
		_tR_LIGHTING5 LIGHTING5;
		_tR_LIGHTING6 LIGHTING6;
		_tR_CHIME CHIME;
		_tR_FAN FAN;
		_tR_CURTAIN1 CURTAIN1;
		_tR_BLINDS1 BLINDS1;
		_tR_RFY RFY;
		_tR_HOMECONFORT HOMECONFORT;
		_tR_FUNKBUS FUNKBUS;
		_tR_SECURITY1 SECURITY1;
		_tR_SECURITY2 SECURITY2;
		_tR_CAMERA1 CAMERA1;
		_tR_REMOTE REMOTE;
		_tR_THERMOSTAT1 THERMOSTAT1;
		_tR_THERMOSTAT2 THERMOSTAT2;
		_tR_THERMOSTAT3 THERMOSTAT3;
		_tR_THERMOSTAT4 THERMOSTAT4;
		_tR_RADIATOR1 RADIATOR1;
		_tR_BBQ BBQ;
		_tR_TEMP_RAIN TEMP_RAIN;
		_tR_TEMP TEMP;
		_tR_HUM HUM;
		_tR_TEMP_HUM TEMP_HUM;
		_tR_BARO BARO;
		_tR_TEMP_HUM_BARO TEMP_HUM_BARO;
		_tR_RAIN RAIN;
		_tR_WIND WIND;
		_tR_UV UV;
		_tR_DT DT;
		_tR_CURRENT CURRENT;
		_tR_ENERGY ENERGY;
		_tR_CURRENT_ENERGY CURRENT_ENERGY;
		_tR_POWER POWER;
		_tR_WEIGHT WEIGHT;
		_tR_TIC TIC;
		_tR_CEENCODER CEENCODER;
		_tR_LINKY LINKY;
		_tR_ASYNCPORT ASYNCPORT;
		_tR_ASYNCDATA ASYNCDATA;
		_tR_RFXSENSOR RFXSENSOR;
		_tR_RFXMETER RFXMETER;
	};
} _tRDomoticz;


typedef union {
		RBUF rfbuf;
		_tRDomoticz domoticz;
} _tRDOMOTICZRBUF;

/* serializes a struct to a std::string */
template <typename StructType>
std::string Serialize(StructType buf)
{
	std::stringstream stream;
	{ // we start a new scope, so the archive flushes to the stream upon destruction
		cereal::PortableBinaryOutputArchive oarchive(stream);
		oarchive(buf);
	}
	return stream.str();
};

/* deserializes a std::string to a struct */
template <typename StructType>
StructType Deserialize(std::string str)
{
	std::stringstream stream(str);
	cereal::PortableBinaryInputArchive iarchive(stream);
	StructType result;
	iarchive(result);
	return result;
};


}