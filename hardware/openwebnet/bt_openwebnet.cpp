//bt_openwebnet.cpp
//class bt_openwebnet is a modification of GNU bticino C++ openwebnet client
//from openwebnet class
//see www.bticino.it; www.myhome-bticino.it
#include <sstream>
#include <iostream>
#include <string>
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

bt_openwebnet::bt_openwebnet(int who, int what, int where)
{
	stringstream whoStr;
	stringstream whereStr;
	stringstream whatStr;

	whoStr << who;
	whereStr << what;
	whatStr << where;

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
string bt_openwebnet::Extract_value(unsigned int i){ if (i >= 0 && i < value.size()) return value.at(i); return "";
}

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
