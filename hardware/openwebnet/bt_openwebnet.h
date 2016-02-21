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


  // assegna chi cosa dove e quando per le frame "normali"
  void Assegna_chi_cosa_dove_quando();
  // assegna chi dove e grandezza chiesta per le frame di richiesta grandezze
  void Assegna_chi_dove_grandezza();
  // assegna chi e dove per le frame di richiesta stato
  void Assegna_chi_dove();
  // assegna chi dove grandezza e i valori per le frame di scrittura grandezze
  void Assegna_chi_dove_grandezza_valori();
  // assegna chi per le frame di risultato elaborazione password
  void Assegna_chi();
  // assegna livello, intrfaccia per le frame estese
  void Assegna_livello_interfaccia();
  // assegna indirizzo
  void Assegna_indirizzo();
  // verifica sintattica ...e restituisce il tipo di frame
  void IsCorrect();

  string EliminoCaratteriControllo(string in_frame);
  string FirstToken(string text, string delimiter);

  // contenuti  della frame normale
  string chi;
  vector<string> indirizzo;
  string cosa;
  string dove;
  string livello;
  string interfaccia;
  string quando;
  string grandezza;
  vector<string> valori;

  // lunghezza frame
  int length_frame_open;

public:

  // frame
  string frame_open;

  // tipo di frame open
  int tipo_frame;

  //indica le frame estese
  bool estesa;

  // costruttori
  bt_openwebnet();
  bt_openwebnet(string message);
  bt_openwebnet(int who, int what, int where);
  bt_openwebnet(string who, string what, string where, string when);

  void CreateNullMsgOpen();
  //open normale
  void CreateMsgOpen(string who, string what, string where, string when);
  void CreateMsgOpen(string who, string what,	string where, string lev, string strInterface, string when);
  //richiesta stato
  void CreateStateMsgOpen(string who, string where);
  void CreateStateMsgOpen(string who, string where,  string lev, string strInterface);
  //richiesta grandezza
  void CreateDimensionMsgOpen(string who, string where,	string dimension);
  void CreateDimensionMsgOpen(string who, string where,	string lev, string strInterface, string dimension);
  //scrittura grandezza
  void CreateWrDimensionMsgOpen(string who, string where, string dimension, vector<string> value);
  void CreateWrDimensionMsgOpen(string who, string where,  string lev, string strInterface,  string dimension, vector<string> value);
  //open generale
  void CreateMsgOpen(string message);

  // confronta due messaggi open
  bool IsEqual(bt_openwebnet msg_to_compare);

  // tipo di frame ?
  bool IsErrorFrame();
  bool IsNullFrame();
  bool IsNormalFrame();
  bool IsMeasureFrame();
  bool IsStateFrame();
  bool IsWriteFrame();
  bool IsPwdFrame();
  bool IsOKFrame();
  bool IsKOFrame();

  // estrai chi, gli indirizzi, cosa, dove, livello, interfaccia, quando, 
  // grandezza e i valori da frame_open
  string Extract_chi();
  string Extract_indirizzo(int i);
  string Extract_cosa();
  string Extract_dove();
  string Extract_livello();
  string Extract_interfaccia();
  string Extract_quando();
  string Extract_grandezza();
  string Extract_valori(int i);

  string Extract_OpenOK();
  string Extract_OpenKO();

  // distruttore
  ~bt_openwebnet();
};
