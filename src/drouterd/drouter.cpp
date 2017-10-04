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
#include <QSignalMapper>

#include <sy/sycmdswitch.h>
#include <sy/syinterfaces.h>
#include <sy/symcastsocket.h>

#include "drouter.h"

DRouter::DRouter(QObject *parent)
  : QObject(parent)
{
  //
  // Livewire Advertisement Sockets
  //
  QSignalMapper *mapper=new QSignalMapper(this);
  connect(mapper,SIGNAL(mapped(int)),this,SLOT(advtReadyReadData(int)));
  SyInterfaces *ifaces=new SyInterfaces();
  if(!ifaces->update()) {
    fprintf(stderr,"drouterd: unable to get network interface information\n");
    exit(1);
  }
  for(int i=0;i<ifaces->quantity();i++) {
    drouter_advt_sockets.
      push_back(new SyMcastSocket(SyMcastSocket::ReadOnly,this));
    if(!drouter_advt_sockets.back()->
       bind(ifaces->ipv4Address(i),SWITCHYARD_ADVERTS_PORT)) {
      fprintf(stderr,"drouterd: unable to bind %s:%d\n",
	      (const char *)ifaces->ipv4Address(i).toString().toUtf8(),
	      SWITCHYARD_ADVERTS_PORT);
      exit(1);
    }
    drouter_advt_sockets.back()->subscribe(SWITCHYARD_ADVERTS_ADDRESS);
    mapper->setMapping(drouter_advt_sockets.back(),
		       drouter_advt_sockets.size()-1);
    connect(drouter_advt_sockets.back(),SIGNAL(readyRead()),
	    mapper,SLOT(map()));
  }
}


QList<QHostAddress> DRouter::nodeHostAddresses() const
{
  QList<QHostAddress> addrs;

  for(QMap<unsigned,SyLwrpClient *>::const_iterator it=drouter_nodes.constBegin();
      it!=drouter_nodes.constEnd();it++) {
    if(it.value()->isConnected()) {
      addrs.push_back(it.value()->hostAddress());
    }
  }
  return addrs;
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


SyLwrpClient *DRouter::nodeBySrcStream(const QHostAddress &strmaddress,
				       int *slot)
{
  for(QMap<unsigned,SyLwrpClient *>::const_iterator it=drouter_nodes.begin();
      it!=drouter_nodes.end();it++) {
    for(unsigned i=0;i<it.value()->srcSlots();i++) {
      if(it.value()->srcAddress(i)==strmaddress) {
	*slot=i;
	return it.value();
      }
    }
  }
  return NULL;
}


SySource *DRouter::src(int srcnum) const
{
  for(QMap<unsigned,SyLwrpClient *>::const_iterator it=drouter_nodes.constBegin();
      it!=drouter_nodes.constEnd();it++) {
    for(unsigned i=0;i<it.value()->srcSlots();i++) {
      if(it.value()->srcNumber(i)==srcnum) {
	return it.value()->src(i);
      }
    }
  }
  return NULL;
}


SySource *DRouter::src(const QHostAddress &hostaddr,int slot) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->srcSlots()) {
      return lwrp->src(slot);
    }
  }
  return NULL;
}


SyDestination *DRouter::dst(const QHostAddress &hostaddr,int slot) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->dstSlots()) {
      return lwrp->dst(slot);
    }
  }
  return NULL;
}


SyGpioBundle *DRouter::gpi(const QHostAddress &hostaddr,int slot) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->gpis()) {
      return lwrp->gpiBundle(slot);
    }
  }
  return NULL;
}


SyGpo *DRouter::gpo(const QHostAddress &hostaddr,int slot) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->gpos()) {
      return lwrp->gpo(slot);
    }
  }
  return NULL;
}


bool DRouter::clearCrosspoint(const QHostAddress &dst_hostaddr,int dst_slot)
{
  SyLwrpClient *lwrp=NULL;

  //
  // Get the destination
  //
  if((lwrp=drouter_nodes.value(dst_hostaddr.toIPv4Address()))==NULL) {
    return false;
  }
  if(dst_slot>=(int)lwrp->dstSlots()) {
    return false;
  }
  lwrp->setDstAddress(dst_slot,0);

  return true;
}


bool DRouter::setCrosspoint(const QHostAddress &dst_hostaddr,int dst_slot,
			    const QHostAddress &src_hostaddr,int src_slot)
{
  SyLwrpClient *lwrp=NULL;

  //
  // Get the source
  //
  if((lwrp=drouter_nodes.value(src_hostaddr.toIPv4Address()))==NULL) {
    return false;
  }
  if(src_slot>=(int)lwrp->srcSlots()) {
    return false;
  }
  QHostAddress addr=lwrp->srcAddress(src_slot);

  //
  // Get the destination
  //
  if((lwrp=drouter_nodes.value(dst_hostaddr.toIPv4Address()))==NULL) {
    return false;
  }
  if(dst_slot>=(int)lwrp->dstSlots()) {
    return false;
  }
  lwrp->setDstAddress(dst_slot,addr);

  return true;
}


void DRouter::nodeConnectedData(unsigned id,bool state)
{
  if(state) {
    if(node(QHostAddress(id))==NULL) {
      fprintf(stderr,"DRouter::nodeConnectedData() - received connect signal from unknown node\n");
      exit(256);
    }
    emit nodeAdded(*(node(QHostAddress(id))->node()));
  }
  else {
    SyLwrpClient *lwrp=node(QHostAddress(id));
    if(lwrp==NULL) {
      fprintf(stderr,"DRouter::nodeConnectedData() - received disconnect signal from unknown node\n");
      exit(256);
    }
    emit nodeAboutToBeRemoved(*(lwrp->node()));
    drouter_nodes.erase(drouter_nodes.find(id));
  }
}


void DRouter::sourceChangedData(unsigned id,int slotnum,const SyNode &node,
				const SySource &src)
{
  emit srcChanged(node,slotnum,src);
}


void DRouter::destinationChangedData(unsigned id,int slotnum,const SyNode &node,
				     const SyDestination &dst)
{
  emit dstChanged(node,slotnum,dst);
}


void DRouter::gpiChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpioBundle &gpi)
{
  emit gpiChanged(node,slotnum,gpi);
}


void DRouter::gpoChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpo &gpo)
{
  emit gpoChanged(node,slotnum,gpo);
}


void DRouter::advtReadyReadData(int ifnum)
{
  QHostAddress addr;
  char data[1501];
  int n;

  while((n=drouter_advt_sockets.at(ifnum)->readDatagram(data,1500,&addr))>0) {
    if(node(addr)==NULL) {
      SyLwrpClient *node=new SyLwrpClient(addr.toIPv4Address(),this);
      connect(node,SIGNAL(connected(unsigned,bool)),
	      this,SLOT(nodeConnectedData(unsigned,bool)));
      connect(node,
	      SIGNAL(sourceChanged(unsigned,int,const SyNode,const SySource &)),
	      this,SLOT(sourceChangedData(unsigned,int,const SyNode,
					  const SySource &)));
      connect(node,SIGNAL(destinationChanged(unsigned,int,const SyNode &,
					     const SyDestination &)),
	      this,SLOT(destinationChangedData(unsigned,int,const SyNode &,
					       const SyDestination &)));
      connect(node,SIGNAL(gpiChanged(unsigned,int,const SyNode &,
				     const SyGpioBundle &)),
	      this,SLOT(gpiChangedData(unsigned,int,const SyNode &,
				       const SyGpioBundle &)));
      connect(node,
	      SIGNAL(gpoChanged(unsigned,int,const SyNode &,const SyGpo &)),
	      this,
	      SLOT(gpoChangedData(unsigned,int,const SyNode &,const SyGpo &)));
      drouter_nodes[addr.toIPv4Address()]=node;
      node->connectToHost(addr,SWITCHYARD_LWRP_PORT,"",false);
    }
  }
}
