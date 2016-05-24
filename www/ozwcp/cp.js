var pollhttp;
var scenehttp;
var topohttp;
var stathttp;
var atsthttp;
var racphttp;
var polltmr=null;
var pollwait=null;
var divcur=new Array();
var divcon=new Array();
var divinfo=new Array();
var nodes=new Array();
var nodename=new Array();
var nodeloc=new Array();
var nodegrp=new Array();
var nodegrpgrp=new Array();
var nodepoll=new Array();
var nodepollpoll=new Array();
var astate = false;
var needsave=0;
var nodecount;
var nodeid;
var sucnodeid;
var tt_top=3;
var tt_left=3;
var tt_maxw=300;
var tt_speed=10;
var tt_timer=20;
var tt_endalpha=95;
var tt_alpha=0;
var tt_h=0;
var tt=document.createElement('div');
var t=document.createElement('div');
var c=document.createElement('div');
var b=document.createElement('div');
var ie=document.all ? true : false;
var curnode=null;
var curscene=null;
var scenes=new Array();
var routes=new Array();
var curclassstat=null;
var classstats=new Array();
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  pollhttp=new XMLHttpRequest();
} else {
  pollhttp=new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  scenehttp=new XMLHttpRequest();
} else {
  scenehttp=new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  topohttp=new XMLHttpRequest();
} else {
  topohttp=new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  stathttp=new XMLHttpRequest();
} else {
  stathttp=new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  atsthttp=new XMLHttpRequest();
} else {
  atsthttp=new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  racphttp=new XMLHttpRequest();
} else {
  racphttp=new ActiveXObject("Microsoft.XMLHTTP");
}
function SaveNode(newid)
{
  var i=newid.substr(4);
  var c=-1;
  if (curnode != null)
    c=curnode.substr(4);
  document.getElementById('divconfigcur').innerHTML=divcur[i];
  document.getElementById('divconfigcon').innerHTML=divcon[i];
  document.getElementById('divconfiginfo').innerHTML=divinfo[i];
  if (c != -1) {
    if (i != c)
      document.getElementById(curnode).className='normal';
  } else {
    document.getElementById('configcur').disabled=false;
    document.getElementById('configcon').disabled=false;
    document.getElementById('configinfo').disabled=false;
  }
  curnode = newid;
  DoNodeHelp();
  UpdateSceneValues(i);
  document.getElementById(curnode).className='click';
  return true;
}
function ClearNode()
{
  if (curnode != null) {
    document.getElementById(curnode).className='normal';
    document.NodePost.nodeops.selectedIndex = 0;
    document.getElementById('divconfigcur').innerHTML='';
    document.getElementById('divconfigcon').innerHTML='';
    document.getElementById('divconfiginfo').innerHTML='';
    document.getElementById('nodeinfo').style.display = 'none';
    document.getElementById('nodecntl').style.display = 'none';
    UpdateSceneValues(-1);
    curnode = null;
  }
  return true;
}
function DisplayNode(n)
{
  return true;
}
function PollTimeout()
{
  pollhttp.abort();
  Poll();
}
function Poll()
{
  try {
    pollhttp.open("GET", 'poll.xml', true);
    pollhttp.onreadystatechange = PollReply;
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      pollhttp.send(null);
    } else {// code for IE6, IE5
      pollhttp.send();
    }
    pollwait = setTimeout(PollTimeout, 3000); //3 seconds
  } catch (e) {
    pollwait = setTimeout(PollTimeout, 3000); //3 seconds
  }
}
function PollReply()
{
  var xml;
  var elem;

  if (pollhttp.readyState == 4 && pollhttp.status == 200) {
    clearTimeout(pollwait);
    xml = pollhttp.responseXML;
    elem = xml.getElementsByTagName('poll');
    if (elem.length > 0) {
      var changed=false;
      if (elem[0].getAttribute('homeid') != document.getElementById('homeid').value)
	document.getElementById('homeid').value = elem[0].getAttribute('homeid');
      if (elem[0].getAttribute('nodecount') != nodecount) {
	nodecount = elem[0].getAttribute('nodecount');
	document.getElementById('nodecount').value = nodecount;
      }
      if (elem[0].getAttribute('nodeid') != nodeid) {
	nodeid = elem[0].getAttribute('nodeid');
      }
      if (elem[0].getAttribute('sucnodeid') != sucnodeid) {
	sucnodeid = elem[0].getAttribute('sucnodeid');
	document.getElementById('sucnodeid').value = sucnodeid;
      }
      if (elem[0].getAttribute('cmode') != document.getElementById('cmode').value)
	document.getElementById('cmode').value = elem[0].getAttribute('cmode');
      if (elem[0].getAttribute('save') != needsave) {
	needsave = elem[0].getAttribute('save');
	if (needsave == '1') {
	  document.getElementById('saveinfo').style.display = 'block';
	} else {
          document.getElementById('saveinfo').style.display = 'none';
	}
      }
      if (elem[0].getAttribute('noop') == '1') {
	var testhealreport = document.getElementById('testhealreport');
	testhealreport.innerHTML = testhealreport.innerHTML + 'No Operation message completed.<br>';  
      }	  
      elem = xml.getElementsByTagName('admin');
      if (elem[0].getAttribute('active') == 'true') {
	if (!astate) {
	  document.AdmPost.admgo.style.display = 'none';
	  document.AdmPost.admcan.style.display = 'inline';    
	  document.AdmPost.adminops.disabled = true;
	  astate = true;
	}
      } else if (elem[0].getAttribute('active') == 'false') {
	if (astate) {
	  document.AdmPost.admgo.style.display = 'inline';
	  document.AdmPost.admcan.style.display = 'none';    
	  document.AdmPost.adminops.disabled = false;
	  astate = false;
	}
      }
      if (elem[0].firstChild != null) {
	ainfo = document.getElementById('adminfo');
	ainfo.innerHTML = elem[0].firstChild.nodeValue;
	ainfo.style.display = 'block';
      }
      elem = xml.getElementsByTagName('update');
      if (elem.length > 0) {
	var remove = elem[0].getAttribute('remove');
	if (remove != undefined) {
	  var remnodes = remove.split(',');
	  changed = true;
	  for (var i = 0; i < remnodes.length; i++) {
	    nodes[remnodes[i]] = null;
	    if (curnode == ('node'+remnodes[i]))
	      ClearNode();
	  }
	}
      }
      elem = xml.getElementsByTagName('node');
      changed |= elem.length > 0;
      for (var i = 0; i < elem.length; i++) {
	var id = elem[i].getAttribute('id');
	nodes[id] = {time: elem[i].getAttribute('time'), btype: elem[i].getAttribute('btype'),
		     id: elem[i].getAttribute('id'), gtype: elem[i].getAttribute('gtype'),
		     manufacturer: elem[i].getAttribute('manufacturer'), product: elem[i].getAttribute('product'),
		     name: elem[i].getAttribute('name'), location: elem[i].getAttribute('location'),
		     listening: elem[i].getAttribute('listening') == 'true', frequent: elem[i].getAttribute('frequent') == 'true',
		     zwaveplus: elem[i].getAttribute('zwaveplus') == 'true',
		     beam: elem[i].getAttribute('beam') == 'true', routing: elem[i].getAttribute('routing') == 'true',
		     security: elem[i].getAttribute('security') == 'true', status: elem[i].getAttribute('status'),
		     values: null, groups: null};
	var k = 0;
	var values = elem[i].getElementsByTagName('value');
	nodes[id].values = new Array();
	for (var j = 0; j < values.length; j++) {
	  nodes[id].values[k] = {readonly: values[j].getAttribute('readonly') == 'true',
				 genre: values[j].getAttribute('genre'),
				 cclass: values[j].getAttribute('class'),
				 type: values[j].getAttribute('type'),
				 instance: values[j].getAttribute('instance'),
				 index: values[j].getAttribute('index'),
				 label: values[j].getAttribute('label'),
				 units: values[j].getAttribute('units'),
				 polled: values[j].getAttribute('polled') == true,
				 help: null, value: null};
	  var help = values[j].getElementsByTagName('help');
	  if (help.length > 0)
	    nodes[id].values[k].help = help[0].firstChild.nodeValue;
	  else
	    nodes[id].values[k].help = '';
	  if (nodes[id].values[k].type == 'list') {
	    var items = values[j].getElementsByTagName('item');
	    var current = values[j].getAttribute('current');
	    nodes[id].values[k].value = new Array();
	    for (var l = 0; l < items.length; l++) {
	      nodes[id].values[k].value[l] = {item: items[l].firstChild.nodeValue, selected: (current == items[l].firstChild.nodeValue)};
	    }
	  } else if (values[j].firstChild != null)
	    nodes[id].values[k].value = values[j].firstChild.nodeValue;
	  else
	    nodes[id].values[k].value = '0';
	  k++;
	}
	var groups = elem[i].getElementsByTagName('groups');
	nodes[id].groups = new Array();
	groups = groups[0].getElementsByTagName('group');
	k = 0;
	for (var j = 0; j < groups.length; j++) {
	  nodes[id].groups[k] = {id: groups[j].getAttribute('ind'),
				 max: groups[j].getAttribute('max'),
				 label: groups[j].getAttribute('label'),
				 nodes: null};
	  if (groups[j].firstChild != null)
	    nodes[id].groups[k].nodes = groups[j].firstChild.nodeValue.split(',');
	  else
	    nodes[id].groups[k].nodes = new Array();
	  k++;
	}
      }
      elem = xml.getElementsByTagName('log');
      if (elem != null && elem[0].getAttribute('size') > 0) {
	var ta = document.getElementById('logdata');
	ta.innerHTML = ta.innerHTML + return2br(elem[0].firstChild.nodeValue);
	ta.scrollTop = ta.scrollHeight;
      }
      if (changed) {
	var stuff = '';
	for (var i = 1; i < nodes.length; i++) {
	  if (nodes[i] == null)
	    continue;
	  var dt = new Date(nodes[i].time*1000);
	  var yd = new Date(dt.getDate()-1);
	  var ts;
	  var ext;
	  var exthelp;
	  if (dt < yd)
	    ts = dt.toLocaleDateString() + ' ' + dt.toLocaleTimeString();
	  else
	    ts = dt.toLocaleTimeString();
	  var val = '';
	  if (nodes[i].values.length > 0)
	    for (var j = 0; j < nodes[i].values.length; j++) {
	      if (nodes[i].values[j].genre != 'user')
		continue;
	      if (nodes[i].values[j].type == 'list') {
		for (var l = 0; l < nodes[i].values[j].value.length; l++)
		  if (!nodes[i].values[j].value[l].selected)
		    continue;
		  else
		    val = nodes[i].values[j].value[l].item;
	      } else if (nodes[i].values[j] != null) {
		val = nodes[i].values[j].value;
		if (val == 'False')
		  val = 'off';
		else if (val == 'True')
		  val = 'on';
	      }
	      break;
	    }
	  ext = ' ';
	  exthelp = '';
	  if (nodes[i].id == nodeid) {
	    ext = ext + '*';
	    exthelp = exthelp + 'controller, ';
	  }
	  if (nodes[i].listening) {
	    ext = ext + 'L';
	    exthelp = exthelp + 'listening, ';
	  }
	  if (nodes[i].frequent) {
	    ext = ext + 'F';
	    exthelp = exthelp + 'FLiRS, ';
	  }
	  if (nodes[i].beam) {
	    ext = ext + 'B';
	    exthelp = exthelp + 'beaming, ';
	  }
	  if (nodes[i].routing) {
	    ext = ext + 'R';
	    exthelp = exthelp + 'routing, ';
	  }
	  if (nodes[i].security) {
	    ext = ext + 'S';
	    exthelp = exthelp + 'security, ';
	  }
	  if (nodes[i].zwaveplus) {
	    ext = ext + "+";
	    exthelp = exthelp + 'ZwavePlus, ';
	  }
	  if (exthelp.length > 0)
	    exthelp = exthelp.substr(0, exthelp.length - 2);
	  stuff=stuff+'<tr id="node'+i+'" onmouseover="this.className=\'highlight\';" onmouseout="if (this.id == curnode) this.className=\'click\'; else this.className=\'normal\';" onclick="return SaveNode(this.id);" ondblClick="ClearNode(); return DisplayNode();"><td onmouseover="ShowToolTip(\''+exthelp+'\',0);" onmouseout="HideToolTip();">'+nodes[i].id+ext+'</td><td>'+nodes[i].btype+'</td><td>'+nodes[i].gtype+'</td><td>'+nodes[i].manufacturer+' '+nodes[i].product+'</td><td>'+nodes[i].name+'</td><td>'+nodes[i].location+'</td><td>'+val+'</td><td>'+ts+'</td><td>'+nodes[i].status+'</td></tr>';
	  CreateDivs('user', divcur, i);
	  CreateDivs('config', divcon, i);
	  CreateDivs('system', divinfo, i);
	  CreateName(nodes[i].name, i);
	  CreateLocation(nodes[i].location, i);
	  CreateGroup(i);
	  CreatePoll(i);
	}
	document.getElementById('tbody').innerHTML=stuff;
	if (curnode != null)
	  SaveNode(curnode);
      }
    }
    polltmr = setTimeout(Poll, 750);
  }
}
function BED()
{
  var forms = document.forms;
  var off = false;
  var info;

  tt.setAttribute('id','tt');
  t.setAttribute('id','tttop');
  c.setAttribute('id','ttcont');
  b.setAttribute('id','ttbot');
  tt.appendChild(t);
  tt.appendChild(c);
  tt.appendChild(b);
  document.body.appendChild(tt);
  tt.style.opacity=0;
  tt.style.filter='alpha(opacity=0)';
  tt.style.display='none';
  for (var i = 0; i < forms.length; i++) {
    if (forms[i].name == '')
      continue;
    for (var j = 0; j < forms[i].elements.length; j++) {
		if ((forms[i].elements[j].tagName == 'BUTTON') ||
			(forms[i].elements[j].tagName == 'SELECT') ||
			(forms[i].elements[j].tagName == 'INPUT'))
			forms[i].elements[j].disabled = off;
		else
			forms[i].elements[j].disabled = !off;
    }
  }
  document.getElementById('configcur').disabled = off;
  document.getElementById('configcur').checked = true;
  document.getElementById('configcon').disabled = off;
  document.getElementById('configcon').checked = false;
  document.getElementById('configinfo').disabled = off;
  document.getElementById('configinfo').checked = false;
  document.NetPost.netops.selectedIndex = 0;
  document.NetPost.netops.disabled = off;
  info = document.getElementById('netinfo');
  info.style.display = 'none';
  document.AdmPost.adminops.selectedIndex = 0;
  document.AdmPost.adminops.disabled = off;
  info = document.getElementById('adminfo');
  info.style.display = 'none';
  info = document.getElementById('admcntl');
  info.style.display = 'none';
  document.NodePost.nodeops.selectedIndex = 0;
  document.NodePost.nodeops.disabled = off;
  info = document.getElementById('nodeinfo');
  info.style.display = 'none';
  info = document.getElementById('nodecntl');
  info.style.display = 'none';
  if (off) {
    document.getElementById('homeid').value = '';
    document.getElementById('cmode').value = ''; 
    document.getElementById('nodecount').value = '';
    document.getElementById('sucnodeid').value = '';
    document.getElementById('saveinfo').style.display = 'none';
    document.getElementById('tbody').innerHTML= '';
    document.getElementById('divconfigcur').innerHTML = '';
    document.getElementById('divconfigcon').innerHTML = '';
    document.getElementById('divconfiginfo').innerHTML = '';
    document.getElementById('logdata').innerHTML= '';
  }
  if (!off) {
    Poll();
  } else {
    pollhttp.abort();
    clearTimeout(polltmr);
    clearTimeout(pollwait);
  }
  curnode=null;
}
function ShowToolTip(help,width)
{
  tt.style.display='block';
  c.innerHTML=help;
  tt.style.width=width?width+'px':'auto';
  if (!width && ie) {
    t.style.display='none';
    b.style.display='none';
    tt.style.width=tt.offsetWidth;
    t.style.display='block';
    b.style.display='block';
  }
  if (tt.offsetWidth > tt_maxw) {
      tt.style.width=tt_maxw+'px';
  }
  tt_h = parseInt(tt.offsetHeight) + tt_top;
  clearInterval(tt.timer);
  tt.timer=setInterval(function(){FadeToolTip(1);},tt_timer)
}
function PosToolTip(e)
{
  tt.style.top = ((ie ? event.clientY + document.documentElement.scrollTop : e.pageY)-tt_h)+'px';
  tt.style.left = ((ie ? event.clientX + document.documentElement.scrollLeft : e.pageX)+tt_left)+'px';
}
function FadeToolTip(d)
{
  var a = tt_alpha;
  if ((a != tt_endalpha && d == 1) || (a != 0 && d == -1)) {
    var i = tt_speed;
    if (tt_endalpha - a < tt_speed && d == 1) {
      i = tt_endalpha - a;
    } else if (tt_alpha < tt_speed && d == -1) {
      i = a;
    }
    tt_alpha = a + (i * d);
    tt.style.opacity = tt_alpha * .01;
    tt.style.filter = 'alpha(opacity='+tt_alpha+')';
  } else {
    clearInterval(tt.timer);
    if (d == -1) {
      tt.style.display='none';
    }
  }
}
function HideToolTip()
{
  clearInterval(tt.timer);
  tt.timer = setInterval(function(){FadeToolTip(-1);},tt_timer);
}
function DoConfig(id)
{
  if (curnode != null) {
    var dcur=document.getElementById('divconfigcur');
    var dcon=document.getElementById('divconfigcon');
    var dinfo=document.getElementById('divconfiginfo');
    var node=curnode.substr(4);

    dcur.innerHTML=divcur[node];
    dcon.innerHTML=divcon[node];
    dinfo.innerHTML=divinfo[node];
    if (id == 'configcur') {
      dcur.className = '';
      dcon.className = 'hide';
      dinfo.className = 'hide';
    } else if (id == 'configcon') {
      dcon.className = '';
      dcur.className = 'hide';
      dinfo.className = 'hide';
    } else {
      dinfo.className = '';
      dcur.className = 'hide';
      dcon.className = 'hide';
    }
    return true;
  } else {
    return false;
  }
}
function DoValue(id, convert)
{
  if (curnode != null) {
    var posthttp;
    var params;
    var arg=document.getElementById(id).value;

    if (typeof convert == 'undefined') {
        convert = true;
    }

    if (convert) {
	    if (arg.toLowerCase() == 'off')
	      arg = 'false';
	    else if (arg.toLowerCase() == 'on')
	      arg = 'true';
    }
    params=id+'='+arg;
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      posthttp=new XMLHttpRequest();
    } else {
      posthttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST','valuepost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);
  }
  return false;
}
function DoButton(id,pushed)
{
  if (curnode != null) {
    var posthttp;
    var params;
    var arg=document.getElementById(id).value;

    if (pushed)
      arg = 'true';
    else
      arg = 'false';
    params=id+'='+arg;
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      posthttp=new XMLHttpRequest();
    } else {
      posthttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST','buttonpost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);
  }
  return false;
}
function DoDevPost(fun)
{
    var posthttp;
    var params;

    params = 'dev=domoticz&fn='+fun+'&usb=1';
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      posthttp=new XMLHttpRequest();
    } else {
      posthttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST','devpost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);
    BED();
  return false;
}
function DoNetHelp()
{
  var ninfo = document.getElementById('netinfo');
  var scencntl = document.getElementById('scencntl');
  var topocntl = document.getElementById('topocntl');
  var topo = document.getElementById('topo');
  var statcntl = document.getElementById('statcntl');
  var statnet = document.getElementById('statnet');
  var statnode = document.getElementById('statnode');
  var statclass = document.getElementById('statclass');
  var thcntl = document.getElementById('thcntl');
  var testhealreport = document.getElementById('testhealreport');
  if (document.NetPost.netops.value == 'scen') {
    ninfo.innerHTML = 'Scene management and execution.';
    ninfo.style.display = 'block';
    scencntl.style.display = 'block';
    topocntl.style.display = 'none';
    topo.style.display = 'none';
    statcntl.style.display = 'none';
    statnet.style.display = 'none';
    statnode.style.display = 'none';
    statclass.style.display = 'none';
    thcntl.style.display = 'none';
    testhealreport.style.display = 'none';
    SceneLoad('load');
  } else if (document.NetPost.netops.value == 'topo') {
    ninfo.innerHTML = 'Topology views';
    ninfo.style.display = 'block';
    scencntl.style.display = 'none';
    topocntl.style.display = 'block';
    topo.style.display = 'block';
    statcntl.style.display = 'none';
    statnet.style.display = 'none';
    statnode.style.display = 'none';
    statclass.style.display = 'none';
    thcntl.style.display = 'none';
    testhealreport.style.display = 'none';
    curscene = null;
    TopoLoad('load');
  } else if (document.NetPost.netops.value == 'stat') {
    ninfo.innerHTML = 'Statistic views';
    ninfo.style.display = 'block';
    scencntl.style.display = 'none';
    topocntl.style.display = 'none';
    topo.style.display = 'none';
    statcntl.style.display = 'block';
    statnet.style.display = 'block';
    statnode.style.display = 'block';
    thcntl.style.display = 'none';
    testhealreport.style.display = 'none';
    curscene = null;
    StatLoad('load');
  } else if (document.NetPost.netops.value == 'test') {
    ninfo.innerHTML = 'Test & Heal Network';
    ninfo.style.display = 'block';
    scencntl.style.display = 'none';
    topocntl.style.display = 'none';
    topo.style.display = 'none';
    statcntl.style.display = 'none';
    statnet.style.display = 'none';
    statnode.style.display = 'none';
    statclass.style.display = 'none';
    thcntl.style.display = 'block';
    testhealreport.style.display = 'block';
    curscene = null;
  } else {
    ninfo.style.display = 'none';
    scencntl.style.display = 'none';
    topocntl.style.display = 'none';
    topo.style.display = 'none';
    statcntl.style.display = 'none';
    statnet.style.display = 'none';
    statnode.style.display = 'none';
    statclass.style.display = 'none';
    thcntl.style.display = 'none';
    testhealreport.style.display = 'none';
    curscene = null;
  }
  return true;
}
function DoAdmPost(can)
{
  var posthttp;
  var fun;
  var params;

  if (can) {
    fun = 'cancel';
    ainfo = document.getElementById('adminfo');
    ainfo.innerHTML = 'Cancelling controller function.';
    ainfo.style.display = 'block';
  } else {
    fun = document.AdmPost.adminops.value;
    if (fun == 'choice')
      return false;
  }
  params = 'fun='+fun;

  if (fun == 'hnf' || fun == 'remfn' || fun == 'repfn' || fun == 'reqnu' ||
      fun == 'reqnnu' || fun == 'assrr' || fun == 'delarr' || fun == 'reps' ||
      fun == 'addbtn' || fun == 'delbtn' || fun == 'refreshnode' ) {
    if (curnode == null) {
      ainfo = document.getElementById('adminfo');
      ainfo.innerHTML = 'Must select a node below for this function.';
      ainfo.style.display = 'block';
      return false;
    }
    params = params+'&node='+curnode;
  } else if (fun == 'snif') {
    if (curnode == null)
      params = params+'&node=node255';
    else
      params = params+'&node='+curnode;
  }

  if (fun == 'addbtn' || fun == 'delbtn') {
    if (document.AdmPost.button.value.length == 0) {
      ainfo = document.getElementById('adminfo');
      ainfo.innerHTML = 'Button number is required.';
      ainfo.style.display = 'block';
      document.AdmPost.button.select();
      return false;
    }
    params = params+'&button='+document.AdmPost.button.value;
  }

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','admpost.html', true);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.send(params);
  if (fun == 'remc' || fun == 'remd') {
    document.getElementById('divconfigcur').innerHTML = '';
    document.getElementById('divconfigcon').innerHTML = '';
    document.getElementById('divconfiginfo').innerHTML = '';
    curnode = null;
  }
  return false;
}
function DoAdmHelp()
{
  ainfo = document.getElementById('adminfo');
  var acntl = document.getElementById('admcntl');
  acntl.innerHTML = '';
  document.AdmPost.admgo.style.display = 'inline';
  if (document.AdmPost.adminops.value == 'addd') {
    ainfo.innerHTML = 'Add a new device or controller to the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'addds') {
    ainfo.innerHTML = 'Add a new device or controller to the Z-Wave Network (Secure Option).';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'cprim') {
    ainfo.innerHTML = 'Create new primary controller in place of dead old controller.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'rconf') {
    ainfo.innerHTML = 'Receive configuration from another controller.';   
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'remd') {
    ainfo.innerHTML = 'Remove a device or controller from the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'remfn') {
    ainfo.innerHTML = 'Remove a failed node that is on the controller\'s list of failed nodes.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'hnf') {
    ainfo.innerHTML = 'Check whether a node is in the controller\'s failed nodes list.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'repfn') {
    ainfo.innerHTML = 'Replace a failed device with a working device.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'tranpr') {
    ainfo.innerHTML = 'Transfer primary to a new controller and make current secondary.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'reqnu') {
    ainfo.style.display = 'block';
    ainfo.innerHTML = 'Update the controller with network information from the SUC/SIS.';
  } else if (document.AdmPost.adminops.value == 'reqnnu') {
    ainfo.innerHTML = 'Get a node to rebuild its neighbor list.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'assrr') {
    ainfo.innerHTML = 'Assign a network return route to a device.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'delarr') {
    ainfo.innerHTML = 'Delete all network return routes from a device.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'snif') {
    ainfo.innerHTML = 'Send a node information frame.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'reps') {
    ainfo.innerHTML = 'Send information from primary to secondary.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'addbtn' ||
	     document.AdmPost.adminops.value == 'delbtn') {
    if (curnode == null) {
      ainfo.innerHTML = 'Must select a node below for this function.';
      ainfo.style.display = 'block';
      document.AdmPost.adminops.selectedIndex = 0;
      document.AdmPost.admgo.style.display = 'none';
      return false;
    }
    acntl.innerHTML = '<label style="margin-left: 10px;"><span class="legend">Button number:&nbsp;</span></label><input type="text" class="legend" size="3" id="button" value="">';
    acntl.style.display = 'block';
    document.AdmPost.button.select();
    if (document.AdmPost.adminops.value == 'addbtn')
      ainfo.innerHTML = 'Add a button from a handheld.';
    else
      ainfo.innerHTML = 'Remove a button from a handheld.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'refreshnode') {
    ainfo.innerHTML = 'Refresh Node Info';
    ainfo.style.display = 'block';
  } else {
    ainfo.style.display = 'none';
    document.AdmPost.admgo.style.display = 'none';
  }
  return true;
}
function DoNodePost(val)
{
  var posthttp;
  var fun;
  var params;

  fun = document.NodePost.nodeops.value;
  if (fun == 'choice')
    return false;

  params = 'fun='+fun+'&node='+curnode+'&value='+val;
  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','nodepost.html', true);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.send(params);
  return false;
}
function DoNodeHelp()
{
  var ninfo = document.getElementById('nodeinfo');
  if (curnode == null) {
    ninfo.innerHTML = 'Must select a node below for this function.';
    ninfo.style.display = 'block';
    document.NodePost.nodeops.selectedIndex = 0;
    return false;
  }
  var node=curnode.substr(4);
  var ncntl = document.getElementById('nodecntl');
  if (document.NodePost.nodeops.value == 'nam') {
    ninfo.innerHTML = 'Naming functions.';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodename[node];
    ncntl.style.display = 'block';
    document.NodePost.name.select();
  } else if (document.NodePost.nodeops.value == 'loc') {
    ninfo.innerHTML = 'Location functions.';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodeloc[node];
    ncntl.style.display = 'block';
    document.NodePost.location.select();
  } else if (document.NodePost.nodeops.value == 'grp') {
    ninfo.innerHTML = 'Group/Association functions';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodegrp[node]+nodegrpgrp[node][1];
    ncntl.style.display = 'block';
  } else if (document.NodePost.nodeops.value == 'pol') {
    ninfo.innerHTML = 'Polling settings';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodepoll[node];
    ncntl.style.display = 'block';
    DoPoll();
  } else {
    ninfo.style.display = 'none';
    ncntl.style.display = 'none';
  }
  return true;
}
function DoGroup()
{
  var node=curnode.substr(4);
  var ngrp = document.getElementById('nodegrp');

  ngrp.innerHTML = nodegrpgrp[node][document.NodePost.group.value];
  return true;
}
function DoGrpPost()
{
  var posthttp;
  var params='fun=group&node='+curnode+'&num='+document.NodePost.group.value+'&groups=';
  var opts=document.NodePost.groups.options;
  var i;

  for (i = 0; i < opts.length; i++)
    if (opts[i].selected) {
      params=params+opts[i].text+',';
    }

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','grouppost.html', true);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.send(params);

  return false;
}
function DoPoll()
{
  var node=curnode.substr(4);
  var npoll = document.getElementById('nodepoll');
  var polled = document.getElementById('polled');

  if (polled != null)
    npoll.innerHTML = nodepollpoll[node][polled.value];
  return true;
}
function DoPollPost()
{
  var posthttp;
  var params='fun=poll&node='+curnode+'&ids=';
  var opts=document.NodePost.polls.options;
  var i;

  for (i = 0; i < opts.length; i++)
    params=params+opts[i].id+',';
  params=params+'&poll=';
  for (i = 0; i < opts.length; i++)
    if (opts[i].selected)
      params=params+'1,';
    else
      params=params+'0,';
	

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','pollpost.html', true);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.send(params);

  return false;
}
function DoSavePost()
{
  var posthttp;
  var params='fun=save';

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','savepost.html', true);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.send(params);

  return false;
}
function SceneLoad(fun)
{
  var params='fun='+fun;
  if (fun == 'load') {
    DisplaySceneSceneValue(null);
    var scenescenevalues = document.getElementById('scenescenevalues');
    while (scenescenevalues.options.length > 0)
      scenescenevalues.remove(0);
  }
  if (fun == 'delete') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    DisplaySceneSceneValue(null);
    params=params+'&id='+curscene;
    var slt = document.getElementById('scenelabeltext');
    slt.value = '';
  } else if (fun == 'execute') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    params=params+'&id='+curscene;
  } else if (fun == 'values') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    params=params+'&id='+curscene;
    var slt = document.getElementById('scenelabeltext');
    slt.value = scenes[curscene].label;
    DisplaySceneSceneValue(null);
  } else if (fun == 'label') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    var slt = document.getElementById('scenelabeltext');
    if (slt.value.length == 0) {
      alert('Missing label text');
      return false;
    }
    params=params+'&id='+curscene+'&label='+slt.value;
    slt.value = '';
  } else if (fun == 'addvalue') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    if (curnode == null) {
      alert('Node not selected');
      return false;
    }
    var values = document.getElementById('scenevalues');
    if (values.options.selectedIndex == -1) {
      alert('Value not selected');
      return false;
    }
    var vals = values.options[values.options.selectedIndex].value.split('-');
    if (vals[3] != 'list' && vals[3] != 'bool') {
      var value = document.getElementById('valuetext');
      if (value.value.length == 0) {
	alert('Data not entered');
	return false;
      }
      params=params+'&id='+curscene+'&vid='+vals[0]+'-'+vals[1]+'-'+vals[2]+'-'+vals[3]+'-'+vals[4]+'-'+vals[5]+'&value='+value.value;
    } else {
      var value = document.getElementById('valueselect');
      params=params+'&id='+curscene+'&vid='+vals[0]+'-'+vals[1]+'-'+vals[2]+'-'+vals[3]+'-'+vals[4]+'-'+vals[5]+'&value='+value.options[value.selectedIndex].value;
    }
    DisplaySceneSceneValue(null);
  } else if (fun == 'update') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    if (curnode == null) {
      alert('Node not selected');
      return false;
    }
    var values = document.getElementById('scenescenevalues');
    if (values.options.selectedIndex == -1) {
      alert('Value not selected');
      return false;
    }
    var vals = values.options[values.options.selectedIndex].value.split('-');
    if (vals[3] != 'list' && vals[3] != 'bool') {
      var value = document.getElementById('scenevaluetext');
      if (value.value.length == 0) {
	alert('Data not entered');
	return false;
      }
      params=params+'&id='+curscene+'&vid='+vals[0]+'-'+vals[1]+'-'+vals[2]+'-'+vals[3]+'-'+vals[4]+'-'+vals[5]+'&value='+value.value;
    } else {
      var value = document.getElementById('valueselect');
      params=params+'&id='+curscene+'&vid='+vals[0]+'-'+vals[1]+'-'+vals[2]+'-'+vals[3]+'-'+vals[4]+'-'+vals[5]+'&value='+value.options[value.selectedIndex].value;
    }
    DisplaySceneSceneValue(null);
  } else if (fun == 'remove') {
    if (curscene == null) {
      alert("Scene not selected");
      return false;
    }
    var values = document.getElementById('scenescenevalues');
    if (values.options.selectedIndex == -1) {
      alert('Scene value not selected');
      return false;
    }
    var vals = values.options[values.options.selectedIndex].value.split('-');
    params=params+'&id='+curscene+'&vid='+vals[0]+'-'+vals[1]+'-'+vals[2]+'-'+vals[3]+'-'+vals[4]+'-'+vals[5];
    DisplaySceneSceneValue(null);
  }
  scenehttp.open('POST','scenepost.html', true);
  scenehttp.onreadystatechange = SceneReply;
  scenehttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  scenehttp.send(params);

  return false;
}
function SceneReply()
{
  var xml;
  var elem;

  if (scenehttp.readyState == 4 && scenehttp.status == 200) {
    xml = scenehttp.responseXML;
    elem = xml.getElementsByTagName('scenes');
    if (elem.length > 0) {
      var i;
      var sceneids = document.getElementById('sceneids');
      var scenevalues = document.getElementById('scenevalues');
      var scenescenevalues = document.getElementById('scenescenevalues');
      var id, home, node, label, units, genre, type, cclass, instance, index;
      i = elem[0].getAttribute('sceneid');
      if (i != null) {
	scenes = new Array();
	while (sceneids.options.length > 0)
	  sceneids.remove(0);
      }
      i = elem[0].getAttribute('scenevalue');
      if (i != null) {
	scenes[curscene].values = new Array();
	while (scenescenevalues.options.length > 0)
	  scenescenevalues.remove(0);
      }
      for (i = 0; i < elem[0].childNodes.length; i++) {
	if (elem[0].childNodes[i].nodeType != 1)
	  continue;
	if (elem[0].childNodes[i].tagName == 'sceneid') {
	  id = elem[0].childNodes[i].getAttribute('id');
	  label = elem[0].childNodes[i].getAttribute('label');
	  scenes[id] = {label: label, values: new Array()};
	  sceneids.add(new Option(id, id));
	} else if (elem[0].childNodes[i].tagName == 'scenevalue') {
	  var value;
	  id = elem[0].childNodes[i].getAttribute('id');
	  home = elem[0].childNodes[i].getAttribute('home');
	  node = elem[0].childNodes[i].getAttribute('node');
	  label = elem[0].childNodes[i].getAttribute('label');
	  units = elem[0].childNodes[i].getAttribute('units');
	  genre = elem[0].childNodes[i].getAttribute('genre');
	  type = elem[0].childNodes[i].getAttribute('type');
	  cclass = elem[0].childNodes[i].getAttribute('class');
	  instance = elem[0].childNodes[i].getAttribute('instance');
	  index = elem[0].childNodes[i].getAttribute('index');
	  value = elem[0].childNodes[i].firstChild.nodeValue;
	  var val = node+','+label;
	  var vid = node+'-'+cclass+'-'+genre+'-'+type+'-'+instance+'-'+index;
	  scenescenevalues.add(new Option(val,vid));
	  scenes[id].values.push({home: home, node: node, label: label, units: units, type: type, cclass: cclass, genre: genre, instance: instance, index: index, value: value});
	}
      }
    }
  }
}
function UpdateSceneValues(c)
{
  var sv = document.getElementById('scenevalues');
  while (sv.options.length > 0)
    sv.remove(0);
  if (c == -1)
    return;
  for (var i = 0; i < nodes[c].values.length; i++) {
    if (nodes[c].values[i].genre != 'user')
      continue;
    if (nodes[c].values[i].readonly)
      continue;
    if (nodes[c].values[i].type == 'button')
      continue;
    var vid=nodes[c].id+'-'+nodes[c].values[i].cclass+'-user-'+nodes[c].values[i].type+'-'+nodes[c].values[i].instance+'-'+nodes[c].values[i].index;
    sv.add(new Option(nodes[c].values[i].label,vid));
  }
  DisplaySceneValue(null);
}
function DisplaySceneValue(opt)
{
  var vt = document.getElementById('valuetext');
  var vs = document.getElementById('valueselect');
  var vu = document.getElementById('valueunits');
  if (opt == null) {
    vt.style.display = 'inline';
    vs.style.display = 'none';
    vt.value = '';
    while(vs.options.length > 0)
      vs.remove(0);
    vu.innerHTML = '';
    return false;
  }
  var vals = opt.value.split('-');
  var j;
  for (j = 0; j < nodes[vals[0]].values.length; j++)
    if (nodes[vals[0]].values[j].cclass == vals[1] &&
	nodes[vals[0]].values[j].genre == 'user' &&
	nodes[vals[0]].values[j].type == vals[3] &&
	nodes[vals[0]].values[j].instance == vals[4] &&
	nodes[vals[0]].values[j].index == vals[5])
      break;
  if (vals[3] == 'list') {
    vt.style.display = 'none';
    vs.style.display = 'inline';
  } else if (vals[3] == 'bool') {
    if (nodes[vals[0]].values[j].value == 'True') {
      vs.add(new Option('On','true',true));
      vs.add(new Option('Off','false'));
    } else {
      vs.add(new Option('Off','false',true));
      vs.add(new Option('On','true'));
    }
    vt.style.display = 'none';
    vs.style.display = 'inline';
  } else {
    vt.value = nodes[vals[0]].values[j].value;
    vt.style.display = 'inline';
    vs.style.display = 'none';
  }
  vu.innerHTML = nodes[vals[0]].values[j].units;
  return false;
}
function DisplaySceneSceneValue(opt)
{
  var vt = document.getElementById('scenevaluetext');
  var vs = document.getElementById('scenevalueselect');
  var vu = document.getElementById('scenevalueunits');
  if (opt == null) {
    vt.style.display = 'inline';
    vs.style.display = 'none';
    vt.value = '';
    while(vs.options.length > 0)
      vs.remove(0);
    vu.innerHTML = '';
    return false;
  }
  var vals = opt.value.split('-');
  var j;
  for (j = 0; j < scenes[curscene].values.length; j++) {
    if (scenes[curscene].values[j].cclass == vals[1] &&
	scenes[curscene].values[j].genre == 'user' &&
	scenes[curscene].values[j].type == vals[3] &&
	scenes[curscene].values[j].instance == vals[4] &&
	scenes[curscene].values[j].index == vals[5])
      break;
  }
  if (vals[3] == 'list') {
    vt.style.display = 'none';
    vs.style.display = 'inline';
  } else if (vals[3] == 'bool') {
    if (scenes[curscene].values[j].value == 'True') {
      vs.add(new Option('On','on',true));
      vs.add(new Option('Off','off'));
    } else {
      vs.add(new Option('Off','off',true));
      vs.add(new Option('On','on'));
    }
    vt.style.display = 'none';
    vs.style.display = 'inline';
  } else {
    vt.value = scenes[curscene].values[j].value;
    vt.style.display = 'inline';
    vs.style.display = 'none';
  }
  vu.innerHTML = scenes[curscene].values[j].units;
  return false;
}
function TopoLoad(fun)
{
  var params='fun='+fun;
  topohttp.open('POST','topopost.html', true);
  topohttp.onreadystatechange = TopoReply;
  topohttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  topohttp.send(params);

  return false;
}
function TopoReply()
{
  var xml;
  var elem;

  if (topohttp.readyState == 4 && topohttp.status == 200) {
    xml = topohttp.responseXML;
    elem = xml.getElementsByTagName('topo');
    if (elem.length > 0) {
      var i;
      var id;
      var list;
      for (i = 0; i < elem[0].childNodes.length; i++) {
	if (elem[0].childNodes[i].nodeType != 1)
	  continue;
	if (elem[0].childNodes[i].tagName == 'node') {
	  id = elem[0].childNodes[i].getAttribute('id');
	  list = elem[0].childNodes[i].firstChild.nodeValue;
	  routes[id] = list.split(',');
	}
      }
      var stuff = '<tr><th>Nodes</th>';
      var topohead = document.getElementById('topohead');
      for (i = 1; i < nodes.length; i++) {
	if (nodes[i] == null)
	  continue;
	stuff=stuff+'<th>'+i+'</th>';
      }
      stuff=stuff+'</tr>'
      topohead.innerHTML = stuff;
      stuff = '';
      for (i = 1; i < nodes.length; i++) {
	if (nodes[i] == null)
	  continue;
	stuff=stuff+'<tr><td style="vertical-align: top; text-decoration: underline; background-color: #FFFFFF;">'+i+'</td>';
	var j, k = 0;
	for (j = 1; j < nodes.length; j++) {
	  if (nodes[j] == null)
	    continue;
	  if (routes[i] != undefined && k < routes[i].length && j == routes[i][k]) {
	    stuff=stuff+'<td>*</td>';
	    k++;
	  } else {
	    stuff=stuff+'<td>&nbsp;</td>';
	  }
	}
	stuff=stuff+'</tr>';
      }
      var topobody = document.getElementById('topobody');
      topobody.innerHTML = stuff;
    }
  }
}
function DisplayStatClass(t,n)
{
  var scb = document.getElementById('statclassbody');
  var sn = document.getElementById('statnode');
  if (curclassstat != null) {
    var lastn = curclassstat.id.substr(8);
    if (n != lastn) {
      curclassstat.className = 'normal';
    }
  }
  if (curclassstat == null || t != curclassstat) {
    curclassstat = t;
    t.className = 'highlight';
    sn.style.width = '72%';
    scb.innerHTML = classstats[n];
    document.getElementById('statclass').style.display = 'block';
  } else {
    curclassstat = null;
    t.className = 'normal';
    sn.style.width = '100%';
    scb.innerHTML = '';
    document.getElementById('statclass').style.display = 'none';
  }
  return true;
}
function StatLoad(fun)
{
  var params='fun='+fun;
  stathttp.open('POST','statpost.html', true);
  stathttp.onreadystatechange = StatReply;
  stathttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  stathttp.send(params);

  return false;
}
function StatReply()
{
  var xml;
  var elem;

  if (stathttp.readyState == 4 && stathttp.status == 200) {
    xml = stathttp.responseXML;
    elem = xml.getElementsByTagName('stats');
    if (elem.length > 0) {
      var errors = xml.getElementsByTagName('errors');
      var counts = xml.getElementsByTagName('counts');
      var info = xml.getElementsByTagName('info');
      var cnt = errors[0].childNodes.length;
      if (counts[0].childNodes.length > cnt)
	cnt = counts[0].childNodes.length;
      if (info[0].childNodes.length > cnt)
	cnt = info[0].childNodes.length;
      var stuff = '';
      var i;
      for (i = 0; i < cnt; i++) {
	if (i < errors[0].childNodes.length)
	  if (errors[0].childNodes[i].nodeType != 1)
	    continue;
	if (i < counts[0].childNodes.length)
	  if (counts[0].childNodes[i].nodeType != 1)
	    continue;
	if (i < info[0].childNodes.length)
	  if (info[0].childNodes[i].nodeType != 1)
	    continue;
	stuff = stuff + '<tr>';
	if (i < errors[0].childNodes.length)
	  stuff = stuff + '<td style="text-align: right;">'+errors[0].childNodes[i].getAttribute('label')+': </td><td style="text-align: left;">'+errors[0].childNodes[i].firstChild.nodeValue+'</td>';
	else
	  stuff = stuff + '<td>&nbsp;</td><td>&nbsp;</td>';
	if (i < counts[0].childNodes.length)
	  stuff = stuff + '<td style="text-align: right;">'+counts[0].childNodes[i].getAttribute('label')+': </td><td style="text-align: left;">'+counts[0].childNodes[i].firstChild.nodeValue+'</td>';
	else
	  stuff = stuff + '<td>&nbsp;</td><td>&nbsp;</td>';
	if (i < info[0].childNodes.length)
	  stuff = stuff + '<td style="text-align: right;">'+info[0].childNodes[i].getAttribute('label')+': </td><td style="text-align: left;">'+info[0].childNodes[i].firstChild.nodeValue+'</td>';
	else
	  stuff = stuff + '<td>&nbsp;</td><td>&nbsp;</td>';
	stuff = stuff + '</tr>';
      }
      var statnetbody = document.getElementById('statnetbody');
      statnetbody.innerHTML = stuff;
      var node = xml.getElementsByTagName('node');
      var stuff = '';
      var oldnode = null;
      if (curclassstat != null)
	oldnode = curclassstat.id;
      for (var i = 0; i < node.length; i++) {
	stuff = stuff+'<tr id="statnode'+i+'" onclick="return DisplayStatClass(this,'+i+');"><td>'+node[i].getAttribute('id')+'</td>';
	var nstat = node[i].getElementsByTagName('nstat');
	for (var j = 0; j < nstat.length; j++) {
	    stuff = stuff+'<td>'+nstat[j].firstChild.nodeValue+'</td>';
	}
	stuff = stuff+'</tr>';
	var cstuff = '';
	var cclass = node[i].getElementsByTagName('commandclass');
	for (var j = 0; j < cclass.length; j++) {
	  cstuff = cstuff+'<tr><td>'+cclass[j].getAttribute('name')+'</td>';
	  var cstat = cclass[j].getElementsByTagName('cstat');
	  for (var k = 0; k < cstat.length; k++) {
	    cstuff = cstuff+'<td>'+cstat[k].firstChild.nodeValue+'</td>';
	  }
	  cstuff = cstuff+'</tr>';
	}
	classstats[i] = cstuff;
      }
      var statnodebody = document.getElementById('statnodebody');
      statnodebody.innerHTML = stuff;
    }
    if (oldnode != null) {
      var scb = document.getElementById('statclassbody');
      scb.innerHTML = classstats[oldnode.substr(8)];
      curclassstat = document.getElementById(oldnode);
      curclassstat.className = 'highlight';
    }
  }
}
function TestHealLoad(fun)
{
  var params='fun='+fun;
  if (fun == 'test') {
    var cnt = document.getElementById('testnode');
    if (cnt.value.length == 0) {
      params = params+'&num=0';
    } else {
      params = params+'&num='+cnt.value;
    }
    var cnt = document.getElementById('testmcnt');
    if (cnt.value.length == 0) {
      alert('Missing count value');
      return false;
    }
    params = params+'&cnt='+cnt.value;
  } else if (fun == 'heal') {
    var cnt = document.getElementById('healnode');
    if (cnt.value.length == 0) {
      params = params+'&num=0';
    } else {
      params = params+'&num='+cnt.value;
    }
    var check = document.getElementById('healrrs');
    if (check.checked)
      params = params+'&healrrs=1';
  }
  atsthttp.open('POST','thpost.html', true);
  atsthttp.onreadystatechange = TestHealReply;
  atsthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  atsthttp.send(params);

  return false;
}
function TestHealReply()
{
  var xml;
  var elem;

  if (atsthttp.readyState == 4 && atsthttp.status == 200) {
    xml = atsthttp.responseXML;
    var threport = document.getElementById('testhealreport');
    elem = xml.getElementsByTagName('test');
    if (elem.length > 0) {
      threport.innerHTML = '';
    }
    elem = xml.getElementsByTagName('heal');
    if (elem.length > 0) {
      threport.innerHTML = '';
    }
  }
}
function RequestAllConfig(n)
{
  var params='fun=racp&node='+n;
  racphttp.open('POST','confparmpost.html', true);
  racphttp.onreadystatechange = PollReply;
  racphttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  racphttp.send(params);

  return false;
}
function RequestAll(n)
{
  var params='fun=racp&node='+n;
  racphttp.open('POST','refreshpost.html', true);
  racphttp.onreadystatechange = PollReply;
  racphttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  racphttp.send(params);

  return false;
}
function quotestring(s)
{
  return s.replace(/\'/g, "");
}
function return2br(dataStr)
{
  return dataStr.replace(/(\r\n|[\r\n])/g, "<br />");
}
function boxsize(field)
{
  if (field.length < 8)
    return 8;
  return field.length+2;
}
function CreateOnOff(i,j,vid)
{
  var data='<tr><td style="float: right;"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  data=data+'><label><span class="legend">'+nodes[i].values[j].label+':&nbsp;</span></label></td><td><select id="'+vid+'" onchange="return DoValue(\''+vid+'\');"'
  if (nodes[i].values[j].readonly)
    data=data+' disabled="true"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  data=data+'>';
  if (nodes[i].values[j].value == 'True')
    data=data+'<option value="off">Off</option><option value="on" selected="true">On</option>';
  else
    data=data+'<option value="off" selected="true">Off</option><option value="on">On</option>';
  data=data+'</select></td><td><span class="legend">'+nodes[i].values[j].units+'</span></td></tr>';
  return data;
}
function CreateTextBox(i,j,vid)
{
  var data = '<tr><td style="float: right;"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  var value=nodes[i].values[j].value.replace(/(\n\s*$)/, "");
  data=data+'><label><span class="legend">'+nodes[i].values[j].label+':&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(value)+'" id="'+vid+'" value="'+value+'"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  if (nodes[i].values[j].readonly)
    data=data+' disabled="true">';
  else
    data=data+'>';
  data=data+'<span class="legend">'+nodes[i].values[j].units+'</span>';
  if (!nodes[i].values[j].readonly)
    data=data+'<button type="submit" onclick="return DoValue(\''+vid+'\');">Submit</button>';
  data=data+'</td></tr>';
  return data;
}
function CreateList(i,j,vid)
{
  var data='<tr><td style="float: right;"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  data=data+'><label><span class="legend">'+nodes[i].values[j].label+':&nbsp;</span></label></td><td><select id="'+vid+'" onchange="return DoValue(\''+vid+'\', false);"';
  if (nodes[i].values[j].help.length > 0)
    data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  if (nodes[i].values[j].readonly)
    data=data+' disabled="true">';
  else
    data=data+'>';
  if (nodes[i].values[j].value != null)
    for (k=0; k<nodes[i].values[j].value.length; k++) {
      data=data+'<option value="'+nodes[i].values[j].value[k].item+'"';
      if (nodes[i].values[j].value[k].selected)
	data=data+' selected="true"';
      data=data+'>'+nodes[i].values[j].value[k].item+'</option>';
    }
  data=data+'</select><span class="legend">'+nodes[i].values[j].units+'</span></td></tr>';
  return data;
}
function CreateLabel(i,j,vid)
{
    return '<tr><td style="float: right;"><label><span class="legend">'+nodes[i].values[j].label+':&nbsp;</span></label></td><td><input type="text" class="legend" disabled="true" size="'+boxsize(nodes[i].values[j].value)+'" id="'+vid+'" value="'+nodes[i].values[j].value+'"><span class="legend">'+nodes[i].values[j].units+'</span></td></tr>';
}
function CreateButton(i,j,vid)
{
  var data='<tr><td style="float: right;"';
  if (nodes[i].values[j].help.length > 0)
      data=data+' onmouseover="ShowToolTip(\''+quotestring(nodes[i].values[j].help)+'\',0);" onmouseout="HideToolTip();"';
  data=data+'><label><span class="legend">'+nodes[i].values[j].label+':&nbsp;</span></label></td><td><button type="submit" id="'+vid+'" onclick="return false;" onmousedown="return DoButton(\''+vid+'\',true);" onmouseup="return DoButton(\''+vid+'\',false);"'
  if (nodes[i].values[j].readonly)
    data=data+' disabled="true"';
  data=data+'>Submit</button></td><td><span class="legend">'+nodes[i].values[j].units+'</span></td></tr>';
  return data;
}
function CreateDivs(genre,divto,ind)
{
  divto[ind]='<table border="0" cellpadding="1" cellspacing="0"><tbody>';
  if (nodes[ind].values != null) {
      var j = 0;
    for (var i = 0; i < nodes[ind].values.length; i++) {
      var match;
      if (genre == 'user')
	match = (nodes[ind].values[i].genre == genre || nodes[ind].values[i].genre == 'basic');
      else
	match = (nodes[ind].values[i].genre == genre);
      if (!match)
	continue;
      var vid=nodes[ind].id+'-'+nodes[ind].values[i].cclass+'-'+nodes[ind].values[i].genre+'-'+nodes[ind].values[i].type+'-'+nodes[ind].values[i].instance+'-'+nodes[ind].values[i].index;
      j++;
      if (nodes[ind].values[i].type == 'bool') {
	divto[ind]=divto[ind]+CreateOnOff(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'byte') {
	divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'int') {
	divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'short') {
	divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'decimal') {
	divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'list') {
	divto[ind]=divto[ind]+CreateList(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'string') {
	divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'button') {
	divto[ind]=divto[ind]+CreateButton(ind,i,vid);
      } else if (nodes[ind].values[i].type == 'raw') {
        divto[ind]=divto[ind]+CreateTextBox(ind,i,vid);
      }
    }
    if (j != 0) {
      if (genre == 'config')
        divto[ind]=divto[ind]+'<tr><td>&nbsp;</td><td><button type="submit" id="requestallconfig" name="requestallconfig" onclick="return RequestAllConfig('+ind+');">Refresh</button></td><td>&nbsp;</td></tr>';
      else
        divto[ind]=divto[ind]+'<tr><td>&nbsp;</td><td><button type="submit" id="requestall" name="requestall" onclick="return RequestAll('+ind+');">Refresh</button></td><td>&nbsp;</td></tr>';
    }
  }
  divto[ind]=divto[ind]+'</tbody></table>';
}
function CreateName(val,ind)
{
  nodename[ind]='<tr><td style="float: right;"><label><span class="legend">Name:&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(val)+'" id="name" value="'+val+'"><button type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.name.value);">Submit</button></td></tr>';
}
function CreateLocation(val,ind)
{
  nodeloc[ind]='<tr><td style="float: right;"><label><span class="legend">Location:&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(val)+'" id="location" value="'+val+'"><button type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.location.value);">Submit</button></td></tr>';
}
function CreateGroup(ind)
{
  var grp;
  var i, j, k;

  if (nodes[ind].groups.length == 0) {
    nodegrp[ind]='';
    nodegrpgrp[ind] = new Array();
    nodegrpgrp[ind][1]='';
    return;
  }
  nodegrp[ind]='<tr><td style="float: right;"><label><span class="legend">Groups:&nbsp;</span></label></td><td><select id="group" style="margin-left: 5px;" onchange="return DoGroup();">';
  nodegrpgrp[ind] = new Array(nodes[ind].groups.length);
  grp = 1;
  for (i = 0; i < nodes[ind].groups.length; i++) {
    nodegrp[ind]=nodegrp[ind]+'<option value="'+nodes[ind].groups[i].id+'">'+nodes[ind].groups[i].label+' ('+nodes[ind].groups[i].id+')</option>';
    nodegrpgrp[ind][grp] = '<td><div id="nodegrp" name="nodegrp" style="float: right;"><select id="groups" multiple size="8" style="vertical-align: top; margin-left: 5px;">';
    k = 0;
    for (j = 1; j < nodes.length; j++) {
      if (nodes[j] == null)
	continue;

	  // build a list of instances 
      var instances = [ String(j) ];
      for (var l = 0; l < nodes[j].values.length; l++) {
	  	instances[ l + 1] =  j + '.' + nodes[j].values[l].instance;
	  	instances.push( j + '.' + nodes[j].values[l].instance);
      }	
	
		// make unique
	  instances = instances.filter(function(item, i, ar){ return ar.indexOf(item) === i; });

	  // only show when we have found multiple instances
	  if ( instances.length <= 2 ) {
		instances = [ String(j) ];
	  }

      if (nodes[ind].groups[i].nodes != null)
	while (k < nodes[ind].groups[i].nodes.length && nodes[ind].groups[i].nodes[k] < j)
	  k++;
	  
	  for (var l=0; l< instances.length; l++) { 
	      if (nodes[ind].groups[i].nodes.indexOf(instances[l]) != -1 )
			nodegrpgrp[ind][grp]=nodegrpgrp[ind][grp]+'<option selected="true">'+instances[l]+'</option>';
      else
			nodegrpgrp[ind][grp]=nodegrpgrp[ind][grp]+'<option>'+instances[l]+'</option>';
	  }			
    }
    nodegrpgrp[ind][grp]=nodegrpgrp[ind][grp]+'</select></td><td><button type="submit" style="margin-left: 5px;" onclick="return DoGrpPost();">Submit</button></div></td></tr>';
    grp++;
  }
  nodegrp[ind]=nodegrp[ind]+'</select></td>';
}
function CreatePoll(ind)
{
  var uc = 0;
  var sc = 0;
  if (nodes[ind].values != null)
    for (var i = 0; i < nodes[ind].values.length; i++) {
      if (nodes[ind].values[i].genre == 'user')
	uc++;
      if (nodes[ind].values[i].genre == 'system')
	sc++;
    }
  if (uc > 0 || sc > 0)
    nodepoll[ind]='<tr><td style="float: right;"><label><span class="legend">Polling&nbsp;</span></label></td><td><select id="polled" style="margin-left: 5px;" onchange="return DoPoll();"><option value="0">User</option><option value="1">System</option></select></td><td><div id="nodepoll" name="nodepoll" style="float: left;"></div></td><td><button type="submit" style="margin-left: 5px; vertical-align: top;" onclick="return DoPollPost();">Submit</button></td></tr>';
  else
    nodepoll[ind]='';
  nodepollpoll[ind] = new Array(2);
  CreatePollPoll('user', ind, uc);
  CreatePollPoll('system', ind, sc);
}
function CreatePollPoll(genre,ind,cnt)
{
  var ind1;
  if (genre == 'user')
    ind1 = 0;
  else
    ind1 = 1;
  nodepollpoll[ind][ind1]='<select id="polls" multiple size="4" style="vertical-align: top; margin-left: 5px;">';
  if (cnt > 0) {
    for (var i = 0; i < nodes[ind].values.length; i++) {
      if (nodes[ind].values[i].genre != genre)
	continue;
      if (nodes[ind].values[i].type == 'button')
	continue;
      var vid=nodes[ind].id+'-'+nodes[ind].values[i].cclass+'-'+genre+'-'+nodes[ind].values[i].type+'-'+nodes[ind].values[i].instance+'-'+nodes[ind].values[i].index;
      nodepollpoll[ind][ind1]=nodepollpoll[ind][ind1]+'<option id="'+vid+'"';
      if (nodes[ind].values[i].polled)
	  nodepollpoll[ind][ind1]=nodepollpoll[ind][ind1]+' selected="true"';
      nodepollpoll[ind][ind1]=nodepollpoll[ind][ind1]+'>'+nodes[ind].values[i].label+'</option>';
    }
    nodepollpoll[ind][ind1]=nodepollpoll[ind][ind1]+'</select>';
  } else
    nodepollpoll[ind][ind1]='';
}
