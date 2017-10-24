//-----------------------------------------------------------------------------
//
//	ozwcp.cpp
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
#include "stdafx.h"
#ifdef WITH_OPENZWAVE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../Options.h"
#include "../Manager.h"
#include "../Node.h"
#include "../Group.h"
#include "../Notification.h"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <tinyxml.h>

//#include "microhttpd.h"
#include "ozwcp.h"
//#include "webserver.h"

using namespace OpenZWave;

MyNode *nodes[MAX_NODES];
int32 MyNode::nodecount = 0;
uint32 homeId = 0;
uint32 nodeId = 0;
bool done = false;
bool needsave = false;
bool noop = false;
uint8 SUCnodeId = 0;
const char *cmode = "";
int32 debug = false;
bool MyNode::nodechanged = false;
list<uint8> MyNode::removed;
extern std::string szUserDataFolder;

//static Webserver *wserver;

#ifdef WIN32
char* strsep(char** stringp, const char* delim)
{
	char* start = *stringp;
	char* p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;

	if (p == NULL)
	{
		*stringp = NULL;
	}
	else
	{
		*p = '\0';
		*stringp = p + 1;
	}

	return start;
}
#endif

/*
 * MyNode::MyNode constructor
 * Just save the nodes into an array and other initialization.
 */
MyNode::MyNode (int32 const ind) : type(0)
{
  if (ind < 1 || ind >= MAX_NODES) {
#ifdef OZW_WRITE_LOG
    Log::Write(LogLevel_Info, "new: bad node value %d, ignoring...", ind);
#endif
    delete this;
    return;
  }
  newGroup(ind);
  setTime(time(NULL));
  setChanged(true);
  nodes[ind] = this;
  nodecount++;
}

/*
 * MyNode::~MyNode destructor
 * Remove stored data.
 */
MyNode::~MyNode ()
{
  while (!values.empty()) {
    MyValue *v = values.back();
    values.pop_back();
    delete v;
  }
  while (!groups.empty()) {
    MyGroup *g = groups.back();
    groups.pop_back();
    delete g;
  }
}

/*
 * MyNode::remove
 * Remove node from array.
 */
void MyNode::remove (int32 const ind)
{
  if (ind < 1 || ind >= MAX_NODES) {
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "remove: bad node value %d, ignoring...", ind);
#endif
    return;
  }
  if (nodes[ind] != NULL) {
    addRemoved(ind);
    delete nodes[ind];
    nodes[ind] = NULL;
    nodecount--;
  }
}

/*
 * compareValue
 * Function to compare values in the vector for sorting.
 */
bool compareValue (MyValue *a, MyValue *b)
{
  return (a->getId() < b->getId());
}

/*
 * MyNode::sortValues
 * Sort the ValueIDs
 */
void MyNode::sortValues ()
{
  sort(values.begin(), values.end(), compareValue);
  setChanged(true);
}
/*
 * MyNode::addValue
 * Per notifications, add a value to a node.
 */
void MyNode::addValue (ValueID id)
{
  MyValue *v = new MyValue(id);
  values.push_back(v);
  setTime(time(NULL));
  setChanged(true);
}

/*
 * MyNode::removeValue
 * Per notification, remove value from node.
 */
void MyNode::removeValue (ValueID id)
{
  vector<MyValue*>::iterator it;
  bool found = false;
  for (it = values.begin(); it != values.end(); it++) {
    if ((*it)->id == id) {
      delete *it;
      values.erase(it);
      found = true;
      break;
    }
  }
  if (!found)
  {
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Error, "removeValue not found Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s\n",
		  id.GetHomeId(), id.GetNodeId(), valueGenreStr(id.GetGenre()),
		  cclassStr(id.GetCommandClassId()), id.GetInstance(), id.GetIndex(),
		  valueTypeStr(id.GetType()));
#endif
  }
  setTime(time(NULL));
  setChanged(true);
}

/*
 * MyNode::saveValue
 * Per notification, update value info. Nothing really but update
 * tracking state.
 */
void MyNode::saveValue (ValueID id)
{
  setTime(time(NULL));
  setChanged(true);
}

/*
 * MyNode::newGroup
 * Get initial group information about a node.
 */
void MyNode::newGroup (uint8 node)
{
	if (Manager::Get() == NULL)
		return;

  int n = Manager::Get()->GetNumGroups(homeId, node);
  for (int i = 1; i <= n; i++) {
    MyGroup *p = new MyGroup();
    p->groupid = i;
    p->max = Manager::Get()->GetMaxAssociations(homeId, node, i);
    p->label = Manager::Get()->GetGroupLabel(homeId, node, i);
    groups.push_back(p);
  }
}

/*
 * MyNode::addGroup
 * Add group membership based on notification updates.
 */
void MyNode::addGroup (uint8 node, uint8 g, uint8 n, uint8 *v)
{
#ifdef OZW_WRITE_LOG
	Log::Write(LogLevel_Info, "addGroup: node %d group %d n %d\n", node, g, n);
#endif
  if (groups.size() == 0)
    newGroup(node);
  for (vector<MyGroup*>::iterator it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == g) {
      (*it)->grouplist.clear();
      for (int i = 0; i < n; i++)
	(*it)->grouplist.push_back(v[i]);
      setTime(time(NULL));
      setChanged(true);
      return;
    }
#ifdef OZW_WRITE_LOG
  Log::Write(LogLevel_Error, "addgroup: node %d group %d not found in list\n", node, g);
#endif
}

/*
 * MyNode::getGroup
 * Return group ptr for XML output
 */
MyGroup *MyNode::getGroup (uint8 i)
{
  for (vector<MyGroup*>::iterator it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == i)
      return *it;
  return NULL;
}

/*
 * MyNode::updateGroup
 * Synchronize changes from user and update to network
 */
void MyNode::updateGroup (uint8 node, uint8 grp, char *glist)
{
  char *p = glist;
  vector<MyGroup*>::iterator it;
  char *np;
  uint8 *v;
  uint8 n;
  uint8 j;
#ifdef OZW_WRITE_LOG
  Log::Write(LogLevel_Info, "updateGroup: node %d group %d\n", node, grp);
#endif
  for (it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == grp)
      break;
  if (it == groups.end()) {
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Error, "updateGroup: node %d group %d not found\n", node, grp);
#endif
    return;
  }
  v = new uint8[(*it)->max];
  n = 0;
  while (p != NULL && *p && n < (*it)->max) {
    np = strsep(&p, ",");
    v[n++] = (uint8)strtol(np, NULL, 10);
  }
  /* Look for nodes in the passed-in argument list, if not present add them */
  vector<uint8>::iterator nit;
  for (j = 0; j < n; j++) {
    for (nit = (*it)->grouplist.begin(); nit != (*it)->grouplist.end(); ++nit)
      if (*nit == v[j])
	break;
    if (nit == (*it)->grouplist.end()) // not found
      Manager::Get()->AddAssociation(homeId, node, grp, v[j]);
  }
  /* Look for nodes in the vector (current list) and those not found in
     the passed-in list need to be removed */
  for (nit = (*it)->grouplist.begin(); nit != (*it)->grouplist.end(); ++nit) {
    for (j = 0; j < n; j++)
      if (*nit == v[j])
	break;
    if (j >= n)
      Manager::Get()->RemoveAssociation(homeId, node, grp, *nit);
  }
  delete [] v;
}

/*
 * Scan list of values to be added to/removed from poll list
 */
void MyNode::updatePoll(char *ilist, char *plist)
{
  vector<char*> ids;
  vector<bool> polls;
  MyValue *v;
  char *p;
  char *np;

  p = ilist;
  while (p != NULL && *p) {
    np = strsep(&p, ",");
    ids.push_back(np);
  }
  p = plist;
  while (p != NULL && *p) {
    np = strsep(&p, ",");
    polls.push_back(*np == '1' ? true : false);
  }
  if (ids.size() != polls.size()) {
#ifdef OZW_WRITE_LOG
    Log::Write(LogLevel_Error, "updatePoll: size of ids %u not same as size of polls %u\n",
	    ids.size(), polls.size());
#endif
    return;
  }
  vector<char*>::iterator it = ids.begin();
  vector<bool>::iterator pit = polls.begin();
  while (it != ids.end() && pit != polls.end()) {
    v = lookup(*it);
    if (v == NULL) {
#ifdef OZW_WRITE_LOG
      Log::Write(LogLevel_Error, "updatePoll: value %s not found\n", *it);
#endif
      continue;
    }
    /* if poll requested, see if not on list */
    if (*pit) {
		if (!Manager::Get()->isPolled(v->getId()))
		{
			if (!Manager::Get()->EnablePoll(v->getId()))
			{
#ifdef OZW_WRITE_LOG
				Log::Write(LogLevel_Error, "updatePoll: enable polling for %s failed\n", *it);
#endif
			}
		}
    } else {			// polling not requested and it is on, turn it off
		if (Manager::Get()->isPolled(v->getId()))
		{
			if (!Manager::Get()->DisablePoll(v->getId()))
			{
#ifdef OZW_WRITE_LOG
				Log::Write(LogLevel_Error, "updatePoll: disable polling for %s failed\n", *it);
#endif
			}
		}
    }
    ++it;
    ++pit;
  }
}

/*
 * Parse textualized value representation in the form of:
 * 2-SWITCH MULTILEVEL-user-byte-1-0
 * node-class-genre-type-instance-index
 */
MyValue *MyNode::lookup (string data)
{
  uint8 node = 0;
  uint8 cls;
  uint8 inst;
  uint8 ind;
  ValueID::ValueGenre vg;
  ValueID::ValueType typ;
  size_t pos1, pos2;
  string str;

  node = (uint8)strtol(data.c_str(), NULL, 10);
  if (node == 0)
    return NULL;
  pos1 = data.find("-", 0);
  if (pos1 == string::npos)
    return NULL;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  cls = cclassNum(str.c_str());
  if (cls == 0xFF)
    return NULL;
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  vg = valueGenreNum(str.c_str());
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  typ = valueTypeNum(str.c_str());
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  inst = (uint8)strtol(str.c_str(), NULL, 10);
  pos1 = pos2 + 1;
  str = data.substr(pos1);
  ind = (uint8)strtol(str.c_str(), NULL, 10);
  ValueID id(homeId, node, vg, cls, inst, ind, typ);
  MyNode *n = nodes[node];
  if (n == NULL)
    return NULL;
  for (vector<MyValue*>::iterator it = n->values.begin(); it != n->values.end(); it++)
    if ((*it)->id == id)
      return *it;
  return NULL;
}

/*
 * Returns a count of values
 */
int32 MyNode::getValueCount ()
{
  return values.size();
}

/*
 * Returns an n'th value
 */
MyValue *MyNode::getValue (uint8 n)
{
  if (n < values.size())
    return values[n];
  return NULL;
}

/*
 * Mark all nodes as changed
 */
void MyNode::setAllChanged (bool ch)
{
  nodechanged = ch;
  int i = 0;
  int j = 1;
  while (j <= nodecount && i < MAX_NODES) {
    if (nodes[i] != NULL) {
      nodes[i]->setChanged(true);
      j++;
    }
    i++;
  }
}

/*
 * Returns next item on the removed list.
 */

uint8 MyNode::getRemoved()
{
  if (removed.size() > 0) {
    uint8 node = removed.front();
    removed.pop_front();
    return node;
  }
  return 0;
}

void COpenZWaveControlPanel::SetAllNodesChanged()
{
	MyNode::setAllChanged(true);
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void COpenZWaveControlPanel::OnCPNotification(Notification const* _notification)
{
  ValueID id = _notification->GetValueID();
  int nodeID = _notification->GetNodeId();

  switch (_notification->GetType()) {
  case Notification::Type_ValueAdded:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Value Added Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
		if (nodes[nodeID] == NULL)
			return;
		nodes[nodeID]->addValue(id);
		nodes[nodeID]->setTime(time(NULL));
		nodes[nodeID]->setChanged(true);
	    break;
  case Notification::Type_ValueRemoved:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Value Removed Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
		if (nodes[nodeID] == NULL)
			return;
		nodes[nodeID]->removeValue(id);
		nodes[nodeID]->setTime(time(NULL));
		nodes[nodeID]->setChanged(true);
    break;
  case Notification::Type_ValueChanged:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Value Changed Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
		if (nodes[nodeID] == NULL)
			return;
		nodes[nodeID]->saveValue(id);
		break;
  case Notification::Type_ValueRefreshed:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Value Refreshed Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->setTime(time(NULL));
    nodes[_notification->GetNodeId()]->setChanged(true);
    break;
  case Notification::Type_Group:
    {
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Group Home 0x%08x Node %d Group %d",
		 _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetGroupIdx());
#endif
      uint8 *v = NULL;
      int8 n = Manager::Get()->GetAssociations(homeId, _notification->GetNodeId(), _notification->GetGroupIdx(), &v);
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->addGroup(_notification->GetNodeId(), _notification->GetGroupIdx(), n, v);
      if (v != NULL)
	delete [] v;
    }
    break;
  case Notification::Type_NodeNew:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node New Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
    needsave = true;
    break;
  case Notification::Type_NodeAdded:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node Added Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
	  new MyNode(_notification->GetNodeId());
    needsave = true;
    break;
  case Notification::Type_NodeRemoved:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node Removed Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
    MyNode::remove(_notification->GetNodeId());
    needsave = true;
    break;
  case Notification::Type_NodeProtocolInfo:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node Protocol Info Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->saveValue(id);
    needsave = true;
    break;
  case Notification::Type_NodeNaming:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node Naming Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->saveValue(id);
    break;
  case Notification::Type_NodeEvent:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node Event Home %08x Node %d Status %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetEvent(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->saveValue(id);
    break;
  case Notification::Type_PollingDisabled:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Polling Disabled Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
    //nodes[_notification->GetNodeId()]->setPolled(false);
    break;
  case Notification::Type_PollingEnabled:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Polling Enabled Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
#endif
    //nodes[_notification->GetNodeId()]->setPolled(true);
    break;
  case Notification::Type_SceneEvent:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Scene Event Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s Scene Id %d",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()), _notification->GetSceneId());
#endif
    break;
  case Notification::Type_CreateButton:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Create button Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
#endif
    break;
  case Notification::Type_DeleteButton:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Delete button Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
#endif
    break;
  case Notification::Type_ButtonOn:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Button On Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
#endif
    break;
  case Notification::Type_ButtonOff:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Button Off Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
#endif
    break;
  case Notification::Type_DriverReady:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Driver Ready, homeId %08x, nodeId %d", _notification->GetHomeId(),
	       _notification->GetNodeId());
#endif
    homeId = _notification->GetHomeId();
    nodeId = _notification->GetNodeId();
    if (Manager::Get()->IsStaticUpdateController(homeId)) {
      cmode = "SUC";
      SUCnodeId = Manager::Get()->GetSUCNodeId(homeId);
    } else if (Manager::Get()->IsPrimaryController(homeId))
      cmode = "Primary";
    else
      cmode = "Slave";
    break;
  case Notification::Type_DriverFailed:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Driver Failed, homeId %08x", _notification->GetHomeId());
#endif
    done = false;
    needsave = false;
    homeId = 0;
    cmode = "";
    for (int i = 1; i < MAX_NODES; i++)
      MyNode::remove(i);
    break;
  case Notification::Type_DriverReset:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Driver Reset, homeId %08x", _notification->GetHomeId());
#endif
    done = false;
    needsave = true;
    homeId = _notification->GetHomeId();
    if (Manager::Get()->IsStaticUpdateController(homeId)) {
      cmode = "SUC";
      SUCnodeId = Manager::Get()->GetSUCNodeId(homeId);
    } else if (Manager::Get()->IsPrimaryController(homeId))
      cmode = "Primary";
    else
      cmode = "Slave";
    for (int i = 1; i < MAX_NODES; i++)
      MyNode::remove(i);
    break;
  case Notification::Type_EssentialNodeQueriesComplete:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Essential Node %d Queries Complete", _notification->GetNodeId());
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->setTime(time(NULL));
    nodes[_notification->GetNodeId()]->setChanged(true);
    break;
  case Notification::Type_NodeQueriesComplete:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Node %d Queries Complete", _notification->GetNodeId());
#endif
	  if (nodes[nodeID] == NULL)
		  return;
	  nodes[_notification->GetNodeId()]->sortValues();
    nodes[_notification->GetNodeId()]->setTime(time(NULL));
    nodes[_notification->GetNodeId()]->setChanged(true);
    needsave = true;
    break;
  case Notification::Type_AwakeNodesQueried:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Awake Nodes Queried");
#endif
    break;
  case Notification::Type_AllNodesQueriedSomeDead:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: Awake Nodes Queried Some Dead");
#endif
    break;
  case Notification::Type_AllNodesQueried:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: All Nodes Queried");
#endif
    break;
  case Notification::Type_Notification:
    switch (_notification->GetNotification()) {
    case Notification::Code_MsgComplete:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Message Complete",
		 _notification->GetHomeId(), _notification->GetNodeId());
#endif
      break;
    case Notification::Code_Timeout:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Timeout",
		 _notification->GetHomeId(), _notification->GetNodeId());
#endif
      break;
    case Notification::Code_NoOperation:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d No Operation Message Complete",
		 _notification->GetHomeId(), _notification->GetNodeId());
#endif
      noop = true;
      break;
    case Notification::Code_Awake:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Awake",
		 _notification->GetHomeId(), _notification->GetNodeId());
#endif
		if (nodes[nodeID] == NULL)
			return;
		nodes[_notification->GetNodeId()]->setTime(time(NULL));
      nodes[_notification->GetNodeId()]->setChanged(true);
      break;
    case Notification::Code_Sleep:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Sleep",
			_notification->GetHomeId(), _notification->GetNodeId());
#endif
		{
			if (nodes[nodeID] == NULL)
				return;
			nodes[nodeID]->setTime(time(NULL));
			nodes[nodeID]->setChanged(true);
		}
	    break;
	case Notification::Code_Dead:
		{
#ifdef OZW_WRITE_LOG
			Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Dead",
				_notification->GetHomeId(), _notification->GetNodeId());
#endif
			if (nodes[nodeID] == NULL)
				return;
			nodes[nodeID]->setTime(time(NULL));
			nodes[nodeID]->setChanged(true);
		}
      break;
    default:
#ifdef OZW_WRITE_LOG
		Log::Write(LogLevel_Info, "Notification: Notification home %08x node %d Unknown %d",
		 _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetNotification());
#endif
      break;
    }
    break;
  default:
#ifdef OZW_WRITE_LOG
	  Log::Write(LogLevel_Info, "Notification: type %d home %08x node %d genre %d class %d instance %d index %d type %d",
	       _notification->GetType(), _notification->GetHomeId(),
	       _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(),
	       id.GetInstance(), id.GetIndex(), id.GetType());
#endif
    break;
  }
}


COpenZWaveControlPanel::COpenZWaveControlPanel() :
logbytes(0),
adminstate(false),
ready(true)
{
}

COpenZWaveControlPanel::~COpenZWaveControlPanel()
{

}

/*
* web_controller_update
* Handle controller function feedback from library.
*/

void web_controller_update(Driver::ControllerState cs, Driver::ControllerError err, void *ct)
{
	COpenZWaveControlPanel *cp = (COpenZWaveControlPanel *)ct;
	string s;
	bool more = true;

	switch (cs) {
	case Driver::ControllerState_Normal:
		s = ": no command in progress.";
		break;
	case Driver::ControllerState_Starting:
		s = ": starting controller command.";
		break;
	case Driver::ControllerState_Cancel:
		s = ": command was cancelled.";
		more = false;
		break;
	case Driver::ControllerState_Error:
		s = ": command returned an error: ";
		more = false;
		break;
	case Driver::ControllerState_Sleeping:
		s = ": device went to sleep.";
		more = false;
		break;
	case Driver::ControllerState_Waiting:
		s = ": waiting for a user action.";
		break;
	case Driver::ControllerState_InProgress:
		s = ": communicating with the other device.";
		break;
	case Driver::ControllerState_Completed:
		s = ": command has completed successfully.";
		more = false;
		break;
	case Driver::ControllerState_Failed:
		s = ": command has failed.";
		more = false;
		break;
	case Driver::ControllerState_NodeOK:
		s = ": the node is OK.";
		more = false;
		break;
	case Driver::ControllerState_NodeFailed:
		s = ": the node has failed.";
		more = false;
		break;
	default:
		s = ": unknown response.";
		break;
	}
	if (err != Driver::ControllerError_None)
		s = s + controllerErrorStr(err);
	cp->setAdminMessage(s);
	cp->setAdminState(more);
}


/*
* web_get_groups
* Return some XML to carry node group associations
*/

void COpenZWaveControlPanel::web_get_groups(int n, TiXmlElement *ep)
{
	int cnt = nodes[n]->numGroups();
	int i;

	TiXmlElement* groupsElement = new TiXmlElement("groups");
	ep->LinkEndChild(groupsElement);
	groupsElement->SetAttribute("cnt", cnt);
	for (i = 1; i <= cnt; i++) {
		TiXmlElement* groupElement = new TiXmlElement("group");
		MyGroup *p = nodes[n]->getGroup(i);
		groupElement->SetAttribute("ind", i);
		groupElement->SetAttribute("max", p->max);
		groupElement->SetAttribute("label", p->label.c_str());
		string str = "";
		for (unsigned int j = 0; j < p->grouplist.size(); j++) {
			char s[12];
			snprintf(s, sizeof(s), "%d", p->grouplist[j]);
			str += s;
			if (j + 1 < p->grouplist.size())
				str += ",";
		}
		TiXmlText *textElement = new TiXmlText(str.c_str());
		groupElement->LinkEndChild(textElement);
		groupsElement->LinkEndChild(groupElement);
	}
}

/*
* web_get_values
* Retreive class values based on genres
*/
void COpenZWaveControlPanel::web_get_values(int i, TiXmlElement *ep)
{
	if (Manager::Get() == NULL)
		return;

	int32 idcnt = nodes[i]->getValueCount();

	for (int j = 0; j < idcnt; j++) {
		TiXmlElement* valueElement = new TiXmlElement("value");
		MyValue *vals = nodes[i]->getValue(j);
		ValueID id = vals->getId();
		valueElement->SetAttribute("genre", valueGenreStr(id.GetGenre()));
		valueElement->SetAttribute("type", valueTypeStr(id.GetType()));
		valueElement->SetAttribute("class", cclassStr(id.GetCommandClassId()));
		valueElement->SetAttribute("instance", id.GetInstance());
		valueElement->SetAttribute("index", id.GetIndex());
		valueElement->SetAttribute("label", Manager::Get()->GetValueLabel(id).c_str());
		valueElement->SetAttribute("units", Manager::Get()->GetValueUnits(id).c_str());
		valueElement->SetAttribute("readonly", Manager::Get()->IsValueReadOnly(id) ? "true" : "false");
		if (id.GetGenre() != ValueID::ValueGenre_Config)
			valueElement->SetAttribute("polled", Manager::Get()->isPolled(id) ? "true" : "false");
		if (id.GetType() == ValueID::ValueType_List) {
			vector<string> strs;
			Manager::Get()->GetValueListItems(id, &strs);
			valueElement->SetAttribute("count", strs.size());
			string str;
			Manager::Get()->GetValueListSelection(id, &str);
			valueElement->SetAttribute("current", str.c_str());
			for (vector<string>::iterator it = strs.begin(); it != strs.end(); it++) {
				TiXmlElement* itemElement = new TiXmlElement("item");
				valueElement->LinkEndChild(itemElement);
				TiXmlText *textElement = new TiXmlText((*it).c_str());
				itemElement->LinkEndChild(textElement);
			}
		}
		else {
			string str;
			TiXmlText *textElement;
			if (Manager::Get()->GetValueAsString(id, &str))
			{
				//make valid string
				for (size_t ii = 0; ii < str.size(); ii++)
				{
					if (str[ii] < 0x20)
						str[ii] = ' ';
				}
				textElement = new TiXmlText(str.c_str());
			}
			else
				textElement = new TiXmlText("");
			if (id.GetType() == ValueID::ValueType_Decimal) {
				uint8 precision;
				if (Manager::Get()->GetValueFloatPrecision(id, &precision))
				{
#ifdef OZW_WRITE_LOG
					Log::Write(LogLevel_Info,"node = %d id = %d value = %s precision = %d\n", i, j, str.c_str(), precision);
#endif
				}
			}
			valueElement->LinkEndChild(textElement);
		}

		string str = Manager::Get()->GetValueHelp(id);
		if (str.length() > 0) {
			TiXmlElement* helpElement = new TiXmlElement("help");
			TiXmlText *textElement = new TiXmlText(str.c_str());
			helpElement->LinkEndChild(textElement);
			valueElement->LinkEndChild(helpElement);
		}
		ep->LinkEndChild(valueElement);
	}
}

/*
* SendPollResponse
* Process poll request from client and return
* data as xml.
*/
std::string COpenZWaveControlPanel::SendPollResponse()
{
	TiXmlDocument doc;
	struct stat buf;
	const int logbufsz = 1024;	// max amount to send of log per poll
	char logbuffer[logbufsz + 1];
	off_t bcnt;
	char str[16];
	int32 i, j;
	int32 logread = 0;
	FILE *fp;

	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* pollElement = new TiXmlElement("poll");
	doc.LinkEndChild(pollElement);
	if (homeId != 0L)
		snprintf(str, sizeof(str), "%08x", homeId);
	else
		str[0] = '\0';
	pollElement->SetAttribute("homeid", str);
	if (nodeId != 0)
		snprintf(str, sizeof(str), "%d", nodeId);
	else
		str[0] = '\0';
	pollElement->SetAttribute("nodeid", str);
	snprintf(str, sizeof(str), "%d", SUCnodeId);
	pollElement->SetAttribute("sucnodeid", str);
	pollElement->SetAttribute("nodecount", MyNode::getNodeCount());
	pollElement->SetAttribute("cmode", cmode);
	pollElement->SetAttribute("save", needsave);
	pollElement->SetAttribute("noop", noop);
	if (noop)
		noop = false;
	bcnt = logbytes;
	if (stat("./Config/OZW_Log.txt", &buf) != -1 &&
		buf.st_size > bcnt &&
		(fp = fopen("./Config/OZW_Log.txt", "r")) != NULL) 
	{
		if (bcnt == 0)
		{
			if (buf.st_size > 100)
				bcnt = buf.st_size - 100;
		}
		if (fseek(fp, bcnt, SEEK_SET) != -1) {
			logread = fread(logbuffer, 1, logbufsz, fp);
			while (logread > 0 && logbuffer[--logread] != '\n')
				;
			logbytes = bcnt + logread;
			fclose(fp);
		}
	}
	logbuffer[logread] = '\0';

	TiXmlElement* logElement = new TiXmlElement("log");
	pollElement->LinkEndChild(logElement);
	logElement->SetAttribute("size", logread);
	logElement->SetAttribute("offset", logbytes - logread);
	TiXmlText *textElement = new TiXmlText(logbuffer);
	logElement->LinkEndChild(textElement);

	TiXmlElement* adminElement = new TiXmlElement("admin");
	pollElement->LinkEndChild(adminElement);
	adminElement->SetAttribute("active", getAdminState() ? "true" : "false");
	if (adminmsg.length() > 0) {
		string msg = getAdminFunction() + getAdminMessage();
		TiXmlText *textElement = new TiXmlText(msg.c_str());
		adminElement->LinkEndChild(textElement);
		adminmsg.clear();
	}

	TiXmlElement* updateElement = new TiXmlElement("update");
	pollElement->LinkEndChild(updateElement);
	i = MyNode::getRemovedCount();
	if (i > 0) {
		logbuffer[0] = '\0';
		while (i > 0) {
			uint8 node = MyNode::getRemoved();
			snprintf(str, sizeof(str), "%d", node);
			strcat(logbuffer, str);
			i = MyNode::getRemovedCount();
			if (i > 0)
				strcat(logbuffer, ",");
		}
		updateElement->SetAttribute("remove", logbuffer);
	}

	if (MyNode::getAnyChanged()) {
		i = 0;
		j = 1;
		while (j <= MyNode::getNodeCount() && i < MAX_NODES) {
			if (nodes[i] != NULL && nodes[i]->getChanged()) {
				bool listening;
				bool flirs;
				bool zwaveplus;
				TiXmlElement* nodeElement = new TiXmlElement("node");
				pollElement->LinkEndChild(nodeElement);
				nodeElement->SetAttribute("id", i);
				zwaveplus = Manager::Get()->IsNodeZWavePlus(homeId, i); 
				if (zwaveplus) {
					string value = Manager::Get()->GetNodePlusTypeString(homeId, i);
					value += " " + Manager::Get()->GetNodeRoleString(homeId, i);
					nodeElement->SetAttribute("btype", value.c_str());
					nodeElement->SetAttribute("gtype", Manager::Get()->GetNodeDeviceTypeString(homeId, i).c_str());
				}
				else {
					nodeElement->SetAttribute("btype", nodeBasicStr(Manager::Get()->GetNodeBasic(homeId, i)));
					nodeElement->SetAttribute("gtype", Manager::Get()->GetNodeType(homeId, i).c_str());
				}
				nodeElement->SetAttribute("name", Manager::Get()->GetNodeName(homeId, i).c_str());
				nodeElement->SetAttribute("location", Manager::Get()->GetNodeLocation(homeId, i).c_str());
				nodeElement->SetAttribute("manufacturer", Manager::Get()->GetNodeManufacturerName(homeId, i).c_str());
				nodeElement->SetAttribute("product", Manager::Get()->GetNodeProductName(homeId, i).c_str());
				listening = Manager::Get()->IsNodeListeningDevice(homeId, i);
				nodeElement->SetAttribute("listening", listening ? "true" : "false");
				flirs = Manager::Get()->IsNodeFrequentListeningDevice(homeId, i);
				nodeElement->SetAttribute("frequent", flirs ? "true" : "false");
				nodeElement->SetAttribute("zwaveplus", zwaveplus ? "true" : "false");
				nodeElement->SetAttribute("beam", Manager::Get()->IsNodeBeamingDevice(homeId, i) ? "true" : "false");
				nodeElement->SetAttribute("routing", Manager::Get()->IsNodeRoutingDevice(homeId, i) ? "true" : "false");
				nodeElement->SetAttribute("security", Manager::Get()->IsNodeSecurityDevice(homeId, i) ? "true" : "false");
				nodeElement->SetAttribute("time", (int)nodes[i]->getTime()); 
#ifdef OZW_WRITE_LOG
				Log::Write(LogLevel_Info,"i=%d failed=%d\n", i, Manager::Get()->IsNodeFailed(homeId, i));
				Log::Write(LogLevel_Info,"i=%d awake=%d\n", i, Manager::Get()->IsNodeAwake(homeId, i));
				Log::Write(LogLevel_Info,"i=%d state=%s\n", i, Manager::Get()->GetNodeQueryStage(homeId, i).c_str());
				Log::Write(LogLevel_Info,"i=%d listening=%d flirs=%d\n", i, listening, flirs);
#endif
				if (Manager::Get()->IsNodeFailed(homeId, i))
					nodeElement->SetAttribute("status", "Dead");
				else {
					string s = Manager::Get()->GetNodeQueryStage(homeId, i);
					if (s == "Complete") {
						if (i != nodeId && !listening && !flirs)
							nodeElement->SetAttribute("status", Manager::Get()->IsNodeAwake(homeId, i) ? "Awake" : "Sleeping");
						else
							nodeElement->SetAttribute("status", "Ready");
					}
					else {
						if (i != nodeId && !listening && !flirs)
							s = s + (Manager::Get()->IsNodeAwake(homeId, i) ? " (awake)" : " (sleeping)");
						nodeElement->SetAttribute("status", s.c_str());
					}
				} 
				web_get_groups(i, nodeElement);
				// Don't think the UI needs these
				//web_get_genre(ValueID::ValueGenre_Basic, i, nodeElement);
				web_get_values(i, nodeElement);
				nodes[i]->setChanged(false);
				j++;
			}
			i++;
		}
	}
	char fntemp[200];
	sprintf(fntemp, "%sozwcp.poll.XXXXXX.xml", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}
	return retstring;
}

std::string COpenZWaveControlPanel::SendNodeConfResponse(int node_id)
{
	Manager::Get()->RequestAllConfigParams(homeId, node_id);
	return "OK";
}

std::string COpenZWaveControlPanel::SendNodeValuesResponse(int node_id)
{
	Manager::Get()->RequestNodeDynamic(homeId, node_id);
	return "OK";
}

std::string COpenZWaveControlPanel::SetNodeValue(const std::string &arg1, const std::string &arg2)
{
	MyValue *val = MyNode::lookup(arg1);
	if (val != NULL) {
		if (!Manager::Get()->SetValue(val->getId(), arg2))
		{
#ifdef OZW_WRITE_LOG
			Log::Write(LogLevel_Error, "SetValue string failed type=%s\n", valueTypeStr(val->getId().GetType()));
#endif
		}
	}
	return "OK";
}

std::string COpenZWaveControlPanel::SetNodeButton(const std::string &arg1, const std::string &arg2)
{
	MyValue *val = MyNode::lookup(arg1);
	if (val != NULL) {
		if (arg2 == "true") {
			if (!Manager::Get()->PressButton(val->getId()))
			{
#ifdef OZW_WRITE_LOG
				Log::Write(LogLevel_Error, "PressButton failed");
#endif
			}
		}
		else {
			if (!Manager::Get()->ReleaseButton(val->getId()))
			{
#ifdef OZW_WRITE_LOG
				Log::Write(LogLevel_Error, "ReleaseButton failed");
#endif
			}
		}
	}
	return "OK";
}
std::string COpenZWaveControlPanel::DoAdminCommand(const std::string &fun, const int node_id, const int button_id)
{
	if (fun == "cancel") { /* cancel controller function */
		Manager::Get()->CancelControllerCommand(homeId);
		setAdminState(false);
	}
	else if (fun == "addd") {
		setAdminFunction("Add Device");
		setAdminState(Manager::Get()->AddNode(homeId, false));
	}
	else if (fun == "addds") {
		setAdminFunction("Add Device");
		setAdminState(Manager::Get()->AddNode(homeId, true));
	}
	else if (fun == "cprim") {
		setAdminFunction("Create Primary");
		setAdminState(Manager::Get()->CreateNewPrimary(homeId));
	}
	else if (fun == "rconf") {
		setAdminFunction("Receive Configuration");
		setAdminState(Manager::Get()->ReceiveConfiguration(homeId));
	}
	else if (fun == "remd") {
		setAdminFunction("Remove Device");
		setAdminState(Manager::Get()->RemoveNode(homeId));
	}
	else if (fun == "hnf") {
		setAdminFunction("Has Node Failed");
		setAdminState(Manager::Get()->HasNodeFailed(homeId, node_id));
	}
	else if (fun == "remfn") {
		setAdminFunction("Remove Failed Node");
		setAdminState(Manager::Get()->RemoveFailedNode(homeId, node_id));
	}
	else if (fun == "repfn") {
		setAdminFunction("Replace Failed Node");
		setAdminState(Manager::Get()->ReplaceFailedNode(homeId, node_id));
	}
	else if (fun == "tranpr") {
		setAdminFunction("Transfer Primary Role");
		setAdminState(Manager::Get()->TransferPrimaryRole(homeId));
	}
	else if (fun == "reqnu") {
		setAdminFunction("Request Network Update");
		setAdminState(Manager::Get()->RequestNetworkUpdate(homeId, node_id));
	}
	else if (fun == "reqnnu") {
		setAdminFunction("Request Node Neighbor Update");
		setAdminState(Manager::Get()->RequestNodeNeighborUpdate(homeId, node_id));
	}
	else if (fun == "assrr") {
		setAdminFunction("Assign Return Route");
		setAdminState(Manager::Get()->AssignReturnRoute(homeId, node_id));
	}
	else if (fun == "delarr") {
		setAdminFunction("Delete All Return Routes");
		setAdminState(Manager::Get()->DeleteAllReturnRoutes(homeId, node_id));
	}
	else if (fun == "snif") {
		setAdminFunction("Send Node Information");
		setAdminState(Manager::Get()->SendNodeInformation(homeId, node_id));
	}
	else if (fun == "reps") {
		setAdminFunction("Replication Send");
		setAdminState(Manager::Get()->ReplicationSend(homeId, node_id));
	}
	else if (fun == "addbtn") {
		setAdminFunction("Add Button");
		setAdminState(Manager::Get()->CreateButton(homeId, node_id, button_id));
	}
	else if (fun == "delbtn") {
		setAdminFunction("Delete Button");
		setAdminState(Manager::Get()->DeleteButton(homeId, node_id, button_id));
	}
	else if (fun == "refreshnode") {
		Manager::Get()->RefreshNodeInfo(homeId, node_id);
	}
	return "OK";
}

std::string COpenZWaveControlPanel::DoSceneCommand(const std::string &fun, const std::string &arg1, const std::string &arg2, const std::string &arg3)
{
	TiXmlDocument doc;
	char str[16];
	string s;
	int cnt;
	int i;
	uint8 sid;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* scenesElement = new TiXmlElement("scenes");
	doc.LinkEndChild(scenesElement);

	if (fun == "create") {
		sid = Manager::Get()->CreateScene();
		if (sid == 0) {
			fprintf(stderr, "sid = 0, out of scene ids\n");
			return "";
		}
	}
	if ((fun == "values") ||
		(fun == "label") ||
		(fun == "delete") ||
		(fun == "execute") ||
		(fun == "addvalue") ||
		(fun == "update") ||
		(fun == "remove")) {
		sid = (uint8)atoi(arg1.c_str());
		if (fun == "delete")
			Manager::Get()->RemoveScene(sid);
		else if (fun == "execute")
			Manager::Get()->ActivateScene(sid);
		else if (fun == "label")
			Manager::Get()->SetSceneLabel(sid, string(arg2));
		if ((fun == "addvalue") ||
			(fun == "update") ||
			(fun == "remove")) {
			MyValue *val = MyNode::lookup(string(arg2));
			if (val != NULL) {
				if (fun == "addvalue") {
					if (!Manager::Get()->AddSceneValue(sid, val->getId(), string(arg3)))
						fprintf(stderr, "AddSceneValue failure\n");
				}
				else if (fun == "update") {
					if (!Manager::Get()->SetSceneValue(sid, val->getId(), string(arg3)))
						fprintf(stderr, "SetSceneValue failure\n");
				}
				else if (fun == "remove") {
					if (!Manager::Get()->RemoveSceneValue(sid, val->getId()))
						fprintf(stderr, "RemoveSceneValue failure\n");
				}
			}
		}
	}
	if ((fun=="load") ||
		(fun=="create") ||
		(fun=="label") ||
		(fun=="delete")) { // send all sceneids
		uint8 *sptr;
		cnt = Manager::Get()->GetAllScenes(&sptr);
		scenesElement->SetAttribute("sceneid", cnt);
		for (i = 0; i < cnt; i++) {
			TiXmlElement* sceneElement = new TiXmlElement("sceneid");
			snprintf(str, sizeof(str), "%d", sptr[i]);
			sceneElement->SetAttribute("id", str);
			s = Manager::Get()->GetSceneLabel(sptr[i]);
			sceneElement->SetAttribute("label", s.c_str());
			scenesElement->LinkEndChild(sceneElement);
		}
		delete[] sptr;
	}
	if ((fun=="values") ||
		(fun=="addvalue") ||
		(fun=="remove") ||
		(fun=="update")) {
		vector<ValueID> vids;
		cnt = Manager::Get()->SceneGetValues(sid, &vids);
		scenesElement->SetAttribute("scenevalue", cnt);
		for (vector<ValueID>::iterator it = vids.begin(); it != vids.end(); it++) {
			TiXmlElement* valueElement = new TiXmlElement("scenevalue");
			valueElement->SetAttribute("id", sid);
			snprintf(str, sizeof(str), "0x%x", (*it).GetHomeId());
			valueElement->SetAttribute("home", str);
			valueElement->SetAttribute("node", (*it).GetNodeId());
			valueElement->SetAttribute("class", cclassStr((*it).GetCommandClassId()));
			valueElement->SetAttribute("instance", (*it).GetInstance());
			valueElement->SetAttribute("index", (*it).GetIndex());
			valueElement->SetAttribute("type", valueTypeStr((*it).GetType()));
			valueElement->SetAttribute("genre", valueGenreStr((*it).GetGenre()));
			valueElement->SetAttribute("label", Manager::Get()->GetValueLabel(*it).c_str());
			valueElement->SetAttribute("units", Manager::Get()->GetValueUnits(*it).c_str());
			Manager::Get()->SceneGetValueAsString(sid, *it, &s);
			TiXmlText *textElement = new TiXmlText(s.c_str());
			valueElement->LinkEndChild(textElement);
			scenesElement->LinkEndChild(valueElement);
		}
	}
	char fntemp[200];
	sprintf(fntemp, "%sozwcp.scenes.XXXXXX", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}

	return retstring;
}

std::string COpenZWaveControlPanel::DoNodeChange(const std::string &fun, const int node_id, const std::string &svalue)
{
	if (fun == "nam") { /* Node naming */
		Manager::Get()->SetNodeName(homeId, node_id, svalue.c_str());
	}
	else if (fun == "loc") { /* Node location */
		Manager::Get()->SetNodeLocation(homeId, node_id, svalue.c_str());
	}
	else if (fun == "pol") { /* Node polling */
	}
	return "OK";
}

std::string COpenZWaveControlPanel::UpdateGroup(const std::string &fun, const int node_id, const int group_id, const std::string &gList)
{
	if ((node_id == 0) || (node_id > 254))
		return "ERR";
	if (nodes[node_id] == NULL)
		return "ERR";
	char *szGList = strdup(gList.c_str());
	nodes[node_id]->updateGroup(node_id, group_id, szGList);
	free(szGList);
	return "OK";
}


std::string COpenZWaveControlPanel::SaveConfig()
{
	Manager::Get()->WriteConfig(homeId);
	return "OK";
}

std::string COpenZWaveControlPanel::DoTestNetwork(const int node_id, const int cnt)
{
	TiXmlDocument doc;

	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* testElement = new TiXmlElement("testheal");
	doc.LinkEndChild(testElement);

	if (node_id == 0)
		Manager::Get()->TestNetwork(homeId, cnt);
	else
		Manager::Get()->TestNetworkNode(homeId, node_id, cnt);
	
	char fntemp[200];
	sprintf(fntemp, "%sozwcp.testheal.XXXXXX", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}

	return retstring;
}

std::string COpenZWaveControlPanel::HealNetworkNode(const int node_id, const bool healrrs)
{
	TiXmlDocument doc;

	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* testElement = new TiXmlElement("testheal");
	doc.LinkEndChild(testElement);

	if (node_id == 0)
		Manager::Get()->HealNetwork(homeId, healrrs);
	else
		Manager::Get()->HealNetworkNode(homeId, node_id, healrrs);

	char fntemp[200];
	sprintf(fntemp, "%sozwcp.testheal.XXXXXX", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}

	return retstring;
}

std::string COpenZWaveControlPanel::GetCPTopo()
{
	TiXmlDocument doc;
	char str[16];
	unsigned int i, j, k;
	uint8 cnt;
	uint32 len;
	uint8 *neighbors;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* topoElement = new TiXmlElement("topo");
	doc.LinkEndChild(topoElement);

	cnt = MyNode::getNodeCount();
	i = 0;
	j = 1;
	while (j <= cnt && i < MAX_NODES) {
		if (nodes[i] != NULL) {
			len = Manager::Get()->GetNodeNeighbors(homeId, i, &neighbors);
			if (len > 0) {
				TiXmlElement* nodeElement = new TiXmlElement("node");
				snprintf(str, sizeof(str), "%d", i);
				nodeElement->SetAttribute("id", str);
				string list = "";
				for (k = 0; k < len; k++) {
					snprintf(str, sizeof(str), "%d", neighbors[k]);
					list += str;
					if (k < (len - 1))
						list += ",";
				}
				fprintf(stderr, "topo: node=%d %s\n", i, list.c_str());
				TiXmlText *textElement = new TiXmlText(list.c_str());
				nodeElement->LinkEndChild(textElement);
				topoElement->LinkEndChild(nodeElement);
				delete[] neighbors;
			}
			j++;
		}
		i++;
	}
	char fntemp[200];
	sprintf(fntemp, "%sozwcp.topo.XXXXXX", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}

	return retstring;
}

TiXmlElement *COpenZWaveControlPanel::newstat(char const *tag, char const *label, uint32 const value)
{
	char str[32];

	TiXmlElement* statElement = new TiXmlElement(tag);
	statElement->SetAttribute("label", label);
	snprintf(str, sizeof(str), "%d", value);
	TiXmlText *textElement = new TiXmlText(str);
	statElement->LinkEndChild(textElement);
	return statElement;
}

TiXmlElement *COpenZWaveControlPanel::newstat(char const *tag, char const *label, char const *value)
{
	TiXmlElement* statElement = new TiXmlElement(tag);
	statElement->SetAttribute("label", label);
	TiXmlText *textElement = new TiXmlText(value);
	statElement->LinkEndChild(textElement);
	return statElement;
}

std::string COpenZWaveControlPanel::GetCPStats()
{
	TiXmlDocument doc;

	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);
	TiXmlElement* statElement = new TiXmlElement("stats");
	doc.LinkEndChild(statElement);

	struct Driver::DriverData data;
	int i, j;
	int cnt;
	char str[16];

	Manager::Get()->GetDriverStatistics(homeId, &data);

	TiXmlElement* errorsElement = new TiXmlElement("errors");
	errorsElement->LinkEndChild(newstat("stat", "ACK Waiting", data.m_ACKWaiting));
	errorsElement->LinkEndChild(newstat("stat", "Read Aborts", data.m_readAborts));
	errorsElement->LinkEndChild(newstat("stat", "Bad Checksums", data.m_badChecksum));
	errorsElement->LinkEndChild(newstat("stat", "CANs", data.m_CANCnt));
	errorsElement->LinkEndChild(newstat("stat", "NAKs", data.m_NAKCnt));
	errorsElement->LinkEndChild(newstat("stat", "Out of Frame", data.m_OOFCnt));
	statElement->LinkEndChild(errorsElement);

	TiXmlElement* countsElement = new TiXmlElement("counts");
	countsElement->LinkEndChild(newstat("stat", "SOF", data.m_SOFCnt));
	countsElement->LinkEndChild(newstat("stat", "Total Reads", data.m_readCnt));
	countsElement->LinkEndChild(newstat("stat", "Total Writes", data.m_writeCnt));
	countsElement->LinkEndChild(newstat("stat", "ACKs", data.m_ACKCnt));
	countsElement->LinkEndChild(newstat("stat", "Total Broadcasts Received", data.m_broadcastReadCnt));
	countsElement->LinkEndChild(newstat("stat", "Total Broadcasts Transmitted", data.m_broadcastWriteCnt));
	statElement->LinkEndChild(countsElement);

	TiXmlElement* infoElement = new TiXmlElement("info");
	infoElement->LinkEndChild(newstat("stat", "Dropped", data.m_dropped));
	infoElement->LinkEndChild(newstat("stat", "Retries", data.m_retries));
	infoElement->LinkEndChild(newstat("stat", "Unexpected Callbacks", data.m_callbacks));
	infoElement->LinkEndChild(newstat("stat", "Bad Routes", data.m_badroutes));
	infoElement->LinkEndChild(newstat("stat", "No ACK", data.m_noack));
	infoElement->LinkEndChild(newstat("stat", "Network Busy", data.m_netbusy));
	infoElement->LinkEndChild(newstat("stat", "Not Idle", data.m_notidle));
	infoElement->LinkEndChild(newstat("stat", "Non Delivery", data.m_nondelivery));
	infoElement->LinkEndChild(newstat("stat", "Routes Busy", data.m_routedbusy));
	statElement->LinkEndChild(infoElement);

	cnt = MyNode::getNodeCount();
	i = 0;
	j = 1;
	while (j <= cnt && i < MAX_NODES) {
		struct Node::NodeData ndata;

		if (nodes[i] != NULL) {
			Manager::Get()->GetNodeStatistics(homeId, i, &ndata);
			TiXmlElement* nodeElement = new TiXmlElement("node");
			snprintf(str, sizeof(str), "%d", i);
			nodeElement->SetAttribute("id", str);
			nodeElement->LinkEndChild(newstat("nstat", "Sent messages", ndata.m_sentCnt));
			nodeElement->LinkEndChild(newstat("nstat", "Failed sent messages", ndata.m_sentFailed));
			nodeElement->LinkEndChild(newstat("nstat", "Retried sent messages", ndata.m_retries));
			nodeElement->LinkEndChild(newstat("nstat", "Received messages", ndata.m_receivedCnt));
			nodeElement->LinkEndChild(newstat("nstat", "Received duplicates", ndata.m_receivedDups));
			nodeElement->LinkEndChild(newstat("nstat", "Received unsolicited", ndata.m_receivedUnsolicited));
			nodeElement->LinkEndChild(newstat("nstat", "Last sent message", ndata.m_sentTS.substr(5).c_str()));
			nodeElement->LinkEndChild(newstat("nstat", "Last received message", ndata.m_receivedTS.substr(5).c_str()));
			nodeElement->LinkEndChild(newstat("nstat", "Last Request RTT", ndata.m_averageRequestRTT));
			nodeElement->LinkEndChild(newstat("nstat", "Average Request RTT", ndata.m_averageRequestRTT));
			nodeElement->LinkEndChild(newstat("nstat", "Last Response RTT", ndata.m_averageResponseRTT));
			nodeElement->LinkEndChild(newstat("nstat", "Average Response RTT", ndata.m_averageResponseRTT));
			nodeElement->LinkEndChild(newstat("nstat", "Quality", ndata.m_quality));
			while (!ndata.m_ccData.empty()) {
				Node::CommandClassData ccd = ndata.m_ccData.front();
				TiXmlElement* ccElement = new TiXmlElement("commandclass");
				snprintf(str, sizeof(str), "%d", ccd.m_commandClassId);
				ccElement->SetAttribute("id", str);
				ccElement->SetAttribute("name", cclassStr(ccd.m_commandClassId));
				ccElement->LinkEndChild(newstat("cstat", "Messages sent", ccd.m_sentCnt));
				ccElement->LinkEndChild(newstat("cstat", "Messages received", ccd.m_receivedCnt));
				nodeElement->LinkEndChild(ccElement);
				ndata.m_ccData.pop_front();
			}
			statElement->LinkEndChild(nodeElement);
			j++;
		}
		i++;
	}

	char fntemp[200];
	sprintf(fntemp, "%sozwcp.stat.XXXXXX", szUserDataFolder.c_str());
	doc.SaveFile(fntemp);

	std::string retstring = "";
	std::ifstream testFile(fntemp, std::ios::binary);
	std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
		std::istreambuf_iterator<char>());
	if (fileContents.size() > 0)
	{
		retstring.insert(retstring.begin(), fileContents.begin(), fileContents.end());
	}

	return retstring;
}

#endif
