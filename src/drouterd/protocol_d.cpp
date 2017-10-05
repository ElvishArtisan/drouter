// protocol_d.cpp
//
// Protocol "D" for DRouter
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

#include "protocol_d.h"

ProtocolD::ProtocolD(DRouter *router,int sock,QObject *parent)
  : Protocol(router,Protocol::ProtocolD)
{
  d_server=new ServerD(sock,this);
  connect(d_server,SIGNAL(processListDestinations(int)),
	  this,SLOT(processListDestinationsD(int)));
  connect(d_server,SIGNAL(processListGpis(int)),
	  this,SLOT(processListGpisD(int)));
  connect(d_server,SIGNAL(processListGpos(int)),
	  this,SLOT(processListGposD(int)));
  connect(d_server,SIGNAL(processListNodes(int)),
	  this,SLOT(processListNodesD(int)));
  connect(d_server,SIGNAL(processListSources(int)),
	  this,SLOT(processListSourcesD(int)));
  connect(d_server,SIGNAL(processSubscribeDestinations(int)),
	  this,SLOT(processSubscribeDestinationsD(int)));
  connect(d_server,SIGNAL(processSubscribeGpis(int)),
	  this,SLOT(processSubscribeGpisD(int)));
  connect(d_server,SIGNAL(processSubscribeGpos(int)),
	  this,SLOT(processSubscribeGposD(int)));
  connect(d_server,SIGNAL(processSubscribeNodes(int)),
	  this,SLOT(processSubscribeNodesD(int)));
  connect(d_server,SIGNAL(processSubscribeSources(int)),
	  this,SLOT(processSubscribeSourcesD(int)));
  connect(d_server,SIGNAL(processClearCrosspoint(int,const QHostAddress &,int)),
	  this,SLOT(processClearCrosspointD(int,const QHostAddress &,int)));
  connect(d_server,
	  SIGNAL(processClearGpioCrosspoint(int,const QHostAddress &,int)),
	  this,SLOT(processClearGpioCrosspointD(int,const QHostAddress &,int)));
  connect(d_server,SIGNAL(processSetCrosspoint(int,const QHostAddress &,int,
					       const QHostAddress &,int)),
	  this,SLOT(processSetCrosspointD(int,const QHostAddress &,int,
					  const QHostAddress &,int)));
  connect(d_server,SIGNAL(processSetGpioCrosspoint(int,const QHostAddress &,int,
						   const QHostAddress &,int)),
	  this,SLOT(processSetGpioCrosspointD(int,const QHostAddress &,int,
					      const QHostAddress &,int)));

  d_server->setReady(true);
}


void ProtocolD::processListDestinationsD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyDestination *dst=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->dstSlots();j++) {
	if((dst=lwrp->dst(j))!=NULL) {
	  d_server->send(DestinationRecord("DST",lwrp,j,dst),id);
	}
      }
    }
  }
}


void ProtocolD::processListGpisD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyGpioBundle *gpi=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->gpis();j++) {
	if((gpi=lwrp->gpiBundle(j))!=NULL) {
	  d_server->send(GpiRecord("GPI",lwrp,j,gpi),id);
	}
      }
    }
  }
}


void ProtocolD::processListGposD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyGpo *gpo=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->gpis();j++) {
	if((gpo=lwrp->gpo(j))!=NULL) {
	  d_server->send(GpoRecord("GPO",lwrp,j,gpo),id);
	}
      }
    }
  }
}


void ProtocolD::processListNodesD(int id)
{
  SyLwrpClient *lwrp=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      d_server->send(NodeRecord("NODE",lwrp),id);
    }
  }
}


void ProtocolD::processListSourcesD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SySource *src=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->srcSlots();j++) {
	if((src=lwrp->src(j))!=NULL) {
	  d_server->send(SourceRecord("SRC",lwrp,j,src),id);
	}
      }
    }
  }
}


void ProtocolD::processSubscribeDestinationsD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyDestination *dst=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->dstSlots();j++) {
	if((dst=lwrp->dst(j))!=NULL) {
	  d_server->send(DestinationRecord("DSTADD",lwrp,j,dst),id);
	}
      }
    }
  }
  ServerDConnection *conn=(ServerDConnection *)(d_server->connection(id)->priv);
  conn->setDstsSubscribed(true);
}


void ProtocolD::processSubscribeGpisD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyGpioBundle *gpi=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->gpis();j++) {
	if((gpi=lwrp->gpiBundle(j))!=NULL) {
	  d_server->send(GpiRecord("GPIADD",lwrp,j,gpi),id);
	}
      }
    }
  }
  ServerDConnection *conn=(ServerDConnection *)(d_server->connection(id)->priv);
  conn->setGpisSubscribed(true);
}


void ProtocolD::processSubscribeGposD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SyGpo *gpo=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->gpos();j++) {
	if((gpo=lwrp->gpo(j))!=NULL) {
	  d_server->send(GpoRecord("GPOADD",lwrp,j,gpo),id);
	}
      }
    }
  }
  ServerDConnection *conn=(ServerDConnection *)(d_server->connection(id)->priv);
  conn->setGposSubscribed(true);
}


void ProtocolD::processSubscribeNodesD(int id)
{
  SyLwrpClient *lwrp=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      d_server->send(NodeRecord("NODEADD",lwrp),id);
    }
  }
  ServerDConnection *conn=(ServerDConnection *)(d_server->connection(id)->priv);
  conn->setNodesSubscribed(true);
}


void ProtocolD::processSubscribeSourcesD(int id)
{
  SyLwrpClient *lwrp=NULL;
  SySource *src=NULL;

  QList<QHostAddress> addrs=router()->nodeHostAddresses();
  for(int i=0;i<addrs.size();i++) {
    if((lwrp=router()->node(addrs.at(i)))!=NULL) {
      for(unsigned j=0;j<lwrp->srcSlots();j++) {
	if((src=lwrp->src(j))!=NULL) {
	  d_server->send(SourceRecord("SRCADD",lwrp,j,src),id);
	}
      }
    }
  }
  ServerDConnection *conn=(ServerDConnection *)(d_server->connection(id)->priv);
  conn->setSrcsSubscribed(true);
}


void ProtocolD::processClearCrosspointD(int id,
					const QHostAddress &dst_hostaddr,
					int dst_slot)
{
  router()->clearCrosspoint(dst_hostaddr,dst_slot);
}


void ProtocolD::processClearGpioCrosspointD(int id,
					    const QHostAddress &gpo_hostaddr,
					    int gpo_slot)
{
  router()->clearGpioCrosspoint(gpo_hostaddr,gpo_slot);
}


void ProtocolD::processSetCrosspointD(int id,const QHostAddress &dst_hostaddr,
				      int dst_slot,
				      const QHostAddress &src_hostaddr,
				      int src_slot)
{
  router()->setCrosspoint(dst_hostaddr,dst_slot,src_hostaddr,src_slot);
}


void ProtocolD::processSetGpioCrosspointD(int id,
					  const QHostAddress &gpo_hostaddr,
					  int gpo_slot,
					  const QHostAddress &gpi_hostaddr,
					  int gpi_slot)
{
  router()->setGpioCrosspoint(gpo_hostaddr,gpo_slot,gpi_hostaddr,gpi_slot);
}


void ProtocolD::processAddedNode(const SyNode &node)
{
  SyLwrpClient *lwrp=NULL;

  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->nodesSubscribed()) {
	d_server->send(NodeRecord("NODEADD",node),i);
      }
      if(conn->gpisSubscribed()) {
	for(unsigned j=0;j<node.gpiSlotQuantity();j++) {
	  if((lwrp=router()->node(node.hostAddress()))!=NULL) {
	    d_server->send(GpiRecord("GPIADD",lwrp,j,lwrp->gpiBundle(j)),i);
	  }
	}
      }
      if(conn->gposSubscribed()) {
	for(unsigned j=0;j<node.gpoSlotQuantity();j++) {
	  if((lwrp=router()->node(node.hostAddress()))!=NULL) {
	    d_server->send(GpoRecord("GPOADD",lwrp,j,lwrp->gpo(j)),i);
	  }
	}
      }
      if(conn->dstsSubscribed()) {
	for(unsigned j=0;j<node.dstSlotQuantity();j++) {
	  if((lwrp=router()->node(node.hostAddress()))!=NULL) {
	    d_server->send(DestinationRecord("DSTADD",lwrp,j,lwrp->dst(j)),i);
	  }
	}
      }
      if(conn->srcsSubscribed()) {
	for(unsigned j=0;j<node.srcSlotQuantity();j++) {
	  if((lwrp=router()->node(node.hostAddress()))!=NULL) {
	    d_server->send(SourceRecord("SRCADD",lwrp,j,lwrp->src(j)),i);
	  }
	}
      }
    }
  }
}


void ProtocolD::processAboutToBeRemovedNode(const SyNode &node)
{
  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->nodesSubscribed()) {
	d_server->send("NODEDEL\t"+node.hostAddress().toString()+"\r\n",i);
      }
      if(conn->dstsSubscribed()) {
	for(unsigned j=0;j<node.dstSlotQuantity();j++) {
	  d_server->send("DSTDEL\t"+node.hostAddress().toString()+"\t"+
			 QString().sprintf("%u",j)+"\r\n",i);
	}
      }
      if(conn->srcsSubscribed()) {
	for(unsigned j=0;j<node.srcSlotQuantity();j++) {
	  d_server->send("SRCDEL\t"+node.hostAddress().toString()+"\t"+
			 QString().sprintf("%u",j)+"\r\n",i);	  
	}
      }
      if(conn->gpisSubscribed()) {
	for(unsigned j=0;j<node.gpiSlotQuantity();j++) {
	  d_server->send("GPIDEL\t"+node.hostAddress().toString()+"\t"+
			 QString().sprintf("%u",j)+"\r\n",i);	  
	}
      }
      if(conn->gposSubscribed()) {
	for(unsigned j=0;j<node.gpoSlotQuantity();j++) {
	  d_server->send("GPODEL\t"+node.hostAddress().toString()+"\t"+
			 QString().sprintf("%u",j)+"\r\n",i);	  
	}
      }
    }
  }
}


void ProtocolD::processChangedSource(const SyNode &node,int slot,
				     const SySource &src)
{
  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->srcsSubscribed()) {
	d_server->send(SourceRecord("SRC",node,slot,src),i);
      }
    }
  }
}


void ProtocolD::processChangedDestination(const SyNode &node,int slot,
					  const SyDestination &dst)
{
  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->dstsSubscribed()) {
	d_server->send(DestinationRecord("DST",node,slot,dst),i);
      }
    }
  }
}


void ProtocolD::processChangedGpi(const SyNode &node,int slot,
				  const SyGpioBundle &gpi)
{
  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->gpisSubscribed()) {
	d_server->send(GpiRecord("GPI",node,slot,gpi),i);
      }
    }
  }
}


void ProtocolD::processChangedGpo(const SyNode &node,int slot,const SyGpo &gpo)
{
  QList<NetConnection *> conns=d_server->connections();
  for(int i=0;i<conns.size();i++) {
    if(conns.at(i)!=NULL) {
      ServerDConnection *conn=(ServerDConnection *)(conns.at(i)->priv);
      if(conn->gposSubscribed()) {
	d_server->send(GpoRecord("GPO",node,slot,gpo),i);
      }
    }
  }
}


QString ProtocolD::DestinationRecord(const QString &keyword,SyLwrpClient *lwrp,
				     int slot,SyDestination *dst) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=lwrp->hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=lwrp->hostName()+"\t";
  ret+=dst->streamAddress().toString()+"\t";
  ret+=dst->name()+"\t";
  ret+=QString().sprintf("%u",dst->channels());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::DestinationRecord(const QString &keyword,const SyNode &node,
				     int slot,const SyDestination &dst) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=node.hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=node.hostName()+"\t";
  ret+=dst.streamAddress().toString()+"\t";
  ret+=dst.name()+"\t";
  ret+=QString().sprintf("%u",dst.channels());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpiRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
			     SyGpioBundle *gpi)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=lwrp->hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=lwrp->hostName()+"\t";
  ret+=gpi->code();
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpiRecord(const QString &keyword,const SyNode &node,int slot,
			     const SyGpioBundle &gpi)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=node.hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=node.hostName()+"\t";
  ret+=gpi.code();
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpoRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
			     SyGpo *gpo)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=lwrp->hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=lwrp->hostName()+"\t";
  ret+=gpo->bundle()->code()+"\t";
  ret+=gpo->name()+"\t";
  ret+=gpo->sourceAddress().toString()+"\t";
  ret+=QString().sprintf("%d",gpo->sourceSlot());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpoRecord(const QString &keyword,const SyNode &node,int slot,
			     const SyGpo &gpo)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=node.hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=node.hostName()+"\t";
  ret+=gpo.bundle()->code()+"\t";
  ret+=gpo.name()+"\t";
  ret+=gpo.sourceAddress().toString()+"\t";
  ret+=QString().sprintf("%d",gpo.sourceSlot());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::NodeRecord(const QString &keyword,SyLwrpClient *lwrp) const
{
  QString ret;

  ret+=keyword+"\t";
  ret+=lwrp->hostAddress().toString()+"\t";
  ret+=lwrp->hostName()+"\t";
  ret+=lwrp->deviceName()+"\t";
  ret+=QString().sprintf("%u\t",lwrp->srcSlots());
  ret+=QString().sprintf("%u\t",lwrp->dstSlots());
  ret+=QString().sprintf("%u\t",lwrp->gpis());
  ret+=QString().sprintf("%u",lwrp->gpos());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::NodeRecord(const QString &keyword,const SyNode &node) const
{
  QString ret;

  ret+=keyword+"\t";
  ret+=node.hostAddress().toString()+"\t";
  ret+=node.hostName()+"\t";
  ret+=node.deviceName()+"\t";
  ret+=QString().sprintf("%u\t",node.srcSlotQuantity());
  ret+=QString().sprintf("%u\t",node.dstSlotQuantity());
  ret+=QString().sprintf("%u\t",node.gpiSlotQuantity());
  ret+=QString().sprintf("%u",node.gpoSlotQuantity());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::SourceRecord(const QString &keyword,SyLwrpClient *lwrp,
				int slot,SySource *src) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=lwrp->hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=lwrp->hostName()+"\t";
  ret+=src->streamAddress().toString()+"\t";
  ret+=src->name()+"\t";
  ret+=QString().sprintf("%u\t",src->enabled());
  ret+=QString().sprintf("%u\t",src->channels());
  ret+=QString().sprintf("%u",src->packetSize());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::SourceRecord(const QString &keyword,const SyNode &node,
				int slot,const SySource &src) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=node.hostAddress().toString()+"\t";
  ret+=QString().sprintf("%d\t",slot);
  ret+=node.hostName()+"\t";
  ret+=src.streamAddress().toString()+"\t";
  ret+=src.name()+"\t";
  ret+=QString().sprintf("%u\t",src.enabled());
  ret+=QString().sprintf("%u\t",src.channels());
  ret+=QString().sprintf("%u",src.packetSize());
  ret+="\r\n";

  return ret;
}
