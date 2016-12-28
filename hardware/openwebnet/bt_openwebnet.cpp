//bt_openwebnet.cpp
//class bt_openwebnet is a modification of GNU bticino C++ openwebnet client
//from openwebnet class
//see www.bticino.it; www.myhome-bticino.it
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include "bt_openwebnet.h"
// private methods ......

string bt_openwebnet::FirstToken(string myText, string delimiters)
{
	std::istringstream iss(myText);
	std::string token;
	if (std::getline(iss, token, (char)delimiters.at(0)))
	{
		return token;
	}
	return "";
}


// performs syntax checking
void bt_openwebnet::IsCorrect()
{
  int j  = 0;
  string sup;
  string field;

  // if frame ACK -->
  if (frame_open.compare(OPENWEBNET_MSG_OPEN_OK) == 0)
  {
    frame_type = OK_FRAME;
    return;
  }

  // if frame NACK -->
  if (frame_open.compare(OPENWEBNET_MSG_OPEN_KO) == 0)
  {
    frame_type = KO_FRAME;
    return;
  }


  //if the first character is not *
  //or the frame is too long
  //or the frame does not end with two '#'
  if ((frame_open[0] != '*') ||
      (length_frame_open >	MAX_LENGTH_OPEN) ||
      (frame_open[length_frame_open-1] != '#') ||
      (frame_open[length_frame_open-2] != '#'))
  {
    frame_type = ERROR_FRAME;
    return;
  }

  //Check if there are bad character
  for (j=0;j<length_frame_open;j++)
  {
    if(!isdigit(frame_open[j]))
    {
      if((frame_open[j] != '*') && (frame_open[j] != '#'))
      {
        frame_type = ERROR_FRAME;
        return;
      }
    }
  }

  // normal frame...
  //*who#address*what*where#level#address*when##
  if (frame_open[1] != '#')
  {
    frame_type = NORMAL_FRAME;
    //extract the various fields of the open frame in the first mode (who + address and where + level + interface)
    Set_who_what_where_when();
    //extract any address values
    Set_address();
    //extract any level values and interface
    Set_level_interface();
    return;
  }

  // frame password ...
  //*#pwd##
  if(frame_open.find('*', 2) == string::npos)
  {
    frame_type = PWD_FRAME;
    //I extract the who
    Set_who();
    return;
  }

  //for other types of frames
  sup = frame_open.substr(2);
  field = FirstToken(sup, "*");
  sup = frame_open.substr(2 + field.length() + 1);
  if (sup.at(0) != '*')
  {
	  field = field + FirstToken(sup, "*");
	  sup = frame_open.substr(2 + field.length() + 1);
  }

  //frame request was ...
  //*#who*where##
  if(sup[0] != '*')
  {
    frame_type = STATE_FRAME;
	//extract the various fields of the open frame in the first mode (who + address and where + level + interface)
    Set_who_where();
	//extract any address values
	Set_address();
	//extract any level values and interface
	Set_level_interface();
    return;
  }
  else
  {
    //frame measurement request ...
    //*#who*where*dimension## or *#who*where*dimension*valueN##
    //or *#who**dimension*value1*value2*value3*value4## example: *#13**0*01*40*07*001##
    if(sup[1] != '#')
    {
      frame_type = MEASURE_FRAME;
      //extract the various fields of the open frame in the first mode (who + address and where + level + interface)
	  Set_who_where_dimension();
	  //extract any address values
      Set_address();
	  //extract any level values and interface
      Set_level_interface();
      return;
    }
    //writing dimension frame ...
    //*#who*where*#dimension*valueN##
    else
    {
      frame_type = WRITE_FRAME;
      //extract the different fields of the open frame in the first mode (who+address and where+level+interface)
      Set_who_where_dimension_value();
	  //extract any address values
      Set_address();
	  //extract any level values and interface
      Set_level_interface();
      return;
    }
  }

  // frame errata !!!
  frame_type = ERROR_FRAME;
  return;
}

// assigns who where dimension required for the request dimension frame
void bt_openwebnet::Set_who_where_dimension()
{
  string sup;
  size_t len = 0;

  // WHO
  sup = frame_open.substr(2);
  if (sup.at(0) != '*') {
	  who = FirstToken(sup, "*");
  }
  // WHERE
  sup = frame_open.substr(2 + who.length() + 1);
  if(sup.find("*") == string::npos)
    where = sup.substr(0, sup.length()-2);
  else
  {
    if(sup.at(0) != '*')
      where = FirstToken(sup, "*");
    // DIMENSION
    sup = frame_open.substr(2+who.length()+1+where.length()+1);
    if(sup.find("*") == string::npos)
		dimension = sup.substr(0, sup.length() - 2);
    else
    {
		if (sup.at(0) != '*') {
			dimension = FirstToken(sup, "*");
		}
      // VALUES **##
      sup = frame_open.substr(2+ who.length() + 1 + where.length() + 1 +dimension.length()+1);
      len = 2 + who.length() + 1 + where.length() + 1 + dimension.length() + 1;
      while ((sup.find("*") != string::npos) && (sup.at(0) != '*'))
      {
        string strValue = FirstToken(sup, "*");
		value.push_back(strValue);
        len = len+ strValue.length()+1;
        sup = frame_open.substr(len);
      }
      if ((sup[0] != '*') && (sup[0] != '#'))
      {
		  string strValue = sup.substr(0, sup.length()-2);
		  value.push_back(strValue);
      }
    }
  }

  return;
}

// assigns who and where required for the request dimension frame
void bt_openwebnet::Set_who_where()
{
  string sup;

  // WHO
  sup = frame_open.substr(2);
  if (sup.at(0) != '*') {
	  who = FirstToken(sup, "*");
  }
  // WHERE
  sup = frame_open.substr(2 + who.length() + 1);
  if (sup.find("*") == string::npos) {
	  where = sup.substr(0, sup.length() - 2);
  }
  else {
	  if (sup.at(0) != '*') {
		  where = FirstToken(sup, "*");
	  }
  }

  return;
}

// assigns who what where and when for the normal frames
void bt_openwebnet::Set_who_what_where_when()
{
	string sup;

	// who
	sup = frame_open.substr(1);
	if (sup.at(0) != '*') {
		who = FirstToken(sup, "*");
	}
	// what
	sup = frame_open.substr(1 + who.length() + 1);
	if (sup.find("*") == string::npos) {
		what = sup.substr(0, sup.length() - 2);
	}
	else {
		if (sup[0] != '*')
			what = FirstToken(sup, "*");
		// where
		sup = frame_open.substr(1 + who.length() + 1 + what.length() + 1);
		if (sup.find("*") == string::npos) {
			where = sup.substr(0, sup.length() - 2);
		}
		else
		{
			if (sup[0] != '*')
				where = FirstToken(sup, "*");
			// when
			sup = frame_open.substr(1 + who.length() + 1 + what.length() + 1 + where.length() + 1);
			if (sup.find("*") == string::npos) {
				when = sup.substr(0, sup.length() - 2);
			}
			else
				if (sup[0] != '*')
					when = FirstToken(sup, "*");
		}
	}

	return;
}

// assigns who where dimension and values for the write dimension frame
void bt_openwebnet::Set_who_where_dimension_value()
{
	string sup;
  int len = 0;
  int i = -1;

  // WHO
  sup = frame_open.substr(2);
  if(sup[0] != '*')
    who = FirstToken(sup, "*");
  // WHERE
  sup =  frame_open.substr(2+who.length()+1);
  if(sup.find('*') == string::npos)
    where = sup.substr(0,sup.length()-2);
  else
  {
    if(sup.at(0) != '*')
      where =FirstToken(sup, "*");
    // DIMENSION
    sup = frame_open.substr(2+who.length()+1+where.length()+2);
    if(sup.find('*') == string::npos)
      frame_type = ERROR_FRAME;
    else
    {
      if(sup.at(0) != '*')
        dimension = FirstToken(sup, "*");
      // VALUES
	  len = 2 + who.length() + 1 + where.length() + 2 + dimension.length() + 1;
	  sup = frame_open.substr(len);
      while (sup.find('*') != string::npos && (sup.at(0) != '*'))
      {
		  string newValue = FirstToken(sup, "*");
		  value.push_back(newValue);
			len = len+ newValue.length() +1;
			sup = frame_open.substr(len);
        while(sup.at(0) == '*')
        {
          len++;
		  sup = frame_open.substr(len);
		  value.push_back("");
        }
        if (sup.at(0) != '*')
			value.push_back("");
      }
      if ((sup.at(0) != '*') && (sup.at(0) != '#'))
      {
		  value.push_back(sup.substr(0, sup.length()-2));
      }
    }
  }

  return;
}

// assignes who for the frame processing password
void bt_openwebnet::Set_who()
{
	string sup;

	// WHO
	sup = frame_open.substr(2);
	if (sup.at(0) != '#') {
		who = FirstToken(sup, "#");
	}
	else {
		frame_type = ERROR_FRAME;
	}

	sup = frame_open.substr(2+who.length());
	if (sup.at(1) != '#') {
		frame_type = ERROR_FRAME;
	}

  return;
}

//assign level interface for extended frame
void bt_openwebnet::Set_level_interface()
{
  string sup;
  string orig;

  if (!where.length()) return;

  // WHERE
  if (where.at(0) == '#')
	  sup = where.substr(1);
  else
	  sup = where;
  orig = where;
  if(sup.find('#') != string::npos)
  {
    extended = true;
    if(orig.at(0) == '#')
      where = "#" +  FirstToken(sup, "#");
    else
      where = FirstToken(sup, "#");
    // LEVEL + INTERFACE
    sup= orig.substr(where.length()+1);
    if(sup.find('#') != string::npos)
    {
      level = FirstToken(sup, "#");
      sInterface = orig.substr(where.length()+1+level.length()+1);
      if(sInterface.length() == 0)
        frame_type = ERROR_FRAME;
    }
    else
      //Modified by Bt_vctMM for "accensione telecamere esetrne" (*6*0*4000#2*##)
      //frame_type = ERROR_FRAME;
      level = sup;
  }

  return;
}

// assign address
void bt_openwebnet::Set_address()
{
	string sup;
	string orig;
	address.reserve(4);

	// WHO
	sup = who;
	orig = who;

	if (sup.find("#") != string::npos)
	{
		who = FirstToken(sup, "#");
		// ADDRESS
		address.clear();
		sup = orig.substr(who.length() + 1);
		if (sup.find('#') == string::npos)
		{
			// unique serial address
			if (sup.length() != 0)
				address.push_back(sup);
			else
				frame_type = ERROR_FRAME;
		}
		else
		{
		  // IP address
		  address[0] = FirstToken(sup, "#");
		  sup = orig.substr(who.length() + 1 + address[0].length() + 1);
		  if(sup.find('#') != string::npos)
		  {
			// IP address
			address[1] = FirstToken(sup, "#");
			sup = orig.substr(who.length() + 1 + address[0].length() + 1 + address[1].length() + 1);
			if(sup.find('#') != string::npos)
			{
			  // IP address
			  address[2] = FirstToken(sup, "#");
			  sup = orig.substr(who.length() + 1 + address[0].length() + 1 + address[1].length() + 1 + address[2].length() + 1);
			  if(sup.find('#') == string::npos)
			  {
				// IP address
				if(sup.length() != 0)
				  address[3] = sup;
				else
				  frame_type = ERROR_FRAME;
			  }
			  else
				frame_type = ERROR_FRAME;
			}
			else
			  frame_type = ERROR_FRAME;
		  }
		  else
			frame_type = ERROR_FRAME;
		}
  }

  return;
}

// public methods ......

// constructors
bt_openwebnet::bt_openwebnet()
{
  //call CreateNullMsgOpen() function
  CreateNullMsgOpen();
}


bt_openwebnet::bt_openwebnet(string message)
{
  //call CreateMsgOpen(string) function
  CreateMsgOpen(message);
}

bt_openwebnet::bt_openwebnet(int who, int where, int what)
{
	stringstream whoStr;
	stringstream whereStr;
	stringstream whatStr;

	whoStr << who;
	whatStr << what;
	whereStr<< where;

	CreateMsgOpen(whoStr.str().c_str(), whatStr.str().c_str(), whereStr.str().c_str(), "");
}


// creates the open message *who*what*where*when##
bt_openwebnet::bt_openwebnet(string who, string what, string where, string when)
{
	//call CreateMsgOpen function
  CreateMsgOpen(who, what, where, when);
}


// destructor
bt_openwebnet::~bt_openwebnet()
{
}

// creates the open message *who*what*where*when##
void bt_openwebnet::CreateMsgOpen(string who, string what,	string where,	string when)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the open message
  frame << "*";
  frame << who;  frame << "*";
  frame << what;  frame << "*";
  frame << where;
  //to remove the trailing asterisk
  if (when.length() != 0) {
	  frame << "*";
	  frame << when;
  }
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

// creates the open message *who*what*where#level#interface*when##
void bt_openwebnet::CreateMsgOpen(string who, string what, string where, string lev, string interf, string when)
{
  //call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the open message
  frame << "*";
  frame << who;  frame << "*";
  frame << what;  frame << "*";
  frame << where; frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interf;
  //to remove the trailing asterisk
  if (!when.empty()) {
	  frame << "*";
	  frame << when;
  }
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

// creates the open message ****##
void bt_openwebnet::CreateNullMsgOpen()
{
  //Counter to reset the values
  int i = 0;

  // clears everything
  frame_open = "";
  frame_type = NULL_FRAME;

  length_frame_open = 0;

  extended = false;

  // clears everything
  who = "";
  address.clear();
  what = "";
  where = "";
  level = "";
  sInterface = "";
  when = "";
  dimension = "";
  value.clear();
}

//creates the OPEN message *#who*where##
void bt_openwebnet::CreateStateMsgOpen(string who, string where)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the OPEN message
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

string bt_openwebnet::DeleteControlCharacters(string in_frame)
{
	string out_frame = in_frame;

	// delete control characters ....
	while ((out_frame.at(out_frame.length() - 1) == '\n') || ((out_frame.at(out_frame.length() - 1) == '\r')))
	{
		out_frame.erase(out_frame.length() - 1, 1);
	}

	return out_frame;
}

//creates the OPEN message *#who*where#level#interface##
void bt_openwebnet::CreateStateMsgOpen(string who, string where, string lev, string interf)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  frame << "#";
  frame << who;  frame << "*";
  frame << where;  frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interf;
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

//creates the OPEN message *#who*where#level#interface##
void bt_openwebnet::CreateTimeReqMsgOpen()
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  frame << "*#13**0##";
  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

// creates the OPEN message *#who*where*dimension##
void bt_openwebnet::CreateDimensionMsgOpen(string who, string where, string dimension)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the OPEN message
  frame << "*#";
  frame << who;  frame << "*";
  frame << where;
  //to remove the trailing asterisk
  if (!dimension.empty()) {
	  frame << "*";
	  frame << dimension;
  }
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

//creates the OPEN message *#who*where#level#interface*dimension##
void bt_openwebnet::CreateDimensionMsgOpen(string who, string where, string lev, string interf, string dimension)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the OPEN message
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interf;

  //to remove the trailing asterisk
  if (dimension.length() != 0)
	  frame << "*";
  frame << dimension; frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

//creates the OPEN message *#who*where*#dimension*val_1*val_2*...val_n##
void bt_openwebnet::CreateWrDimensionMsgOpen(string who, string where, string dimension, vector<string> value)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the OPEN message
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "*#";
  frame << dimension;
  for (vector<string>::iterator it = value.begin(); it != value.end(); it++)
  {
	  frame << "*";
	  frame << *it;
  }
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

//creates the OPEN message *#who*where#level#interface*#dimension*val_1*val_2*...val_n##
void bt_openwebnet::CreateWrDimensionMsgOpen(string who, string where, string lev, string interf, string dimension, vector<string> value)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  stringstream frame;

  // creates the OPEN message
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "*#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interf;

  //to remove the trailing asterisk
  if (!dimension.empty())
  {
	  frame << "*";
	  frame << dimension;
  }
  for (vector<string>::iterator it = value.begin(); it != value.end(); it++)
  {
	  frame << "*";
	  frame << *it;
  }
  frame << "##";

  frame_open = DeleteControlCharacters(frame.str());
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}


void bt_openwebnet::CreateMsgOpen(string message)
{
	//call CreateNullMsgOpen function
  CreateNullMsgOpen();

  // saves the type of frame and its length
  frame_open = message;

  frame_open = DeleteControlCharacters(frame_open);
  length_frame_open = frame_open.length();

  // checks for correct syntax ...
  IsCorrect();
}

bool bt_openwebnet::IsEqual(bt_openwebnet msg_to_compare)
{
	// checks for correct syntax ...
  IsCorrect();

  //control that is the same type
  if(msg_to_compare.frame_type != frame_type)
    return false;

  //control that are both two frames extended or not
  if(msg_to_compare.extended != extended)
    return false;

  if(!extended)
  {
    if ((msg_to_compare.Extract_who().compare(who) == 0) &&
        (msg_to_compare.Extract_what().compare(what) == 0) &&
        (msg_to_compare.Extract_where().compare(where) == 0) &&
        (msg_to_compare.Extract_when().compare(when) == 0) &&
        (msg_to_compare.Extract_dimension().compare(dimension) == 0))
      return true;
    else
      return false;
  }
  else
  {
    if ((msg_to_compare.Extract_who().compare(who) == 0) &&
        (msg_to_compare.Extract_what().compare(what) == 0) &&
        (msg_to_compare.Extract_where().compare(where) == 0) &&
        (msg_to_compare.Extract_level().compare(level) == 0) &&
        (msg_to_compare.Extract_interface().compare(sInterface) == 0) &&
        (msg_to_compare.Extract_when().compare(when) == 0) &&
        (msg_to_compare.Extract_dimension().compare(dimension) == 0))
      return true;
    else
      return false;
  }
}

  // extract who, addresses, what, where, level, interface, when, dimension and values of frame_open
string bt_openwebnet::Extract_who(){return who;}
string bt_openwebnet::Extract_address(unsigned int i) { if (i >= 0 && i < address.size()) return address.at(i); return ""; }
string bt_openwebnet::Extract_what(){return what;}
string bt_openwebnet::Extract_where(){return where;}
string bt_openwebnet::Extract_level(){return level;}
string bt_openwebnet::Extract_interface(){return sInterface;}
string bt_openwebnet::Extract_when(){return when;}
string bt_openwebnet::Extract_dimension(){return dimension;}
string bt_openwebnet::Extract_value(unsigned int i){ if (i >= 0 && i < value.size()) return value.at(i); return "";}

string bt_openwebnet::Extract_OpenOK(){return OPENWEBNET_MSG_OPEN_OK;};
string bt_openwebnet::Extract_OpenKO(){return OPENWEBNET_MSG_OPEN_KO;};


// frame type?

bool bt_openwebnet::IsErrorFrame(){return (frame_type == ERROR_FRAME);}
bool bt_openwebnet::IsNullFrame(){return (frame_type == NULL_FRAME);}
bool bt_openwebnet::IsNormalFrame(){return (frame_type == NORMAL_FRAME);}
bool bt_openwebnet::IsMeasureFrame(){return (frame_type == MEASURE_FRAME);}
bool bt_openwebnet::IsStateFrame(){return (frame_type == STATE_FRAME);}
bool bt_openwebnet::IsWriteFrame(){return (frame_type == WRITE_FRAME);}
bool bt_openwebnet::IsPwdFrame(){return (frame_type == PWD_FRAME);}
bool bt_openwebnet::IsOKFrame(){return (frame_type == OK_FRAME);}
bool bt_openwebnet::IsKOFrame(){return (frame_type == KO_FRAME);}

bool bt_openwebnet::ParseData(char* data, int length, vector<bt_openwebnet>& messages)
{
	string buffer = string(data, length);
	size_t begin = 0;
	size_t end = string::npos;
	do {
		end = buffer.find(OPENWEBNET_END_FRAME, begin);
		if (end != string::npos) {
			bt_openwebnet message(buffer.substr(begin, end - begin + 2));
			messages.push_back(message);
			begin = end + 2;
		}
	} while (end != string::npos);

	return true;
}

string bt_openwebnet::getWhoDescription(string& who)
{
	if (who == "0") {
		return "Scenario";
	}
	else if (who == "1") {
		return "Lighting";
	}
	else if (who == "2") {
		return "Automation";
	}
	else if (who == "3") {
		return "Load control";
	}
	else if (who == "4") {
		return "Temperature control";
	}
	else if (who == "5") {
		return "Burglar alarm";
	}
	else if (who == "6") {
		return "Door entry system";//the Who 6 addresses directly to cameras instead the Who 7 was written for frames sent to the web server
	}
	else if (who == "7") {
		return "Video door entry system";
	}
	else if (who == "9") {
		return "Auxiliary";//not documented
	}
	else if (who == "13") {
		return "Gateway interfaces management";
	}
	else if (who == "14") {
		return "Light shutter actuator lock";
	}
	else if (who == "15") {
		return "Scenario Scheduler Switch";
	}
	else if (who == "16") {
		return "Audio";
	}
	else if (who == "17") {
		return "Scenario programming";
	}
	else if (who == "18") {
		return "Energy management";
	}
	else if (who == "22") {
		return "Sound diffusion";
	}
	else if (who == "24") {
		return "Lighting management";
	}
	else if (who == "25") {
		return "Dry contact/IR Detection";
	}
	else if (who == "1000") {
		return "Diagnostic";//not documented
	}
	else if (who == "1001") {
		return "Automation diagnostic";//not documented
	}
	else if (who == "1004") {
		return "Thermoregulation diagnostic";// documented with who=4
	}
	else if (who == "1013") {
		return "Device diagnostic";//not documented
	}
	else if (who == "1018") {
		return "Energy device diagnostic";//not documented
	}

	return "Unknown who : " + who;
}


string bt_openwebnet::getWhatDescription(string& who, string& what, vector<string>& whatParameters)
{
	if (who == "0") {
		// "Scenario"
		int scenNumber = atoi(what.c_str());
		if (scenNumber >= 1 && scenNumber <= 32) {
			stringstream sstr;
			sstr << "Scenario ";
			sstr << scenNumber;
			return sstr.str();
		}
		else if (what == "40" && !whatParameters.empty()) {
			return "Start recording scenario " + whatParameters[0];
		}
		else if (what == "41" && !whatParameters.empty()) {
			return "End recording scenario " + whatParameters[0];
		}
		else if (what == "42") {
			if (!whatParameters.empty()) {
				return "Erase scenario " + whatParameters[0];
			}
			else {
				return "Erase all scenarios";
			}
		}
		else if (what == "43") {
			return "Lock scenario central unit";
		}
		else if (what == "44") {
			return "Unlock scenario central unit";
		}
		else if (what == "45") {
			return "Unavailable scenario central unit";
		}
		else if (what == "46") {
			return "Memory full of scenarios central unit";
		}
	}
	else if (who == "1") {
		// "Lighting"
		if (what == "0") {
			if (whatParameters.empty()) {
				return "Turn off";
			}
			else {
				return "Turn off at " + whatParameters[0] + " speed for step";
			}
		}
		else if (what == "1") {
			if (whatParameters.empty()) {
				return "Turn on";
			}
			else {
				return "Turn on at " + whatParameters[0] + " speed for step";
			}
		}
		else if (what == "2") {
			return "20%";
		}
		else if (what == "3") {
			return "30%";
		}
		else if (what == "4") {
			return "40%";
		}
		else if (what == "5") {
			return "50%";
		}
		else if (what == "6") {
			return "60%";
		}
		else if (what == "7") {
			return "70%";
		}
		else if (what == "8") {
			return "80%";
		}
		else if (what == "9") {
			return "90%";
		}
		else if (what == "10") {
			return "100%";
		}
		else if (what == "11") {
			return "ON timed 1 Min";
		}
		else if (what == "12") {
			return "ON timed 2 Min";
		}
		else if (what == "13") {
			return "ON timed 3 Min";
		}
		else if (what == "14") {
			return "ON timed 4 Min";
		}
		else if (what == "15") {
			return "ON timed 5 Min";
		}
		else if (what == "16") {
			return "ON timed 15 Min";
		}
		else if (what == "17") {
			return "ON timed 30 Min";
		}
		else if (what == "18") {
			return "ON timed 0.5 Sec";
		}
		else if (what == "20") {
			return "Blinking on 0.5 sec";
		}
		else if (what == "21") {
			return "Blinking on 1 sec";
		}
		else if (what == "22") {
			return "Blinking on 1.5 sec";
		}
		else if (what == "23") {
			return "Blinking on 2 sec";
		}
		else if (what == "24") {
			return "Blinking on 2.5 sec";
		}
		else if (what == "25") {
			return "Blinking on 3 sec";
		}
		else if (what == "26") {
			return "Blinking on 3.5 sec";
		}
		else if (what == "27") {
			return "Blinking on 4 sec";
		}
		else if (what == "28") {
			return "Blinking on 4.5 sec";
		}
		else if (what == "29") {
			return "Blinking on 5 sec";
		}
		else if (what == "30") {
			if (whatParameters.size()<2) {
				return "Up one level";
			}
			else {
				return "Up of " + whatParameters[0] + " levels at " + whatParameters[1] + " speed for steep";
			}
		}
		else if (what == "31") {
			if (whatParameters.size()<2) {
				return "Down one level";
			}
			else {
				return "Down of " + whatParameters[0] + " levels at " + whatParameters[1] + " speed for steep";
			}
		}
		else if (what == "1000") {
			return "Command translation : " + vectorToString(whatParameters);
		}
	}
	else if (who == "2") {
		// "Automation"
		if (what == "0") {
			return "Stop";
		}
		else if (what == "1") {
			return "Up";
		}
		else if (what == "2") {
			return "Down";
		}
		else if (what == "10") {
			return "Advanced stop";
		}
		else if (what == "11") {
			return "Advanced up";
		}
		else if (what == "12") {
			return "Advanced down";
		}
	}
	else if (who == "3") {
		// "Load control"
		if (what == "0") {
			return "disabled";
		}
		else if (what == "1") {
			return "enabled";
		}
		else if (what == "2") {
			return "forced";
		}
		else if (what == "3") {
			return "remove forced";
		}
	}
	else if (who == "4") {
		// "Temperature control";
		//TODO : verify documentation
		if (what == "0") {
			return "conditioning Mode";
		}
		else if (what == "1") {
			return "heating Mode";
		}
		else if (what == "102") {
			return "anti Freeze";
		}
		else if (what == "202") {
			return "thermal Protection";
		}
		else if (what == "302") {
			return "protection(generic)";
		}
		else if (what == "103") {
			return "OFF – Heating Mode";
		}
		else if (what == "203") {
			return "OFF – Conditioning Mode";
		}
		else if (what == "303") {
			return "OFF(Generic)";
		}
		else if (what == "110") {
			return "manual - adjustment Mode – Heating";
		}
		else if (what == "210") {
			return "manual - adjustment Mode – Conditioning";
		}
		else if (what == "310") {
			return "manual - adjustment Mode(Generic)";
		}
		else if (what == "111") {
			return "programming Mode – Heating";
		}
		else if (what == "211") {
			return "programming Mode - Conditioning";
		}
		else if (what == "311") {
			return "programming Mode(generic)";
		}
		else if (what == "115") {
			return "holiday daily plan – Heating Mode";
		}
		else if (what == "215") {
			return "holiday daily plan – Conditioning Mode";
		}
		else if (what == "315") {
			return "holiday daily plan";
		}
		else if (what == "13xxx") {
			return "Vacation scenario for xxx days – Heating mode(Xxx = 0…999)";
		}
		else if (what == "23xxx") {
			return "acation scenario for xxx days – Conditioning mode(xxx = 0….999)";
		}
		else if (what == "33xxx") {
			return "Vacation scenario for xxx days(xxx = 0….999)";
		}
		else if (what == "3000") {
			return "Vacation scenario disabled";
		}
		else if (what == "11xx") {
			return "Heating program x(x = 1…3)";
		}
		else if (what == "21xx") {
			return "Conditioning program x(x = 1…3)";
		}
		else if (what == "31xx") {
			return "Last activated program";
		}

		else if (what == "12xx") {
			return "Scenario xx(xx = 1…16)";
		}
		else if (what == "22xx") {
			return "Last activated scenario";
		}
		else if (what == "32xx") {
			return "Heating program x(x = 1…3)";
		}
		else if (what == "3200") {
			return "Conditioning program x(x = 1…3)";
		}
		else if (what == "31xx") {
			return "Last activated program";
		}

		//	3100			Scenario xx(xx = 1…16)
		//	12xx			Scenario xx(xx = 1...16)
		//	22xx			Last activated scenario
		//	32xx			Holiday scenario for xxx days – Heating mode(Xxx = 0…999)
		//	3200			Holiday scenario for xxx days – Conditioning mode(xxx = 0….999)
		//	20			Remote control disabled
		//	21			Remote control enabled
		//	22			At least one probe OFF
		//	23		At least one probe in Anti Freeze
		//	24			At least one probe in Manual
		//	30			Failure discovered
		//	31			Central Unit battery KO
		//	40			Release of sensor local adjustment
	}
	else if (who == "5") {
		// "Burglar alarm";
		if (what == "0") {
			return "maintenance";
		}
		else if (what == "1") {
			return "activation";
		}
		else if (what == "2") {
			return "disactivation";
		}
		else if (what == "3") {
			return "delay end";
		}
		else if (what == "4") {
			return "system battery fault";
		}
		else if (what == "5") {
			return "battery ok";
		}
		else if (what == "6") {
			return "no network";
		}
		else if (what == "7") {
			return "network present";
		}
		else if (what == "8") {
			return "engage";
		}
		else if (what == "9") {
			return "disengage";
		}
		else if (what == "10") {
			return "battery unloads";
		}
		else if (what == "11") {
			return "active zone";
		}
		else if (what == "12") {
			return "technical alarm";
		}
		else if (what == "13") {
			return "reset technical alarm";
		}
		else if (what == "14") {
			return "no reception - ack peripheral device";
		}
		else if (what == "15") {
			return "intrusion alarm";
		}
		else if (what == "16") {
			return "alarm 24h / tampering";
		}
		else if (what == "17") {
			return "anti - panic alarm";
		}
		else if (what == "18") {
			return "non - active zone";
		}
		else if (what == "26") {
			return "start programming";
		}
		else if (what == "27") {
			return "stop programming";
		}
		else if (what == "31") {
			return "silent alarm";
		}

	}
	else if (who == "7") {
		// "Video Door entry system";
		if (what == "0") {
			return " receive video";
		}
		else if (what == "	9") {
			return "free audio / video resources";
		}
		else if (what == "120") {
			return "zoom in";
		}
		else if (what == "121") {
			return "zoom out";
		}
		else if (what == "130") {
			return "increases x coordinate of the central part of the image to be zoomed";
		}
		else if (what == "131") {
			return "decreases x coordinate of the central part of the image to be zoomed";
		}
		else if (what == "140") {
			return "increases y coordinate of the central unit part of the image to be zoomed";
		}
		else if (what == "141") {
			return "decreases y coordinate of the central part of the image to be zoomed";
		}
		else if (what == "150") {
			return "increases luminosity";
		}
		else if (what == "151") {
			return "decreases luminosity";
		}
		else if (what == "160") {
			return "increases contrast";
		}
		else if (what == "	161") {
			return "decreases contrast";
		}
		else if (what == "170") {
			return "increases color";
		}
		else if (what == "	171") {
			return "decreases color";
		}
		else if (what == "	180") {
			return "increases image quality";
		}
		else if (what == "	181") {
			return "decreases image quality";
		}
		else
		{
			int iWhat = atoi(what.c_str());
			if (iWhat >= 311 && iWhat <= 344) {
				int iFirstDial = (iWhat - 300) / 10;
				int iSecondDial = iWhat - 300 - 10 * iFirstDial;
				stringstream sstr;
				sstr << "display DIAL " << iFirstDial << " - " << iSecondDial;
				return sstr.str();
			}
		}
	}
	else if (who == "13") {
		// "Gateway interfaces management";
		if (what == "22") {
			return "reset device";
		}
		else if (what == "30") {
			return "create network";
		}
		else if (what == "31") {
			return "close network";
		}
		else if (what == "32") {
			return "open network";
		}
		else if (what == "33") {
			return "join network";
		}
		else if (what == "34") {
			return "leave network";
		}
		else if (what == "60") {
			return "keep connect";
		}
		else if (what == "65") {
			return "scan";
		}
		else if (what == "66") {
			return "supervisor";
		}
	}
	else if (who == "14") {
		// "Light shutter actuator lock";
		if (what == "0") {
			return "disable";
		}
		else if (what == "1") {
			return "enable";
		}
	}
	else if (who == "15") {
		// "Scenario Scheduler Switch";
		stringstream sstr;

		if (whatParameters.size() >= 1) {
			if (whatParameters[0] == "0") {
				return "release after short pressure of";
			}
			else if (whatParameters[0] == "1") {
				return "release after an extended pressure of";
			}
			else if (whatParameters[0] == "2") {
				return "extended pressure of";
			}
		}

		int iWhat = atoi(what.c_str());
		if (iWhat >= 0 && iWhat <= 31) {
			int iFirstDial = (iWhat - 300) / 10;
			int iSecondDial = iWhat - 300 - 10 * iFirstDial;
			stringstream sstr;
			sstr << "button number" << iWhat;
		}

		return sstr.str();
	}
	else if (who == "16") {
		// "Audio" : old version, cf who 22
		if (what == "0") {
			return "ON amplifier / source \"base band\"";
		}
		else if (what == "3") {
			return "ON amplifier / source stereo channel";
		}
		else if (what == "10") {
			return "OFF amplifier / source \"base band\"";
		}
		else if (what == "13") {
			return "OFF amplifier / source stereo channel";
		}
		else if (what == "20") {
			return "source cycle(\"base band\")";
		}
		else if (what == "23") {
			return "source cycle(channel stereo)";
		}
		else if (what == "30") {
			return "sleep on “base band”";
		}
		else if (what == "33") {
			return "sleep on channel stereo";
		}
		else if (what == "40") {
			return "sleep OFF";
		}
		else if (what == "50") {
			return "follow me \"base band\"";
		}
		else if (what == "53") {
			return "follow me channel stereo";
		}
		else if (what == "100") {
			return "source Busy";
		}
		else if (what == "101") {
			return "start send RDS";
		}
		else if (what == "102") {
			return "stop send RDS";
		}
		else if (what == "5000") {
			return "find the first free frequency greater than the selected one";
		}
		else if (what == "5100") {
			return "find the first free frequency less than the selected one";
		}

		int iWhat = atoi(what.c_str());
		if (iWhat >= 1001 && iWhat <= 1015) {
			stringstream sstr;
			int iDeltaVolume = iWhat - 1000;
			sstr << "delta volume : +" << iDeltaVolume;
			return sstr.str();
		}
		else if (iWhat >= 1101 && iWhat <= 1115) {
			stringstream sstr;
			int iDeltaVolume = iWhat - 1100;
			sstr << "delta volume : -" << iDeltaVolume;
			return sstr.str();
		}
		else if (iWhat >= 2001 && iWhat <= 2015) {
			stringstream sstr;
			int iDeltaTone = iWhat - 2000;
			sstr << "delta high tones : +" << iDeltaTone;
			return sstr.str();
		}
		else if (iWhat >= 2101 && iWhat <= 2115) {
			stringstream sstr;
			int iDeltaTone = iWhat - 2100;
			sstr << "delta high tones : -" << iDeltaTone;
			return sstr.str();
		}
		else if (iWhat >= 5001 && iWhat <= 5015) {
			stringstream sstr;
			int iDeltaFreq = 5 * (iWhat - 5000);
			sstr << "delta frequency : +0.";
			sstr << std::setfill('0') << std::setw(2) << iDeltaFreq;
			sstr << "MHz";
			return sstr.str();
		}
		else if (iWhat >= 5101 && iWhat <= 5115) {
			stringstream sstr;
			int iDeltaFreq = 5 * (iWhat - 5100);
			sstr << "delta frequency : -0.";
			sstr << std::setfill('0') << std::setw(2) << iDeltaFreq;
			return sstr.str();
		}
		else if (iWhat >= 6001 && iWhat <= 6015) {
			stringstream sstr;
			int iDeltaStation = iWhat - 6000;
			sstr << "delta radio station or track: +" << iDeltaStation;
			return sstr.str();
		}
		else if (iWhat >= 6101 && iWhat <= 6115) {
			stringstream sstr;
			int iDeltaStation = iWhat - 6100;
			sstr << "delta radio station or track: +" << iDeltaStation;
			return sstr.str();
		}

	}
	else if (who == "17") {
		// "Scenario programming";
		if (what == "1") {
			return "start scene";
		}
		else if (what == "2") {
			return "stop scene";
		}
		else if (what == "3") {
			return "enable scene";
		}
		else if (what == "4") {
			return "disable scene";
		}
	}
	else if (who == "18") {
		// "Energy management"
		if (what == "26") {
			return "Activation of the automatic reset";
		}
		else if (what == "27") {
			return "Deactivation of the automatic reset";
		}
		else if (what == "57") {
			return "start sending daily totalizers on an hourly basis for 16 - bit daily graphics";
		}
		else if (what == "58") {
			return "start sending monthly on an hourly basis for 16 - bit graphics average daily";
		}
		else if (what == "59") {
			return "start sending monthly totalizers current year on a daily basis for 32 - bit monthly graphics";
		}
		else if (what == "510") {
			return "start sending monthly totalizers on a daily basis, last year compared to 32 - bit graphics touchx previous year";
		}
		else if (what == "71") {
			return "enable actuator";
		}
		else if (what == "73") {
			return "forced actuator for x time";
		}
		else if (what == "74") {
			return "end forced actuator";
		}
		else if (what == "75") {
			return "reset totalizers";
		}
	}
	else if (who == "22") {
		// "Sound Diffusion"
		if (what == "0") {
			return "Turn Off";
		}
		else if (what == "1") {
			return "Turn on";
		}
		else if (what == "2") {
			return "Source turned on";
		}
		else if (what == "3") {
			return "Increase volume";
		}
		else if (what == "4") {
			return "Decrease volume";
		}
		else if (what == "5") {
			return "Automatic tuner search towards higher frequencies";
		}
		else if (what == "6") {
			return "Manual tuner search towards lower frequencies";
		}
		else if (what == "9") {
			return "Go to a following station";
		}
		else if (what == "10") {
			return "Go to a previous station";
		}
		else if (what == "11") {
			return "Go to a following track";
		}
		else if (what == "12") {
			return "Go to a previous track";
		}
		else if (what == "22") {
			return "Sliding request";
		}
		else if (what == "31") {
			return "Ask a source to start telling RDS message";
		}
		else if (what == "32") {
			return "Ask a source to stop telling RDS message";
		}
		else if (what == "33") {
			return "Store the presently tuned frequency on a certain station";
		}
		else if (what == "34") {
			return "Turn ON Amplifier with follow me method";
		}
		else if (what == "35") {
			return "Turn ON Amplifier to a certain source";
		}
		else if (what == "36") {
			return "Increment Low Tones";
		}
		else if (what == "37") {
			return "Decrement Low Tones";
		}
		else if (what == "38") {
			return "Increment Mid Tones";
		}
		else if (what == "39") {
			return "Decrement Mid Tones";
		}
		else if (what == "40") {
			return "Increment High Tones";
		}
		else if (what == "41") {
			return "Decrement High Tones";
		}
		else if (what == "42") {
			return "Increment balance(left>right)";
		}
		else if (what == "43") {
			return "Decrement balance(left<right)";
		}
		else if (what == "55") {
			return "Next Preset";
		}
		else if (what == "56") {
			return "Previous Preset";
		}
	}
	else if (who == "24") {
		// "Lighting management";
		stringstream sstr;

		if (what == "1") {
			sstr << "profile id ";
			if (whatParameters.size() >= 1) {
				int profileID = atoi(whatParameters[0].c_str());
				sstr << profileID;
			}
		}
		else if (what == "2") {
			sstr << "slave offset ";
			if (whatParameters.size() >= 1) {
				if (whatParameters[0] == "0") {
					return "enable";
				}
				else if (whatParameters[0] == "1") {
					return "disable";
				}
			}
		}
	}
	else if (who == "25") {
		// "Dry contact";
		stringstream sstr;

		if (what == "31") {
			sstr << "contact ON or IR detection";
		}
		else if (what == "32") {
			sstr << "contact OFF or IR end of detection";
		}

		if (whatParameters.size() >= 1) {
			if (whatParameters[0] == "0") {
				sstr << " (request)";
			}
			else if (whatParameters[0] == "1") {
				sstr << " (system event)";
			}
		}

		if (!sstr.str().empty()) {
			return sstr.str();
		}
	}
	else if (who == "1004") {
		// "Thermoregulation diagnostic failure" : no "what table"
	}


	return "Unknown what : " + what + vectorToString(whatParameters);
}

string bt_openwebnet::getWhereDescription(string& who, string& where, vector<string>& whereParameters)
{
	if (!whereParameters.empty() && whereParameters[whereParameters.size() - 1] == "9") {
		stringstream desc;
		string addr;
		string unit;
		string macAddr;

		//Zigbee where
		if (whereParameters.size() == 1) {
			//unicast : ADDR is the four last bytes of the MAC address of the product converted in decimal format + the unit you want to control
			desc << "unicast ";
			addr = whereParameters[0];
		}
		else if (whereParameters.size() == 2) {
			if (whereParameters[0].empty()) {
				//multicast : ADDR is the destination address
				desc << "multicast ";
				addr = whereParameters[1];
			}
			else if (whereParameters[0] == "0") {
				//broadcast : ADDR is the UNIT you want to control. All products do an action on the selected UNIT
				desc << "broadcast ";
				unit = whereParameters[1];
			}
		}

		if (unit.empty() && addr.length() >= 6) {
			//addr is made of 6 bytes : mac addr (4) + unit (2)
			macAddr = addr.substr(0, 4);
			unit = addr.substr(4, 2);
		}

		if (!macAddr.empty()) {
			long lMacAddr = atol(macAddr.c_str());
			desc << "mac address ";
			desc << std::hex << lMacAddr;
		}

		if (!unit.empty()) {
			if (atoi(unit.c_str()) == 0) {
				desc << "all units";
			}
			else {
				desc << "unit " << unit;
			}
		}
	}

	bool translateAmbPL = false;

	if (who == "0") {
		// "Scenario";
		return "Control Panel " + where;
	}
	else if (who == "1") {
		// "Lighting";
		translateAmbPL = true;
	}
	else if (who == "2") {
		// "Automation";
		translateAmbPL = true;
	}
	else if (who == "3") {
		// "Load control";
		if (where == "0") {
			return "General";
		}
		else if (where == "10") {
			return "Control unit";
		}
		else {
			return where + vectorToString(whereParameters);
		}
	}
	else if (who == "4") {
		// "Temperature control"
		//0			General probes(all probes)
		//	1			Zone 1 master probe ...
		//	99			Zone 99 master probe
		//	001			All probes(master and slave) belonging to Zone 1 ...
		//			099			All probes(master and slave) belonging to Zone 99
		//	101			Probe 1 of Zone 1 ...
		// 801			Probe 8 of Zone 1
		//	102 Probe 1 of Zone 2...
		//899			Probe 8 of Zone 99
		//	#0			Central Unit
		//	#1			Zone 1 via Central Unit...
		//#99			Zone 99 via Central Unit
		//pour dim 22, Where= 3#<where actuators> avec actuators = Z#N = [0 - 99]#[1 - 9]
	}
	else if (who == "5") {
		// "Burglar alarm"
		//1 CONTROL PANEL
		//	#0..8 ZONE 0..8 CENTRAL
		//	#1..9 			WHO = 9  			AUX1..9
		//	01 INPUT ZONE : DEVICE 1 ...			0n INPUT ZONE : DEVICE n
		//	11 ZONE 1 : SENSOR n°1 ..			1n ZONE 1 : SENSOR n°n
		//	81 ZONE 8 : SENSOR n°1 ..	8n ZONE 8 : SENSOR n°n
		//	#12 ZONE C / AUX C
		//	#15 ZONE F / AUX F
	}
	else if (who == "7") {
		// "Video door entry system";
		//4000			Camera 00.. 4099			Camera 99
	}
	else if (who == "13") {
		// "Gateway interfaces management" : TODO

	}
	else if (who == "14") {
		// "Light shutter actuator lock"
		translateAmbPL = true;
	}
	else if (who == "15") {
		// "Scenario Scheduler Switch" : TODO
		translateAmbPL = true;
	}
	else if (who == "16") {
		// "Audio" : old version, cf who 22
		//0 General amplifiers #0 0 Environment amplifiers #1 1 Environment amplifiers #2 2 Environment amplifiers #3 3 Environment amplifiers #4 4 Environment amplifiers #5 5 Environment amplifiers #6 6 Environment amplifiers #7 7 Environment amplifiers #8 8 Environment amplifiers #9 9 Environment amplifiers 01 01 Amplifier …. …. 99 99 Amplifier 100 General source 101 1 Source 102 2 Source 103 3 Source 104 4 Source 105 5 Source 106 6 Source 107 7 Source 108 8 Source 109 9 Source

	}
	else if (who == "17") {
		// "Scenario programming";
		//General 0
		//Scene num
		//If MH200N [1 - 300]
		//If MH202 Numeric Value
	}
	else if (who == "18") {
		// "Energy management";
		//1N			N = [1 - 127]			Stop & Go
		//	5N			N = [1 - 255]			Energy Management Central Unit, Pulse Counter, Power Meter : Bticino reference : F520, F523, 3522.			Legrand reference : 03555, 03557, 03554
		//	7N#0			N = [1 - 255]			Energy Management Actuators : Bticino reference : F522, F523			Legrand reference : 03558, 03559
	}
	else if (who == "22") {
		// "Sound Diffusion"
		//Source 2#sourceID
		//	Speaker 3#area#point
		//	Speaker Area 4#area
		//	General 5#sender_address
		//	All Source 6
	}
	else if (who == "24") {
		// "Lighting management";
		//RECIPIENT SENDER : 			LM_zone_num # dev_type & sys_addr #00# LM_zone_num # dev_type # sys_addr
		//	LM_zone_num
		//	Code Description
		//	0 No zones
		//	1000 + zone number Zone selection
		//	1000 Every zones
		//	dev_type
		//	Code Description
		//	1 BMNE500 / 002645 device
		//	99991 Lighting Console
		//	9991 Virtual Configurator
		//	4 Broadcast
		//	8 Unknown
		//	sys_addr
		//	From number 1 to 9
	}
	else if (who == "25") {
		// "Dry contact";
		if (where.length() >= 2 && where[0] == '3') {
			string whereRight = where.substr(1);
			int iWhere = atoi(whereRight.c_str());
			if (iWhere > 10 && iWhere < 100) {
				int iZ = iWhere / 10;
				int iN = iWhere - 10 * iZ;
				stringstream sstr;
				sstr << "automation " << whereRight << " or " << "alarm/IR Z=" << iZ << "/N=" << iN;
				return sstr.str();
			}
			else if (iWhere >= 1 && iWhere <= 201) {
				return "automation " + whereRight;
			}
		}
	}
	else if (who == "1004") {
		// "Thermoregulation diagnostic";
		//1 Zone 1 master probe 2 Zone 2 master probe … … 99 Zone 99 master probe #0 Central unit #1 Zone 1 via central unit #2 Zone 2 via central unit … … #99 Zone 99 via central unit

	}


	if (translateAmbPL) {
		if (atoi(where.c_str()) == 0) {
			return "General";
		}
		if (whereParameters.size()>0) {
			//group from 1 to 255
			return "Group " + whereParameters[0];
		}

		string room;
		string pointOfLight;

		if (where.length() == 2) {
			//A = [1 - 9] , PL = [1 - 9]
			room = "room " + where.substr(0, 1);
			pointOfLight = where.substr(1, 1);
		}
		else if (where.length() == 4) {
			string begin = where.substr(0, 2);
			string end = where.substr(2, 2);

			if (begin == "00") {
				//A=00 ,  PL=[01-15]
				room = "all rooms";
				pointOfLight = end;
			}
			else if (begin == "10") {
				//A = 10 , PL = [01 - 15];
				room = "room " + begin;
				pointOfLight = end;
			}
			else {
				int amb = atoi(begin.c_str());
				if (amb >= 1 && amb <= 9) {
					//A = [01 - 09], PL = [10 - 15]
					room = "room " + begin;
					pointOfLight = end;
				}
			}
		}

		if (!room.empty() && !pointOfLight.empty()) {
			return room + ", point of light " + pointOfLight;
		}
	}

	return "Unknown where : " + where + vectorToString(whereParameters);
}

string bt_openwebnet::getDimensionsDescription(string& who, string& dimension, vector<string>& values)
{
	string dimDescription;

	if (who == "0") {
		// "Scenario" : no dimension
	}
	else if (who == "1") {
		// "Lighting";
		if (dimension == "1") {
			dimDescription = "dimming level and speed";
		}
	}
	else if (who == "2") {
		// "Automation";
		if (dimension == "10") {
			dimDescription = "shutter status";
		}
		else if (dimension == "11") {
			dimDescription = "goto level";
		}
	}
	else if (who == "3") {
		// "Load control";
		if (dimension == "0") {
			dimDescription = "all dimensions";
		}
		else if (dimension == "1") {
			dimDescription = "voltage";
		}
		else if (dimension == "2") {
			dimDescription = "current";
		}
		else if (dimension == "3") {
			dimDescription = "power";
		}
		else if (dimension == "4") {
			dimDescription = "energy";
		}
	}
	else if (who == "4") {
		// "Temperature control";
		//0 Measures Temperature
		//11 Fan coil Speed 0 = Auto 1 = vel 1 2 = vel2 3 = vel3 15 = OFF
		//12 Complete probe status
		//13 Local set offset : 00  knob on 0 // 01  knob on + 1 (degree) //11  knob on - 1 (degree)//02  knob on + 2 (degree)//12  knob on - 2 (degree)//03  knob on + 3 (degree)//13  knob on - 3 (degree)//4  knob on Local OFF
		//14 Set Point temperature
		//14 et 0 : values T et M : T = Zone operation temperature not ad just by local offset.The T field is composed from 4 digits: c1c2c3c4, included between “0050”(5° temperature) and “0400”(40° temperature). c1 is always equal to 0, it indicates a positive temperature.The c2c3 couple indicates the temperature values between[05° - 40°].c4 indicates the decimal Celsius degree by 0.5° step. M = operation mode1  heating mode2  conditional mode3  generic mode
		//19 Valves status 		CV, HV = Valves’ status, CV: Conditioning Valve and HV : Heating Valve
		//			CV, HV = 0  OFF
		//		CV, HV = 1  ON
		//	CV, HV = 2  Opened
		//	CV, HV = 3  Closed
		//	CV, HV = 4  Stop
		//	CV, HV = 5  OFF Fan Coil
		//	CV, HV = 6  ON speed 1
		//	CV, HV = 7  ON speed 2
		//	CV, HV = 8  ON speed 3
		//20 Actuator Status
		//	 0 = OFF
		//	 1 = ON
		//	 2 = Opened
		//	 3 = Closed
		//	 4 = Stop
		//	 5 = Off Fan Coil
		//	 6 = ON Vel 1
		//	 7 = ON Vel 2
		//	 8 = ON Vel 3
		//	 9 = ON Fan Coil
		//22 Split Control : MOD*SP*VEL*SWING
		//30 End date Holiday 
		//	parameter = Day*Month*Year
		//	Day = [01 - 31]
		//	Month = [01 - 12]
		//	Year = [2000 - 2099]
		//	Example: 12 June 2007 is holiday end date			*#4 * #0 * #30 * 12 * 06 * 2007##
		//31 Holiday deadline hour
		//	Hour*Minutes
		//	Hour = [00 - 23]
		//	Minutes = [00 - 59]
		//	Example: 8 : 59 is holiday end time			*#4 * #0 * #31 * 08*59##
	}
	else if (who == "5") {
		// "Burglar alarm" : no dimension
	}
	else if (who == "7") {
		// "Video Door entry system" : no dimension
	}
	else if (who == "13") {
		// "Gateway interfaces management";
		if (dimension == "0") {
			dimDescription = "time";
			//H [0-23]*M [0-59]*S [0-59]*TimeZone 001=GMT+1heure, 102=GMT-2heures
		}
		else if (dimension == "1") {
			dimDescription = "date";
			//W [0=sun-6=sat] *D[01-31]*M[01-12]*A 4 digits
		}
		else if (dimension == "10") {
			dimDescription = "ip address";
			//ip*ip*ip*ip
		}
		else if (dimension == "11") {
			dimDescription = "net mask";
			//mask*mask*mask*mask
		}
		else if (dimension == "12") {
			dimDescription = "mac addrress";
			//decimal mac*mac*mac*mac*mac*mac
		}
		else if (dimension == "15") {
			dimDescription = "device type";
			//2 MHServer 3 MH200 6 F452 7 F452V 11 MHServer2 13 H4684
		}
		else if (dimension == "16") {
			dimDescription = "firmware version";
			//version*release*build
		}
		else if (dimension == "17") {
			dimDescription = "hardware version";
			//version*release*build
		}
		else if (dimension == "19") {
			dimDescription = "uptime";
			//d*h*m*s
		}
		else if (dimension == "22") {
			dimDescription = "date and time";
			//h*m*s*t*w*d*m*a
		}
		else if (dimension == "23") {
			dimDescription = "kernel version";
			//version*release*build
		}
		else if (dimension == "24") {
			dimDescription = "distribution version";
			//version*release*build
		}
		else if (dimension == "26") {
			dimDescription = "who implemented";
		}
		else if (dimension == "66") {
			dimDescription = "product information";
		}
		else if (dimension == "67") {
			dimDescription = "get number of network product";
		}
		else if (dimension == "70") {
			dimDescription = "identify";
		}
		else if (dimension == "71") {
			dimDescription = "zigbee channel";
		}
	}
	else if (who == "14") {
		// "Light shutter actuator lock" : no dimension
	}
	else if (who == "15") {
		// "Scenario Scheduler Switch" : no dimension
	}
	else if (who == "16") {
		// "Audio" : old version, cf who 22
		//1 Volume 2 High Tones 3 Low Tones 4 Balance 5 State 6 Frequency 7 Radio station / track 8 RDS 9 Frequency + Radio station / track 10 Radio station
	}
	else if (who == "17") {
		// "Scenario programming" : no dimension
	}
	else if (who == "18") {
		// "Energy management"
		//113			Active Power
		//	1200			End Automatic Update size
		//	51			Energy / Unit Totalizer
		//	52			Energy / Unit pe month
		//	53			Partial totalizer for current month
		//	54			Partial totalizer for current day
		//	71			Actuators info
		//	72			Totalizers
		//	73			Differential current level
		//	250			Status Stop&Go(Général)
		//	251			Status Stop&Go(open / close)
		//	252			Status Stop&Go(failure / no failure)
		//	253			Status Stop&Go(block / not block)
		//	254			Status Stop&Go(open for CC between the N / not open for CC between the N / )
		//	255			Status Stop&Go(opened ground falt / not opened ground falt)
		//	256			Status Stop&Go(open for Vmax / Not open for Vmax)
		//	257			Status Stop&Go(Self - test disabled / close)
		//	258			Status Stop&Go(Automatic reset off / close)
		//	259			Status Stop&Go(check off / close)
		//	260			Status Stop&Go(Witing for closing / close)
		//	261			Status Stop&Go(First 24hours of opening / close)
		//	262			Status Stop&Go(Power failure downstream / close)
		//	263			Status Stop&Go(Power failure upstream / close)
		//	511			Daily totalizers on an hourly basis for 16 - bit Daily graphics
		//	512			Monthly average on an hourly basis for 16 - bit Media Daily graphics
		//	513			Monthly totalizers current year on a daily basis for 32 - bit Monthly graphics
		//	514			Monthly totalizers on a daily basislast year compared to 32 bit graphics TouchX Previous Year
	}
	else if (who == "22") {
		// "Sound Diffusion"
		//		1 Volume			2 High Tones			3 Medium Tones			4 Low Tones			5 Frequency			6 Track / station			7 Play status
		//	11 Frequency and Station			12 Device state			17 Balance			18 3D			19 Preset			20 Loudness
	}
	else if (who == "24") {
		// "Lighting management";
		//1 Switch ON
		//	2 Max Lux
		//	3 Maintained Level
		//	4 Automatic Switch ON
		//	5 Switch ON Delay
		//	6 Automatic Switch OFF
		//	7 Switch OFF Delay
		//	8 Delay Timer
		//	9 Stand - by Timer
		//	10 Stand - by value
		//	11 Off value
		//	12 Slave Offset(GAP) value
		//	17 State(Automatic / Manual / Stop)
		//	18 Centralised Lux value
	}
	else if (who == "25") {
		// "Dry contact" : no dimension
	}
	else if (who == "1004") {
		// "Thermoregulation diagnostic";
		//7 Central Unit Diagnostic R 11 Central Unit Auto diagnostic R 20 Probe diagnostic (only zones with failures) R 21 Probe diagnostic (all zones) R 22 Auto diagnostic of failures R 23 Number of zone with failures R
		//23 : Parameter1 = The number of non-answering probes Parameter2 = The number of failure probes
	}

	if (dimDescription.empty()) {
		dimDescription = "unknown dimension";
	}

	return dimDescription + " : " + vectorToString(values);
}

string bt_openwebnet::vectorToString(vector<string>& strings)
{
	stringstream frameStr;
	bool begin = true;
	for (vector<string>::iterator it = strings.begin(); it != strings.end(); it++) {
		if (!begin) {
			frameStr << ", ";
		}
		begin = false;
		frameStr << *it;
	}
	return frameStr.str();
}

string bt_openwebnet::frameToString(bt_openwebnet& frame)
{
	stringstream frameStr;

	frameStr << frame.frame_open;
	frameStr << " : ";

	if (frame.IsErrorFrame())
	{
		frameStr << "ERROR FRAME";
	}
	else if (frame.IsNullFrame())
	{
		frameStr << "NULL FRAME";
	}
	else if (frame.IsMeasureFrame())
	{
		frameStr << "MEASURE FRAME";
	}
	else if (frame.IsStateFrame())
	{
		frameStr << "STATE FRAME";
	}
	else if (frame.IsWriteFrame())
	{
		frameStr << "WRITE FRAME";
	}
	else if (frame.IsPwdFrame())
	{
		frameStr << "PASSWORD FRAME";
	}
	else if (frame.IsOKFrame())
	{
		frameStr << "ACK FRAME";
	}
	else if (frame.IsKOFrame())
	{
		frameStr << "NACK FRAME";
	}
	else if (frame.IsNormalFrame())
	{
		frameStr << "NORMAL FRAME";

		if (frame.extended) {
			//bus interconnection
			frameStr << " - EXTENDED : ";
			frameStr << " level=" << frame.Extract_level();
			frameStr << " - interface=" << frame.Extract_interface();
		}
		else if (frame.Extract_level() == "3") {
			frameStr << " - Private Raiser BUS";
		}

		frameStr << " - who=" << getWhoDescription(frame.Extract_who());
		frameStr << " - what=" << getWhatDescription(frame.Extract_who(), frame.Extract_what(), frame.Extract_whatParameters());
		frameStr << " - where=" << getWhereDescription(frame.Extract_who(), frame.Extract_where(), frame.Extract_whereParameters());

		if (!frame.Extract_dimension().empty()) {
			frameStr << " - dimensions : " << getDimensionsDescription(frame.Extract_who(), frame.Extract_dimension(), frame.Extract_values());
		}


		if (!frame.Extract_addresses().empty()) {
			frameStr << " - address=";
			frameStr << vectorToString(frame.Extract_addresses());
		}
	}

	return frameStr.str();
}
