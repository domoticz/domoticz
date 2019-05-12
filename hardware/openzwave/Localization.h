//-----------------------------------------------------------------------------
//
//	Localization.h
//
//	Localization for CC and Value Classes
//
//	Copyright (c) 2018 Justin Hammond <justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef VALUEHELP_H
#define VALUEHELP_H

#include <cstdio>
#include <string>
#include <map>
#include "Defs.h"
#include "Driver.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{

class LabelLocalizationEntry : public Ref
{
public:
    LabelLocalizationEntry (uint16 _index, uint32 _pos = -1);
    ~LabelLocalizationEntry() {}
    void AddLabel(string label, string lang = "");
    string GetLabel(string lang);
    uint64 GetIdx();
    bool HasLabel(string lang);

private:
    uint16 m_index;
    uint32 m_pos;
    map<string, string> m_Label;
    string m_defaultLabel;
};


class ValueLocalizationEntry : public Ref
{
public:
    ValueLocalizationEntry ( uint8 _commandClass, uint16 _index, uint32 _pos = -1 );
    ~ValueLocalizationEntry() {

    }
    uint64 GetIdx();
    string GetHelp(string lang);
    void AddHelp(string HelpText, string lang = "");
    bool HasHelp(string lang);
    string GetLabel(string lang);
    void AddLabel(string Label, string lang = "");
    bool HasLabel(string lang);
    void AddItemLabel(string label, int32 itemIndex, string lang = "");
    string GetItemLabel(string lang, int32 itemIndex);
    bool HasItemLabel(int32 itemIndex, string lang);
    void AddItemHelp(string label, int32 itemIndex, string lang = "");
    string GetItemHelp(string lang, int32 itemIndex);
    bool HasItemHelp(int32 itemIndex, string lang);


private:
    uint8 m_commandClass;
    uint16 m_index;
    uint32 m_pos;
    map<string, string> m_HelpText;
    map<string, string> m_LabelText;
    map<string, map<int32, string> > m_ItemLabelText;
    map<string, map<int32, string> > m_ItemHelpText;
    string m_DefaultHelpText;
    string m_DefaultLabelText;
    map<int32, string> m_DefaultItemLabelText;
    map<int32, string> m_DefaultItemHelpText;
};



class Localization
{
    //-----------------------------------------------------------------------------
    // Construction
    //-----------------------------------------------------------------------------
private:
    Localization();
    ~Localization();

    static void ReadXML();
    static void ReadCCXMLLabel(uint8 ccID, const TiXmlElement *labelElement);
    static void ReadXMLValue(uint8 ccID, const TiXmlElement *valueElement);
    static void ReadXMLVIDItemLabel(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *labelElement);
    static void ReadGlobalXMLLabel(const TiXmlElement *labelElement);
    static uint64 GetValueKey (uint8 _commandClass, uint16 _index, uint32 _pos = -1);
public:
    static Localization* Get();
    void SetupCommandClass(CommandClass *cc);
    string GetSelectedLang() { return Localization::m_selectedLang;};
    bool SetValueHelp(uint8 ccID, uint16 indexID, uint32 pos, string help, string lang="");
    string const GetValueHelp(uint8 ccID, uint16 indexId, uint32 pos);
    bool SetValueLabel(uint8 ccID, uint16 indexID, uint32 pos, string label, string lang="");
    string const GetValueLabel(uint8 ccID, uint16 indexId, int32 pos) const;
    string const GetValueItemLabel(uint8 ccID, uint16 indexId, int32 pos, int32 itemIndex) const;
    bool SetValueItemLabel(uint8 ccID, uint16 indexId, int32 pos, int32 itemIndex, string label, string lang="");
    string const GetValueItemHelp(uint8 ccID, uint16 indexId, int32 pos, int32 itemIndex) const;
    bool SetValueItemHelp(uint8 ccID, uint16 indexId, int32 pos, int32 itemIndex, string label, string lang="");
    string const GetGlobalLabel(string text);
    bool SetGlobalLabel(string index, string text, string lang);
    static void ReadXMLVIDLabel(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *labelElement);
    static void ReadXMLVIDHelp(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *helpElement);
    bool WriteXMLVIDHelp(uint8 ccID, uint16 indexId, uint32 pos, TiXmlElement *valueElement);
    //-----------------------------------------------------------------------------
    // Instance Functions
    //-----------------------------------------------------------------------------
private:
    static Localization* m_instance;
    static map<uint64,ValueLocalizationEntry*> m_valueLocalizationMap;
    static map<uint8,LabelLocalizationEntry*> m_commandClassLocalizationMap;
    static map<string, LabelLocalizationEntry*> m_globalLabelLocalizationMap;
    static string m_selectedLang;
    static uint32 m_revision;


};

};
#endif // VALUEHELP_H
