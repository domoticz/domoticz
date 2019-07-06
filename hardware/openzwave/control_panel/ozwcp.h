#ifdef WITH_OPENZWAVE
//-----------------------------------------------------------------------------
//
//	ozwcp.h
//
//	OpenZWave Control Panel
//
//	Copyright (c) 2010 Greg Satz <satz@iranger.com>
//	All rights reserved.
//
// SOFTWARE NOTICE AND LICENSE
// This work (including software, documents, or other related items) is being 
// provided by the copyright holders under the following license. By obtaining,
// using and/or copying this work, you (the licensee) agree that you have read,
// understood, and will comply with the following terms and conditions:
//
// Permission to use, copy, and distribute this software and its documentation,
// without modification, for any purpose and without fee or royalty is hereby 
// granted, provided that you include the full text of this NOTICE on ALL
// copies of the software and documentation or portions thereof.
//
// THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS 
// MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR 
// PURPOSE OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE 
// ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//
// COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR 
// CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR 
// DOCUMENTATION.
//
// The name and trademarks of copyright holders may NOT be used in advertising 
// or publicity pertaining to the software without specific, written prior 
// permission.  Title to copyright in this software and any associated 
// documentation will at all times remain with copyright holders.
//-----------------------------------------------------------------------------

#include <list>
#include <algorithm>
#include "Driver.h"
#include "Notification.h"

#define MAX_NODES 255

extern const char *valueGenreStr(OpenZWave::ValueID::ValueGenre);
extern OpenZWave::ValueID::ValueGenre valueGenreNum(char const *);
extern const char *valueTypeStr(OpenZWave::ValueID::ValueType);
extern OpenZWave::ValueID::ValueType valueTypeNum(char const *);
extern const char *nodeBasicStr(uint8);
extern const char *cclassStr(uint8);
extern uint8 cclassNum(char const *str);
extern const char *controllerErrorStr(OpenZWave::Driver::ControllerError err);

class MyValue {
  friend class MyNode;

 public:
 MyValue(OpenZWave::ValueID id) : id(id) {};
  ~MyValue() {};
  OpenZWave::ValueID getId() { return id; }
 private:  
	 OpenZWave::ValueID id;
};

typedef struct {
  uint8 groupid;
  uint8 max;
  std::string label;
  std::vector<uint8> grouplist;
} MyGroup;

class MyNode {
public:
  MyNode(int32 const);
  static void remove(int32 const);
  static int32 getNodeCount() { return nodecount; };
  void sortValues();
  void addValue(OpenZWave::ValueID id);
  void removeValue(OpenZWave::ValueID id);
  void saveValue(OpenZWave::ValueID id);
  int32 getValueCount();
  static MyValue *lookup(std::string id);
  MyValue *getValue(size_t n);
  time_t getTime() { return mtime; }
  void setTime(time_t t) { mtime = t; }
  static bool getAnyChanged() { return nodechanged; }
  static void setAllChanged(bool ch);
  bool getChanged() { return changed; }
  void setChanged(bool ch) { changed = ch; nodechanged = ch; }
  static void addRemoved(uint8 node) { removed.push_back(node); }
  static uint32 getRemovedCount() { return removed.size(); }
  static uint8 getRemoved();
  void addGroup(uint8 node, uint8 g, uint8 n, uint8 *v);
  MyGroup *getGroup(uint8 i);
  void updateGroup(uint8 node, uint8 grp, char *glist);
  uint8 numGroups() { return (uint8)groups.size(); }
  void updatePoll(char *ilist, char *plist);
private:
  ~MyNode();
  void newGroup(uint8 n);
  static int32 nodecount;
  int32 type;
  time_t mtime;
  bool changed;
  static bool nodechanged;
  static std::list<uint8> removed;
  std::vector<MyGroup*> groups;
  std::vector<MyValue*> values;
};

class COpenZWaveControlPanel
{
public:
	COpenZWaveControlPanel();
	~COpenZWaveControlPanel();
	void OnCPNotification(OpenZWave::Notification const* _notification);
	std::string SendPollResponse();
	std::string SendNodeConfResponse(int node_id);
	std::string SendNodeValuesResponse(int node_id);
	std::string SetNodeValue(const std::string &arg1, const std::string &arg2);
	std::string SetNodeButton(const std::string &arg1, const std::string &arg2);
	std::string DoAdminCommand(const std::string &fun, const int node_id, const int button_id);
	std::string DoNodeChange(const std::string &fun, const int node_id, const std::string &svalue);
	std::string DoSceneCommand(const std::string &fun, const std::string &arg1, const std::string &arg2, const std::string &arg3);
	std::string UpdateGroup(const std::string &fun, const int node_id, const int group_id, const std::string &gList);
	std::string SaveConfig();
	std::string GetCPTopo();
	std::string GetCPStats();
	std::string DoTestNetwork(const int node_id, const int cnt);
	std::string HealNetworkNode(const int node_id, const bool healrrs);

	bool isReady() { return ready; }
	void SetAllNodesChanged();
	bool getAdminState() { return adminstate; }
	void setAdminState(bool st) { adminstate = st; }
	std::string getAdminFunction() { return adminfun; }
	void setAdminFunction(std::string msg) { adminfun = msg; }
	std::string getAdminMessage() { return adminmsg; }
	void setAdminMessage(std::string msg) { adminmsg = msg; }
private:
	void web_get_groups(int n, TiXmlElement *ep);
	void web_get_values(int i, TiXmlElement *ep);
	TiXmlElement *newstat(char const *tag, char const *label, uint32 const value);
	TiXmlElement *newstat(char const *tag, char const *label, char const *value);
	unsigned long logbytes;
	bool adminstate;
	std::string adminmsg;
	std::string adminfun;
	bool ready;
};
#endif
