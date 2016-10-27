#pragma once

#include "DomoticzHardware.h"
#include <iostream>

// PCF8574 (8-bit I/O expaner for I2C bus)
#define PCF8574_ADDRESS_1		0x20    /* I2C address for I/O expaner on base board - control relays and optical isolated input and output*/
#define PCF8574_ADDRESS_2		0x24    /* I2C address for I/O expaner on display board - control LCD-light and keys*/
#define SEAHU_I2C_BUS_PATH	"/dev/i2c-1"	/* path to kernel i2c bus. If use another version raspberrryPI tehn v3 may by must change to "/dev/i2c-0" */


class CSeahu : public CDomoticzHardwareBase
{
	typedef struct {
		char i2c_addr ; /* i2c address for PCF8574 chip */
		char mask;      /* 8-bit mask for select pin bit */
	} SEAHU_I2C_PIN;
public:
	CSeahu(const int ID);
	~CSeahu();
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	static SEAHU_I2C_PIN seahuPins[];

	std::string device;

	void ReadSwitchs();
	
	char ReadPin(char gpioId, char *buf);
	char WritePin(char gpioId,char  value);

	int openI2C(const char *path);
	char readByteI2C(int file, char *byte, char i2c_addr);
	char writeByteI2C(int file, char byte, char i2c_addr);
	

};

// NOTE HOW TO ADD NEW HARDWARE (in czech language) 
// !! WARNING: add new hardware is realy big smut !!
//-------------------------------------------------
// muj vzor: Dummy a daikin
// new hw zapsat do /main/RFXNames.h jako HTYPE_jmeno
// a taky do /main/RFXNames.cpp - kde je preklad na delsi text pouzivany pri selectu na webu
// a nasledne pridat do
// main/mainworker.cpp - je zapotrebi pridat odkaz na HTYPE_Seahu(ID) jinak se nemuze vybec modul spustit a na zacatku #include "../hardware/Seahu.h"
// main/WebServer.cpp - je potreba pridat odkaz na pridani a update modulu HTYPE_Daikin a rovnez na zactku #include "../hardware/Seahu.h"
// a jeste do
// CMakeLists.txt - pridat odkaz na .cpp soubor (hardware/Seahu.cpp) jinak to kompilator ignoruje
// pote smozdrejmne spustiti:
// cmake -DCMAKE_BUILD_TYPE=Release
// make

// navic uplne oddelene je potreba upravit javascript pro vyber noveho HW www/app/HardwareControler.js
//    v tomto js souboru je potreba pridat oddil pro novy hw a to oddelne pro upravu (function UpdateHardware)
//    a oddelene pro pridani hw (function AddHardware) je to takova prasarna pre if a else if se testuji ne id ale vetsinou nazvy ze selektu
//    a se nastavi ktere <div> budou viditelne a nasledne jsou radky pro kontrolu validace dat zadanych do formulare
//    a konecne sformovani ajaxu, ktery to odesle. 
//    Kdyby bylo potreba pridat nejake nestandartni formularove policko tak, samotne <div> s definici formularovych
//    policek se nachazi v souboru www/views/hardware.html

// v Seahu.cpp by bylo vhodne zamenit pTypeGeneralSwitch za vlastni typ a pridat ho do hardwaretypes.h
// 	to z duvodu jednodussiho vyhledavani mych zarizeni protoze, v prubehu startu budu musetr pridat
//	svoje devices ale kdyz budou jednou pridany tak uz ne, tak je musim na zaklde neceho rozeznat
// v tabulce DeviceStatus do kolonky subType zadam cisla svitchu a podle toho je rzlisim

// vypda to ze neni potreba dopredu vytvaret nove zarizeni, ze ho vytvori aktualizaacni dotaz napr. 
// SendLuxSensor a jemu podobne v hardware/DomoticzHardware.h

// RFXtrx.h - obsahuje definici typu a struktury obsahu jakysich vnitrnich paktu, dulezite hlavne pro spracovani fce WriteToHardware
//      ktera sprostredkovava zapis zmen do hw
//	vetsina informaci o zarizeni je ulozena v DB tabulce DeviceStatus avsak pojmenovani sloupecku primo neopavida nazvum poli 
//      ve strukture vnirniho paketu, tak je to takovy docela chaos. Nize se pokusim vysvetlit mapovani
//	mezi DB tabulkou a strukturou vnitrniho mesage paketu (nutno dotat, ze kazdy typ device ma vnitrni strukturu paketu trochu jinou).
//	Nimene alspon prvni dve polozky paketu jsou vzdy stejne. Prvni udava delku celeho paketu a druha typ paketu.
//	A prave podle typu paketu se lisi dalsi obssah. Nize budu popisovat jen ty typy paketu se kterymi jsem prisel do skytku.
//	
//	packettype ve strukture msg odkazuje na cilo definovane v hardware
// 	
//
//	ted to trochu preskocim a budy nejspise uplne choticky protoze hledam postupne souvislosti
//	-- main/RTXtrx.h - defineice msg strupktury (kazda obsahuje packetlength a packettype )
//			rovnez obsahuje i definice packetttype
//

// v RFXNames.cpp je pole kde se preklada definice pTypeXX na nazev sobrazovany na webu devices jako type
//	je vsak zvlastni, ze nekdy odkazuje na hodnoty definovane v hradware/hardwaretypes.h a podruhe do
//	main/RTXtrx.h 
//	#define pTypeXxx - definice typu
//	#define sTypeXxx - definice subTypu v main/RFXNames.cpp je pole spojujici typy s subtypy
//	tyto definice maji svuj protejsek v DB tabulce DeviceStatus sloupec "Type" a "SubType"
//	problem je, ze z funkce ktera obsluhuje HW - WriteToHardware(const char *pdata, const unsigned char length)
//	zjistim typ paketu, ale ne kazdy takovy typ ma definouvou svoji strukturu paketu, respektive nedokazal jsem
//	najit vazbu mezi typem paketu a strukturou paketu. Nekdy je to logicke ne vsak pevne dane napr:
//	lze predpokladat ze typ "pTypeLighting2" bude pouzivat strukturu "LIGHTING2"
//	avsak napr pro typ "pTypeGeneralSwitch" neexistuje zadna pouzitelna struktura "GENERALSWITCH" ktere by zaroven byla ve stejne union jako LIGHTING1,LIGHTING2,..
//	vysledek pro mne je ten, ze sam musim vedet jak jadro dany typ spracuje do msg paketu a pak to spravne obslouzit
//	nez toto zjistovat je lepsi pouzivat primo typ ktery je definovany v RFXtrx.h i s predpokladanou  strukturou pouzivanou v msg paketech.
//	Jako napr. pTypeLighting1, pTypeLighting2



// Shrnuti jak na novy hardware:
// ------------------------------
//
// 1. v /main/RFXNames.h do strukury "enum _eHardwareTypes" pridat HTYPE_Seahu,
// 2. v /main/RFXNames.cpp pridat popisku k HTYPE_MyName ve strukture "const char *Hardware_Type_Desc(int hType)..."
// 3. v main/mainworker.cpp - je zapotrebi pridat odkaz na HTYPE_Seahu(ID) ve fci "MainWorker::AddHardwareFromParams" jinak se nemuze vybec modul spustit a na zacatku #include "../hardware/Seahu.h"
// 4. v main/WebServer.cpp - je potreba pridat odkaz na pridani a update modulu HTYPE_Daikin a rovnez na zactku #include "../hardware/Seahu.h"
//		ve vetsine pripdau kdy neni pri zavadeni nebo zmene noveho hw potreba nic delat, tak neni treba ze nic pridavat ani include
//		nicmene jen tak z dodrzovani stabni kulturu je dobre pro myj typ vytvorit prazne else if (pokd pri tom nevolam svje funkce tak include neni zapotrebi)
// 5. v www/app/HardwareControler.js - upravit javascript pro zobrazovani weboveho formulare noveho HW a jeho parametru
//    		v tomto js souboru je potreba pridat oddil pro novy hw a to oddelne pro upravu (function UpdateHardware)
//    		a oddelene pro pridani hw (function AddHardware) je to takova prasarna pre if a else if se testuji ne id ale vetsinou nazvy ze selektu
//    		a se nastavi ktere <div> budou viditelne a nasledne jsou radky pro kontrolu validace dat zadanych do formulare
//    		a konecne sformovani ajaxu, ktery to odesle. 
//    		Kdyby bylo potreba pridat nejake nestandartni formularove policko tak, samotne <div> s definici formularovych
//    		policek se nachazi v souboru www/views/hardware.html
// 6. vytvorit ve slozce  hardware/ vlastni Seahu.h a Seahu.cpp vlastni program (je takova konvence, ze pred vlasni nazev tridy se pise jecte C tj. misto Seahu -> CSeahu)
//		dulezite fce:
//		CSeahu::CSeahu(const int ID) - konstruktor
//		CSeahu::~CSeahu() - nevim asi taky neco jako konstruktor
//		bool CSeahu::StartHardware() - start hw (modulu) zvykem je si zde vytvorit vlastni vlakno standartne z fce: void CSeahu::Do_Work()
//			a pokud je potreba tak taky zavolat svuj vlastni init().
//		bool CSeahu::StopHardware() - stop hw (modulu) v podstate provede zastaveni sveho vlakna
//		void CSeahu::Do_Work() - nazev asi nemusi byt jednotny, ale tato fci spousti startHardvare jako samostane vlakno.
//			fce by mnela byt cyklicka reaguji na ukoncujici priznak, dale by mnela aktualizavat promnenou m_LastHeartbeat
//			a zjistovat aktualni stav cidel, ktery by mnala zapsat do spravne struktury (nejjednoduseji pomoci fce. SendLuxSensor, SendSwitchSenzor,.. z hardware/DomoticzHardware.h )
//		
// 		bool CSeahu::WriteToHardware(const char *pdata, const unsigned char length) - funkce, ktera je volana pri pozadavku na zmenu stavu cidla
//			je ji predan jen nejaky MSG paket, jehoz struktura je rozdilna pro jednotlive typy hardweru ci messaqe paktu (presnou vztah jsem dosud neodhalil)
//			ale pokusne s vypisovanim do logu se lze dopatrat tech spravnych informaci.
//
// 7. v CMakeLists.txt pridat odkaz na .cpp soubor (hardware/Seahu.cpp) jinak to kompilator ignoruje (maji to tam razene abecedne, tak to dodrzim)
// 8. cmake -DCMAKE_BUILD_TYPE=Release
// 9. make
//