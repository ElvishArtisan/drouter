// server_d.cpp
//
// Software Authority Protocol Server for lwpathd(8).
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#include <QStringList>

#include "server_d.h"

ServerDConnection::ServerDConnection()
{
  dsts_subscribed=false;
  gpis_subscribed=false;
  gpos_subscribed=false;
  nodes_subscribed=false;
  srcs_subscribed=false;
}


bool ServerDConnection::dstsSubscribed() const
{
  return dsts_subscribed;
}


void ServerDConnection::setDstsSubscribed(bool state)
{
  dsts_subscribed=state;
}


bool ServerDConnection::gpisSubscribed() const
{
  return gpis_subscribed;
}


void ServerDConnection::setGpisSubscribed(bool state)
{
  gpis_subscribed=state;
}


bool ServerDConnection::gposSubscribed() const
{
  return gpos_subscribed;
}


void ServerDConnection::setGposSubscribed(bool state)
{
  gpos_subscribed=state;
}


bool ServerDConnection::nodesSubscribed() const
{
  return nodes_subscribed;
}


void ServerDConnection::setNodesSubscribed(bool state)
{
  nodes_subscribed=state;
}


bool ServerDConnection::srcsSubscribed() const
{
  return srcs_subscribed;
}


void ServerDConnection::setSrcsSubscribed(bool state)
{
  srcs_subscribed=state;
}




ServerD::ServerD(QObject *parent)
  : ServerNet(SERVER_D_PORT,parent)
{
}


ServerD::~ServerD()
{
}


void ServerD::newConnection(int id,NetConnection *conn)
{
  conn->priv=new ServerDConnection();
}


void ServerD::aboutToCloseConnection(int id,NetConnection *conn)
{
  delete (ServerDConnection *)conn->priv;
}


void ServerD::processCommand(int id,const SyAString &cmd)
{
  //  printf("processCommand(%d,%s)\n",id,(const char *)cmd.toUtf8());

  QStringList cmds=cmd.split(" ");

  if(cmds.at(0).isEmpty()) {
    return;
  }
  if(cmds.at(0).toLower()=="exit") {
    closeConnection(id);
    send("ok\r\n",id);
    return;
  }

  if(cmds.at(0).toLower()=="listdestinations") {
    emit processListDestinations(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="listnodes") {
    emit processListNodes(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="listgpis") {
    emit processListGpis(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="listgpos") {
    emit processListGpos(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="listsources") {
    emit processListSources(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="ping") {
    send("pong\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="subscribedestinations") {
    emit processSubscribeDestinations(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="subscribegpis") {
    emit processSubscribeGpis(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="subscribegpos") {
    emit processSubscribeGpos(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="subscribenodes") {
    emit processSubscribeNodes(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="subscribesources") {
    emit processSubscribeSources(id);
    send("ok\r\n",id);
    return;
  }
  if(cmds.at(0).toLower()=="clearcrosspoint") {
    if(cmds.size()==3) {
      bool ok=false;
      QHostAddress dst_hostaddr(cmds.at(1));
      if(!dst_hostaddr.isNull()) {
	int dst_slot=cmds.at(2).toInt(&ok);
	if(ok&&(dst_slot>=0)) {
	  emit processClearCrosspoint(id,dst_hostaddr,dst_slot);
	  send("ok\r\n",id);
	  return;
	}
      }
    }
  }
  if(cmds.at(0).toLower()=="setcrosspoint") {
    if(cmds.size()==5) {
      bool ok=false;
      QHostAddress dst_hostaddr(cmds.at(1));
      if(!dst_hostaddr.isNull()) {
	int dst_slot=cmds.at(2).toInt(&ok);
	if(ok&&(dst_slot>=0)) {
	  QHostAddress src_hostaddr(cmds.at(3));
	  if(!src_hostaddr.isNull()) {
	    int src_slot=cmds.at(4).toInt(&ok);
	    if(ok&&(src_slot>=0)) {
	      emit processSetCrosspoint(id,dst_hostaddr,dst_slot,
					src_hostaddr,src_slot);
	      send("ok\r\n",id);
	      return;
	    }
	  }
	}
      }      
    }
  }
  send("error\r\n",id);
}
