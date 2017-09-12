// drouter.cpp
//
// Dynamic router for Livewire networks
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <QHostAddress>

#include <sy/sycmdswitch.h>
#include <sy/symcastsocket.h>

#include "drouter.h"

DRouter::DRouter(QObject *parent)
  : QObject(parent)
{
  //
  // Livewire Advertisement Socket
  //
  drouter_advt_socket=new SyMcastSocket(SyMcastSocket::ReadOnly,this);
  if(!drouter_advt_socket->bind(SWITCHYARD_ADVERTS_PORT)) {
    fprintf(stderr,"drouterd: unable to bind port %d\n",
	    SWITCHYARD_ADVERTS_PORT);
    exit(1);
  }
  drouter_advt_socket->subscribe(SWITCHYARD_ADVERTS_ADDRESS);
  connect(drouter_advt_socket,SIGNAL(readyRead()),
	  this,SLOT(advtReadyReadData()));
}


SyLwrpClient *DRouter::node(const QHostAddress &hostaddr)
{
  try {
    return drouter_nodes.value(hostaddr.toIPv4Address());
  }
  catch(...) {
    return NULL;
  }
}


void DRouter::nodeConnectedData(unsigned id,bool state)
{
  if(state) {
    if(node(QHostAddress(id))==NULL) {
      fprintf(stderr,"DRouter::nodeConnectedData() - received connect signal from unknown node\n");
      exit(256);
    }
    emit nodeAdded(QHostAddress(id));
  }
  else {
    SyLwrpClient *lwrp=node(QHostAddress(id));
    if(lwrp==NULL) {
      fprintf(stderr,"DRouter::nodeConnectedData() - received disconnect signal from unknown node\n");
      exit(256);
    }
    drouter_nodes.erase(drouter_nodes.find(id));
    delete lwrp;
    emit nodeRemoved(QHostAddress(id));
  }
}


void DRouter::advtReadyReadData()
{
  QHostAddress addr;
  char data[1501];
  int n;

  while((n=drouter_advt_socket->readDatagram(data,1500,&addr))>0) {
    if(node(addr)==NULL) {
      SyLwrpClient *node=new SyLwrpClient(addr.toIPv4Address(),this);
      connect(node,SIGNAL(connected(unsigned,bool)),
	      this,SLOT(nodeConnectedData(unsigned,bool)));
      drouter_nodes[addr.toIPv4Address()]=node;
      node->connectToHost(addr,SWITCHYARD_LWRP_PORT,"",false);
    }
  }
}
