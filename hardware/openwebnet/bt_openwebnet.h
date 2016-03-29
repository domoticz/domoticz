//bt_openwebnet.h
//class bt_openwebnet is a modification of GNU bticino C++ openwebnet client
//from openwebnet class
//see www.bticino.it; www.myhome-bticino.it

#pragma once

#include <string>
#include <vector>

#define OPENWEBNET_MSG_OPEN_OK "*#*1##"
#define OPENWEBNET_MSG_OPEN_KO  "*#*0##"
#define OPENWEBNET_COMMAND_SESSION "*99*0##"
#define OPENWEBNET_EVENT_SESSION "*99*1##"
#define OPENWEBNET_END_FRAME "##"
#define OPENWEBNET_COMMAND_SOCKET_DURATION 30

using namespace std;

class bt_openwebnet {

private:

  // costanti varie
  const static int MAX_LENGTH_OPEN  = 1024;
  const static int ERROR_FRAME      = 1;
  const static int NULL_FRAME       = 2;
  const static int NORMAL_FRAME     = 3;
  const static int MEASURE_FRAME    = 4;
  const static int STATE_FRAME      = 5;
  const static int OK_FRAME         = 6;
  const static int KO_FRAME         = 7;
  const static int WRITE_FRAME      = 8;
  const static int PWD_FRAME        = 9;


  // assign who what where and when for normal frame
  void Assegna_chi_cosa_dove_quando();
  // assign who where and dimension for dimension frame request
  void Assegna_chi_dove_grandezza();
  // assign who and where for request state frame
  void Assegna_chi_dove();
  // assign who where dimension and value for write dimension frame
  void Assegna_chi_dove_grandezza_valori();
  // assign who for frame result of elaborate password
  void Assegna_chi();
  // assign level, interface for extended frame
  void Assegna_livello_interfaccia();
  // assign address
  void Assegna_indirizzo();
  // check frame syntax
  void IsCorrect();

  string EliminoCaratteriControllo(string in_frame);
  string FirstToken(string text, string delimiter);

  // contents of normal frame
  string chi;
  vector<string> indirizzo;
  string cosa;
  string dove;
  string livello;
  string interfaccia;
  string quando;
  string grandezza;
  vector<string> valori;

  // frame length
  int length_frame_open;

public:

  // frame
  string frame_open;

  // type of frame open
  int frame_type;

  //indicates extended frame
  bool extended;

  // constructors
  bt_openwebnet();
  bt_openwebnet(string message);
  bt_openwebnet(int who, int what, int where);
  bt_openwebnet(string who, string what, string where, string when);

  void CreateNullMsgOpen();
  //normal open
  void CreateMsgOpen(string who, string what, string where, string when);
  void CreateMsgOpen(string who, string what,	string where, string lev, string strInterface, string when);
  //state request
  void CreateStateMsgOpen(string who, string where);
  void CreateStateMsgOpen(string who, string where,  string lev, string strInterface);
  //dimension request
  void CreateDimensionMsgOpen(string who, string where,	string dimension);
  void CreateDimensionMsgOpen(string who, string where,	string lev, string strInterface, string dimension);
  //dimension write
  void CreateWrDimensionMsgOpen(string who, string where, string dimension, vector<string> value);
  void CreateWrDimensionMsgOpen(string who, string where,  string lev, string strInterface,  string dimension, vector<string> value);
  //general message
  void CreateMsgOpen(string message);

  // confronta due messaggi open
  bool IsEqual(bt_openwebnet msg_to_compare);

  // frame type?
  bool IsErrorFrame();
  bool IsNullFrame();
  bool IsNormalFrame();
  bool IsMeasureFrame();
  bool IsStateFrame();
  bool IsWriteFrame();
  bool IsPwdFrame();
  bool IsOKFrame();
  bool IsKOFrame();

  // extract who, addresses, what, where, level, interface, when
  // dimensions and values of frame open
  string Extract_who();
  string Extract_address(unsigned int i);
  string Extract_what();
  string Extract_where();
  string Extract_level();
  string Extract_interface();
  string Extract_when();
  string Extract_dimension();
  string Extract_value(unsigned int i);

  string Extract_OpenOK();
  string Extract_OpenKO();

  // destructor
  ~bt_openwebnet();
};
