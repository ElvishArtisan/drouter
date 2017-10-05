// server_sa.cpp
//
// Software Authority Protocol Server for drouterd(8)
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <stdio.h>
#include <syslog.h>

#include <QStringList>

#include "server_sa.h"

ServerSa::ServerSa(int sock,QObject *parent)
  : ServerNet(sock,SERVER_SA_PORT,parent)
{
  LoadHelp();
}


ServerSa::~ServerSa()
{
}


void ServerSa::processCommand(int id,const SyAString &cmd)
{
  //  printf("processCommand(%d,%s)\n",id,(const char *)cmd.toUtf8());

  unsigned cardnum=0;
  unsigned input=0;
  unsigned output=0;
  unsigned msecs=0;
  bool ok=false;
  QStringList cmds=cmd.split(" ");

  if(cmds[0].toLower()=="login") {  // FIXME: We should check the password here!
    send("Login Successful\r\n",id);
    send(">>",id);
  }

  if((cmds[0].toLower()=="exit")||(cmds[0].toLower()=="quit")) {
    closeConnection(id);
  }

  if((cmds[0].toLower()=="help")||(cmds[0]=="?")) {
    if(cmds.size()==1) {
      send(sa_help_strings[""]+"\r\n\r\n",id);
    }
    else {
      if(sa_help_strings[cmds[1].toLower()]==NULL) {
	send("\r\n\r\n",id);
      }
      else {
	send(sa_help_strings[cmds[1].toLower()]+"\r\n\r\n",id);
      }
    }
    send(">>",id);
  }

  if(cmds[0].toLower()=="routernames") {
    emit sendMatrixNames(id);
  }

  if((cmds[0].toLower()=="gpistat")&&(cmds.size()>=2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	emit sendGpiInfo(id,cardnum-1,-1);
      }
      else {
	input=cmds[2].toUInt(&ok);
	if(ok) {
	  emit sendGpiInfo(id,cardnum-1,input);
	}
      }
    }
    else {
      send("Error - Router Does Not exist.\r\n",id);
      send(">>",id);
    }
  }

  if((cmds[0].toLower()=="gpostat")&&(cmds.size()>=2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	emit sendGpoInfo(id,cardnum-1,-1);
      }
      else {
	input=cmds[2].toUInt(&ok);
	if(ok) {
	  emit sendGpoInfo(id,cardnum-1,input);
	}
      }
    }
    else {
      send("Error - Router Does Not exist.\r\n",id);
      send(">>",id);
    }
  }

  if((cmds[0].toLower()=="sourcenames")&&(cmds.size()==2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      emit sendInputNames(id,cardnum-1);
    }
    else {
      send("Error - Bay Does Not exist.\r\n",id);
      send(">>",id);
    }
  }

  if((cmds[0].toLower()=="destnames")&&(cmds.size()==2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      emit sendOutputNames(id,cardnum-1);
    }
    else {
      send("Error - Bay Does Not exist.\r\n",id);
      send(">>",id);
    }
  }

  if(cmds[0].toLower()=="activateroute") {
    if(cmds.size()==4) {
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	output=cmds[2].toUInt(&ok);
	if(ok) {
	  input=cmds[3].toUInt(&ok);
	  if(ok) {
	    emit setRoute(id,cardnum-1,input,output-1);
	  }
	  else {
	    send("Error\r\n",id);
	    send(">>",id);
	  }
	}
	else {
	  send("Error\r\n",id);
	  send(">>",id);
	}
      }
      else {
	send("Error\r\n",id);
	send(">>",id);
      }
    }
    else {
      send("Error\r\n",id);
      send(">>",id);
    }
  }

  if(cmds[0].toLower()=="routestat") {
    if(cmds.size()>=2) {
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	if(cmds.size()>=3) {
	  output=cmds[2].toUInt(&ok);
	  if(!ok) {
	    send("Error\r\n",id);
	    send(">>",id);
	    return;
	  }
	}
      }
      else {
	send("Error\r\n",id);
	send(">>",id);
	return;
      }
      emit sendRouteInfo(id,cardnum-1,output);
    }
    else {
      send("Error\r\n",id);
      send(">>",id);
    }
  }

  if(cmds[0].toLower()=="triggergpi") {
    if((cmds.size()==4)||(cmds.size()==5)) {
      if(cmds.size()==5) {
	msecs=cmds[4].toUInt();
      }
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	input=cmds[2].toUInt(&ok);
	if(cmds[3].length()==5) {
	  emit setGpiState(id,cardnum-1,input-1,msecs,cmds[3]);
	}
      }
    }
  }

  if(cmds[0].toLower()=="triggergpo") {
    if((cmds.size()==4)||(cmds.size()==5)) {
      if(cmds.size()==5) {
	msecs=cmds[4].toUInt();
      }
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	output=cmds[2].toUInt(&ok);
	if(cmds[3].length()==5) {
	  emit setGpoState(id,cardnum-1,output-1,msecs,cmds[3]);
	}
      }
    }
  }
}


void ServerSa::LoadHelp()
{
  sa_help_strings[""]=QString("ActivateRoute")+
    ", DestNames"+
    ", Exit"+
    ", GPIStat"+
    ", GPOStat"+
    ", Quit"+
    ", RouterNames"+
    ", RouteStat"+
    ", SourceNames"+
    ", TriggerGPI"+
    ", TriggerGPO"+
    "\r\n\r\nEnter \"Help\" or \"?\" followed by the name of the command.";
  sa_help_strings["activateroute"]="ActivateRoute <router> <output> <input>\r\n\r\nRoute <input> to <output> on <router>.";
  sa_help_strings["destnames"]="DestNames <router>\r\n\r\nReturn names of all outputs on the specified router.";
  sa_help_strings["exit"]="Exit\r\n\r\nClose TCP/IP connection.";
  sa_help_strings["gpistat"]="GPIStat <router> [<gpi-num>]\r\n\r\nQuery the state of one or more GPIs.\r\nIf <gpi-num> is not given, the entire set of GPIs for the specified\r\n<router> will be returned.";
  sa_help_strings["gpostat"]="GPOStat <router> [<gpo-num>]\r\n\r\nQuery the state of one or more GPOs.\r\nIf <gpo-num> is not given, the entire set of GPOs for the specified\r\n<router> will be returned.";
  sa_help_strings["quit"]="Quit\r\n\r\nClose TCP/IP connection.";
  sa_help_strings["routernames"]="RouterNames\r\n\r\nReturn a list of configured matrices.";
  sa_help_strings["routestat"]="RouteStat <router> [<output>]\r\n\r\nReturn the <output> crosspoint's input assignment.\r\nIf not <output> is given, the crosspoint states for all outputs on\r\n<router> will be returned.";
  sa_help_strings["sourcenames"]="SourceNames <router>\r\n\r\nReturn names of all inputs on the specified router.";
  sa_help_strings["triggergpi"]="TriggerGPI <router> <gpi-num> <state> [<duration>]\r\n\r\nSet the specified GPI to <state> for <duration> milliseconds.\r\n(Supported only by virtual GPI devices.)";
  sa_help_strings["triggergpo"]="TriggerGPO <router> <gpo-num> <state> [<duration>]\r\n\r\nSet the specified GPO to <state> for <duration> milliseconds.";
}
