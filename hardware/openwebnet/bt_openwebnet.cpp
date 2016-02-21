//bt_openwebnet.cpp
//class bt_openwebnet is a modification of GNU bticino C++ openwebnet client
//from openwebnet class
//see www.bticino.it; www.myhome-bticino.it

#include <sstream>
#include <iostream>
#include <string>
#include "bt_openwebnet.h"

// metodi privati ......

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


// esegue il controllo sintattico
void bt_openwebnet::IsCorrect()
{
  int j  = 0;
  string sup;
  string campo;

  // se frame ACK -->
  if (frame_open.compare(OPENWEBNET_MSG_OPEN_OK) == 0)
  {
    tipo_frame = OK_FRAME;
    return;
  }

  // se frame NACK -->
  if (frame_open.compare(OPENWEBNET_MSG_OPEN_KO) == 0)
  {
    tipo_frame = KO_FRAME;
    return;
  }


  //se il primo carattere non è *
  //oppure la frame è troppo lunga
  //oppure se non termina con due '#'
  if ((frame_open[0] != '*') ||
      (length_frame_open >	MAX_LENGTH_OPEN) ||
      (frame_open[length_frame_open-1] != '#') ||
      (frame_open[length_frame_open-2] != '#'))
  {
    tipo_frame = ERROR_FRAME;
    return;
  }

  //Controllo se sono stati digitati dei caratteri
  for (j=0;j<length_frame_open;j++)
  {
    if(!isdigit(frame_open[j]))
    {
      if((frame_open[j] != '*') && (frame_open[j] != '#'))
      {
        tipo_frame = ERROR_FRAME;
        return;
      }
    }
  }

  // frame normale ...
  //*chi#indirizzo*cosa*dove#livello#indirizzo*qunado##
  if (frame_open[1] != '#')
  {
    tipo_frame = NORMAL_FRAME;
    //estraggo i vari campi della frame open nella prima modalità (chi+indirizzo e dove+livello+interfaccia)
    Assegna_chi_cosa_dove_quando();
    //estraggo gli eventuali valori di indirizzo
    Assegna_indirizzo();
    //estraggo gli eventuali valori di livello ed interfaccia
    Assegna_livello_interfaccia();
    return;
  }

  // frame password ...
  //*#pwd##
  if(frame_open.find('*', 2) == string::npos)
  {
    tipo_frame = PWD_FRAME;
    // estraggo il chi
    Assegna_chi();
    return;
  }

  //per le altre tipologie di frame
  sup = frame_open.substr(2);
  campo = FirstToken(sup, "*");
  sup = frame_open.substr(2 + campo.length() + 1);
  if (sup.at(0) != '*')
  {
	  campo = campo + FirstToken(sup, "*");
	  sup = frame_open.substr(2 + campo.length() + 1);
  }

  //frame richiesta stato ...
  //*#chi*dove##
  if(sup[0] != '*')
  {
    tipo_frame = STATE_FRAME;
    //estraggo i vari campi della frame open nella prima modalità (chi+indirizzo e dove+livello+interfaccia)
    Assegna_chi_dove();
    //estraggo gli eventuali valori di indirizzo
    Assegna_indirizzo();
    //estraggo gli eventuali valori di livello ed interfaccia
    Assegna_livello_interfaccia();
    return;
  }
  else
  {
    //frame di richiesta misura ...
    //*#chi*dove*grandezza## o *#chi*dove*grandezza*valore_N°##
    if(sup[1] != '#')
    {
      tipo_frame = MEASURE_FRAME;
      //estraggo i vari campi della frame open nella prima modalità (chi+indirizzo e dove+livello+interfaccia)
      Assegna_chi_dove_grandezza();
      //estraggo gli eventuali valori di indirizzo
      Assegna_indirizzo();
      //estraggo gli eventuali valori di livello ed interfaccia
      Assegna_livello_interfaccia();
      return;
    }
    //frame di scrittura grandezza ...
    //*#chi*dove*#grandezza*valore_N°##
    else
    {
      tipo_frame = WRITE_FRAME;
      //estraggo i vari campi della frame open nella prima modalità (chi+indirizzo e dove+livello+interfaccia)
      Assegna_chi_dove_grandezza_valori();
      //estraggo gli eventuali valori di indirizzo
      Assegna_indirizzo();
      //estraggo gli eventuali valori di livello ed interfaccia
      Assegna_livello_interfaccia();
      return;
    }
  }

  // frame errata !!!
  tipo_frame = ERROR_FRAME;
  return;
}

// assegna chi dove e la grandezza richiesta per le frame di richiesta grandezze
void bt_openwebnet::Assegna_chi_dove_grandezza()
{
  string sup;
  size_t len = 0;

  // CHI
  sup = frame_open.substr(2);
  if (sup.at(0) != '*') {
	  chi = FirstToken(sup, "*");
  }
  // DOVE
  sup = frame_open.substr(2 + chi.length() + 1);
  if(sup.find("*") == string::npos)
    dove = sup.substr(0, sup.length()-2);
  else
  {
    if(sup.at(0) != '*')
      dove = FirstToken(sup, "*");
    // GRANDEZZA
    sup = frame_open.substr(2+chi.length()+1+dove.length()+1);
    if(sup.find("*") == string::npos)
		grandezza = sup.substr(0, sup.length() - 2);
    else
    {
		if (sup.at(0) != '*') {
			grandezza = FirstToken(sup, "*");
		}
      // VALORI **##
      sup = frame_open.substr(2+ chi.length() + 1 + dove.length() + 1 +grandezza.length()+1);
      len = 2 + chi.length() + 1 + dove.length() + 1 + grandezza.length() + 1;
      while ((sup.find("*") != string::npos) && (sup.at(0) != '*'))
      {
        string valoriValue = FirstToken(sup, "*");
		valori.push_back(valoriValue);
        len = len+ valoriValue.length()+1;
        sup = frame_open.substr(len);
      }
      if ((sup[0] != '*') && (sup[0] != '#'))
      {
		  string valoriValue = sup.substr(0, sup.length()-2);
		  valori.push_back(valoriValue);
      }
    }
  }

  return;
}

// assegna chi e dove per le frame di richiesta stato
void bt_openwebnet::Assegna_chi_dove()
{
  string sup;

  // CHI
  sup = frame_open.substr(2);
  if (sup.at(0) != '*') {
	  chi = FirstToken(sup, "*");
  }
  // DOVE
  sup = frame_open.substr(2 + chi.length() + 1);
  if (sup.find("*") == string::npos) {
	  dove = sup.substr(0, sup.length() - 2);
  }
  else {
	  if (sup.at(0) != '*') {
		  dove = FirstToken(sup, "*");
	  }
  }

  return;
}

// assegna chi cosa dove e quando per le frame "normali"
void bt_openwebnet::Assegna_chi_cosa_dove_quando()
{
	string sup;

	// CHI
	sup = frame_open.substr(1);
	if (sup.at(0) != '*') {
		chi = FirstToken(sup, "*");
	}
	// COSA
	sup = frame_open.substr(1 + chi.length() + 1);
	if (sup.find("*") == string::npos) {
		cosa = sup.substr(0, sup.length() - 2);
	}
	else {
		if (sup[0] != '*')
			cosa = FirstToken(sup, "*");
		// DOVE
		sup = frame_open.substr(1 + chi.length() + 1 + cosa.length() + 1);
		if (sup.find("*") == string::npos) {
			dove = sup.substr(0, sup.length() - 2);
		}
		else
		{
			if (sup[0] != '*')
				dove = FirstToken(sup, "*");
			// QUANDO
			sup = frame_open.substr(1 + chi.length() + 1 + cosa.length() + 1 + dove.length() + 1);
			if (sup.find("*") == string::npos) {
				quando = sup.substr(0, sup.length() - 2);
			}
			else
				if (sup[0] != '*')
					quando = FirstToken(sup, "*");
		}
	}

	return;
}

// assegna chi dove grandezza e i valori per le frame di scrittura grandezze
void bt_openwebnet::Assegna_chi_dove_grandezza_valori()
{
	string sup;
  int len = 0;
  int i = -1;

  // CHI
  sup = frame_open.substr(2);
  if(sup[0] != '*')
    chi = FirstToken(sup, "*");
  // DOVE
  sup =  frame_open.substr(2+chi.length()+1);
  if(sup.find('*') == string::npos)
    dove = sup.substr(0,sup.length()-2);
  else
  {
    if(sup.at(0) != '*')
      dove =FirstToken(sup, "*");
    // GRANDEZZA
    sup = frame_open.substr(2+chi.length()+1+dove.length()+2);
    if(sup.find('*') == string::npos)
      tipo_frame = ERROR_FRAME;
    else
    {
      if(sup.at(0) != '*')
        grandezza = FirstToken(sup, "*");
      // VALORI
	  len = 2 + chi.length() + 1 + dove.length() + 2 + grandezza.length() + 1;
	  sup = frame_open.substr(len);
      while (sup.find('*') != string::npos && (sup.at(0) != '*'))
      {
		  string newValue = FirstToken(sup, "*");
		  valori.push_back(newValue);
			len = len+ newValue.length() +1;
			sup = frame_open.substr(len);
        while(sup.at(0) == '*')
        {
          len++;
		  sup = frame_open.substr(len);
		  valori.push_back("");
        }
        if (sup.at(0) != '*')
			valori.push_back("");
      }
      if ((sup.at(0) != '*') && (sup.at(0) != '#'))
      {
		  valori.push_back(sup.substr(0, sup.length()-2));
      }
    }
  }

  return;
}

// assegna chi per le frame di risultato elaborazione password
void bt_openwebnet::Assegna_chi()
{
	string sup;

	// CHI
	sup = frame_open.substr(2);
	if (sup.at(0) != '#') {
		chi = FirstToken(sup, "#");
	}
	else {
		tipo_frame = ERROR_FRAME;
	}
	
	sup = frame_open.substr(2+chi.length());
	if (sup.at(1) != '#') {
		tipo_frame = ERROR_FRAME;
	}

  return;
}

// assegna livello, intrfaccia per le frame estese
void bt_openwebnet::Assegna_livello_interfaccia()
{
	string sup;
  string orig;

  // DOVE
  if (dove.at(0) == '#')
	  sup = dove.substr(1);
  else
	  sup = dove;
  orig = dove;
  if(sup.find('#') != string::npos)
  {
    estesa = true;
    if(orig.at(0) == '#')
      dove, "#" +  FirstToken(sup, "#");
    else
      dove = FirstToken(sup, "#");
    // LIVELLO + INTERFACCIA
    sup= orig.substr(dove.length()+1);
    if(sup.find('#') != NULL)
    {
      livello = FirstToken(sup, "#");
      interfaccia = orig.substr(dove.length()+1+livello.length()+1);
      if(interfaccia.length() == 0)
        tipo_frame = ERROR_FRAME;
    }
    else
      //Modifica per Bt_vctMM per accensione telecamere esetrne (*6*0*4000#2*##)
      //tipo_frame = ERROR_FRAME;
      livello = sup;
  }
  
  return;
}

// assegna indirizzo
void bt_openwebnet::Assegna_indirizzo()
{
	string sup;
	string orig;
	indirizzo.reserve(4);

	// CHI
	sup = chi;
	orig = chi;

	if (sup.find("#") != string::npos)
	{
		chi = FirstToken(sup, "#");
		// INDIRIZZO
		indirizzo.clear();
		sup = orig.substr(chi.length() + 1);
		if (sup.find('#') == string::npos)
		{
			// solo indirizzo seriale
			if (sup.length() != 0)
				indirizzo.push_back(sup);
			else
				tipo_frame = ERROR_FRAME;
		}
		else
		{
		  // indirizzo IP
		  indirizzo[0] = FirstToken(sup, "#");
		  sup = orig.substr(chi.length() + 1 + indirizzo[0].length() + 1);
		  if(sup.find('#') != string::npos)
		  {
			// indirizzo IP
			indirizzo[1] = FirstToken(sup, "#");
			sup = orig.substr(chi.length() + 1 + indirizzo[0].length() + 1 + indirizzo[1].length() + 1);
			if(sup.find('#') != string::npos)
			{
			  // indirizzo IP
			  indirizzo[2] = FirstToken(sup, "#");
			  sup = orig.substr(chi.length() + 1 + indirizzo[0].length() + 1 + indirizzo[1].length() + 1 + indirizzo[2].length() + 1);
			  if(sup.find('#') == string::npos)
			  {
				// indirizzo IP
				if(sup.length() != 0)
				  indirizzo[3] = sup;
				else
				  tipo_frame = ERROR_FRAME;
			  }
			  else
				tipo_frame = ERROR_FRAME;
			}
			else
			  tipo_frame = ERROR_FRAME;
		  }
		  else
			tipo_frame = ERROR_FRAME;
		}
  }

  return;
}

// metodi publici ......

// costruttori
bt_openwebnet::bt_openwebnet()
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();
}


bt_openwebnet::bt_openwebnet(string message)
{
  //richiamo la procedura CreateMsgOpen(char message[MAX_LENGTH_OPEN], int length_message)
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

// crea il messaggio OPEN *chi*cosa*dove*quando##
bt_openwebnet::bt_openwebnet(string who, string what, string where, string when)
{
  //richiamo la procedura CreateMsgOpen()
  CreateMsgOpen(who, what, where, when);
}


// distruttore
bt_openwebnet::~bt_openwebnet()
{
}

// crea il messaggio OPEN *chi*cosa*dove*quando##
void bt_openwebnet::CreateMsgOpen(string who, string what,	string where,	string when)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame;

  // creo il messaggio open
  frame << "*";
  frame << who;  frame << "*";
  frame << what;  frame << "*";
  frame << where;
  //per togliere l'asterisco finale
  if (when.length() != 0) {
	  frame << "*";
	  frame << when;
  }
  frame << "##";

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

// crea il messaggio OPEN *chi*cosa*dove#livello#interfaccia*quando##
void bt_openwebnet::CreateMsgOpen(string who, string what, string where, string lev, string interface, string when)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame; 
  
  // creo il messaggio open
  frame << "*";
  frame << who;  frame << "*";
  frame << what;  frame << "*";
  frame << where; frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interface;
  //per togliere l'asterisco finale
  if (!when.empty()) {
	  frame << "*";
	  frame << when;
  }
  frame << "##";

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

// crea il messaggio OPEN ****##
void bt_openwebnet::CreateNullMsgOpen()
{
  //contatore per azzarare i valori
  int i = 0;

  // azzera tutto
  frame_open = "";
  tipo_frame = NULL_FRAME;

  length_frame_open = 0;

  estesa = false;

  // azzera tutto
  chi = "";
  indirizzo.clear();
  cosa = "";
  dove = "";
  livello = "";
  interfaccia = "";
  quando = "";
  grandezza = "";
  valori.clear();
}

//crea il messaggio open *#chi*dove##
void bt_openwebnet::CreateStateMsgOpen(string who, string where)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame;

  // creo il messaggio open
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "##";
  
  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

string bt_openwebnet::EliminoCaratteriControllo(string in_frame)
{
	string out_frame = in_frame;

	// elimino i caratteri di controllo ....
	while ((out_frame.at(out_frame.length() - 1) == '\n') || ((out_frame.at(out_frame.length() - 1) == '\r')))
	{
		out_frame.erase(out_frame.length() - 1, 1);
	}

	return out_frame;
}

//crea il messaggio open *#chi*dove#livello#interfaccia##
void bt_openwebnet::CreateStateMsgOpen(string who, string where, string lev, string interface)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame;

  frame << "#";
  frame << who;  frame << "*";
  frame << where;  frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interface;
  frame << "##";

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

//crea il messaggio open *#chi*dove*grandezza##
void bt_openwebnet::CreateDimensionMsgOpen(string who, string where, string dimension)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame;
  
  // creo il messaggio open
  frame << "*#";
  frame << who;  frame << "*";
  frame << where;  
  //per togliere l'asterisco finale
  if (!dimension.empty()) {
	  frame << "*";
	  frame << dimension;
  }
  frame << "##";

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

//crea il messaggio open *#chi*dove#livello#interfaccia*grandezza##
void bt_openwebnet::CreateDimensionMsgOpen(string who, string where, string lev, string interface, string dimension)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame; 
  
  // creo il messaggio open
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interface;

  //per togliere l'asterisco finale
  if (dimension.length() != 0)
	  frame << "*";
  frame << dimension; frame << "##";

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

//crea il messaggio open *#chi*dove*#grandezza*val_1*val_2*...val_n##
void bt_openwebnet::CreateWrDimensionMsgOpen(string who, string where, string dimension, vector<string> value)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame; 
  
  // creo il messaggio open
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

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

//crea il messaggio open *#chi*dove#livello#interfaccia*#grandezza*val_1*val_2*...val_n##
void bt_openwebnet::CreateWrDimensionMsgOpen(string who, string where, string lev, string interface, string dimension, vector<string> value)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  stringstream frame;

  // creo il messaggio open
  frame << "*#";
  frame << who;  frame << "*";
  frame << where; frame << "*#";
  if (lev.empty())
	  frame << "4";
  else
	  frame << lev;
  frame << "#"; frame << interface;

  //per togliere l'asterisco finale
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

  frame_open = EliminoCaratteriControllo(frame.str());
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}


void bt_openwebnet::CreateMsgOpen(string message)
{
  //richiamo la procedura CreateNullMsgOpen()
  CreateNullMsgOpen();

  // salva il tipo di frame e la sua lunghezza
  frame_open = message;
  
  frame_open = EliminoCaratteriControllo(frame_open);
  length_frame_open = frame_open.length();

  // controlla se sintassi corretta ...
  IsCorrect();
}

bool bt_openwebnet::IsEqual(bt_openwebnet msg_to_compare)
{
  // controlla se sintassi corretta ...
  IsCorrect();

  //conttrollo che sia la stessa tipologia
  if(msg_to_compare.tipo_frame != tipo_frame)
    return false;

  //conttrollo che siano entrambe due frame estese o meno
  if(msg_to_compare.estesa != estesa)
    return false;

  if(!estesa)
  {
    if ((msg_to_compare.Extract_chi().compare(chi) == 0) &&
        (msg_to_compare.Extract_cosa().compare(cosa) == 0) &&
        (msg_to_compare.Extract_dove().compare(dove) == 0) &&
        (msg_to_compare.Extract_quando().compare(quando) == 0) &&
        (msg_to_compare.Extract_grandezza().compare(grandezza) == 0))
      return true;
    else
      return false;
  }
  else
  {
    if ((msg_to_compare.Extract_chi().compare(chi) == 0) &&
        (msg_to_compare.Extract_cosa().compare(cosa) == 0) &&
        (msg_to_compare.Extract_dove().compare(dove) == 0) &&
        (msg_to_compare.Extract_livello().compare(livello) == 0) &&
        (msg_to_compare.Extract_interfaccia().compare(interfaccia) == 0) &&
        (msg_to_compare.Extract_quando().compare(quando) == 0) &&
        (msg_to_compare.Extract_grandezza().compare(grandezza) == 0))
      return true;
    else
      return false;
  }
}

  // estrai chi, gli indirizzi, cosa, dove, livello, interfaccia, quando, grandezza e i valori da frame_open
string bt_openwebnet::Extract_chi(){return chi;}
string bt_openwebnet::Extract_indirizzo(int i) { if (i >= 0 && i < indirizzo.size()) return indirizzo.at(i); return ""; }
string bt_openwebnet::Extract_cosa(){return cosa;}
string bt_openwebnet::Extract_dove(){return dove;}
string bt_openwebnet::Extract_livello(){return livello;}
string bt_openwebnet::Extract_interfaccia(){return interfaccia;}
string bt_openwebnet::Extract_quando(){return quando;}
string bt_openwebnet::Extract_grandezza(){return grandezza;}
string bt_openwebnet::Extract_valori(int i){ if (i >= 0 && i < valori.size()) return valori.at(i); return "";
}

string bt_openwebnet::Extract_OpenOK(){return OPENWEBNET_MSG_OPEN_OK;};
string bt_openwebnet::Extract_OpenKO(){return OPENWEBNET_MSG_OPEN_KO;};


// tipo di frame ?

bool bt_openwebnet::IsErrorFrame(){return (tipo_frame == ERROR_FRAME);}
bool bt_openwebnet::IsNullFrame(){return (tipo_frame == NULL_FRAME);}
bool bt_openwebnet::IsNormalFrame(){return (tipo_frame == NORMAL_FRAME);}
bool bt_openwebnet::IsMeasureFrame(){return (tipo_frame == MEASURE_FRAME);}
bool bt_openwebnet::IsStateFrame(){return (tipo_frame == STATE_FRAME);}
bool bt_openwebnet::IsWriteFrame(){return (tipo_frame == WRITE_FRAME);}
bool bt_openwebnet::IsPwdFrame(){return (tipo_frame == PWD_FRAME);}
bool bt_openwebnet::IsOKFrame(){return (tipo_frame == OK_FRAME);}
bool bt_openwebnet::IsKOFrame(){return (tipo_frame == KO_FRAME);}
