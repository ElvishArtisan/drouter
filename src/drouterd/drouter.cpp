// drouter.cpp
//
// Dynamic router database component for Drouter
//
//   (C) Copyright 2018-2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <errno.h>
#include <linux/un.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QSignalMapper>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <sy/syconfig.h>
#include <sy/syinterfaces.h>

#include "drouter.h"
#include "protoipc.h"

DRouter::DRouter(int *proto_socks,QObject *parent)
  : QObject(parent)
{
  drouter_proto_socks=proto_socks;
  drouter_writeable=false;

  drouter_config=new Config();
  drouter_config->load();

  drouter_flasher=new GpioFlasher(this);
}


DRouter::~DRouter()
{
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
  if((0xFFFF&strmaddress.toIPv4Address())==0) {
    return NULL;
  }
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


bool DRouter::clipAlarmActive(const QHostAddress &hostaddr,int slot,
			      SyLwrpClient::MeterType type,int chan) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    return lwrp->clipAlarmActive(slot,type,chan);
  }
  return false;
}


bool DRouter::silenceAlarmActive(const QHostAddress &hostaddr,int slot,
				 SyLwrpClient::MeterType type,int chan) const
{
  SyLwrpClient *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    return lwrp->silenceAlarmActive(slot,type,chan);
  }
  return false;
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


bool DRouter::start(QString *err_msg)
{
  LoadMaps();
  if(!StartDb(err_msg)) {
    return false;
  }
  if(!StartProtocolIpc(err_msg)) {
    return false;
  }
  if(!StartLivewire(err_msg)) {
    return false;
  }

  return true;
}


bool DRouter::isWriteable() const
{
  return drouter_writeable;
}


void DRouter::setWriteable(bool state)
{
  QString sql;
  QSqlQuery *q;

  if(drouter_writeable!=state) {
    drouter_flasher->setActive(state);

    //
    // Update Protocols
    //
    QString letter="N";
    if(state) {
      letter="Y";
    }
    sql=QString("update TETHER set IS_ACTIVE='"+letter+"'");
    q=new QSqlQuery(sql);
    delete q;
    drouter_writeable=state;
    NotifyProtocols("TETHER",letter);
  }
}


void DRouter::nodeConnectedData(unsigned id,bool state)
{
  QString sql;
  QSqlQuery *q;
  int endpt;
  int last_id=0;

  if(state) {
    if(node(QHostAddress(id))==NULL) {
      syslog(LOG_ERR,"DRouter::nodeConnectedData() - received connect signal from unknown node, aborting");
      exit(256);
    }
    SyLwrpClient *lwrp=node(QHostAddress(id));
    Log(drouter_config->nodeLogPriority(),
	"node connected from "+QHostAddress(id).toString()+
	" ["+lwrp->hostName()+" / "+lwrp->deviceName()+"]");
    if(drouter_config->configureAudioAlarms(lwrp->deviceName())) {
      for(unsigned i=0;i<lwrp->srcSlots();i++) {
	if(lwrp->src(i)->exists()) {
	  if((drouter_config->clipAlarmThreshold()<0)&&
	     (drouter_config->clipAlarmTimeout()>0)) {
	    lwrp->setClipMonitor(i,SyLwrpClient::InputMeter,
				 drouter_config->clipAlarmThreshold(),
				 drouter_config->clipAlarmTimeout());
	  }
	  if((drouter_config->silenceAlarmThreshold()<0)&&
	     (drouter_config->silenceAlarmTimeout()>0)) {
	    lwrp->setSilenceMonitor(i,SyLwrpClient::InputMeter,
				    drouter_config->silenceAlarmThreshold(),
				    drouter_config->silenceAlarmTimeout());
	  }
	}
      }
      for(unsigned i=0;i<lwrp->dstSlots();i++) {
	if(lwrp->dst(i)->exists()) {
	  if((drouter_config->clipAlarmThreshold()<0)&&
	     (drouter_config->clipAlarmTimeout()>0)) {
	    lwrp->setClipMonitor(i,SyLwrpClient::OutputMeter,
				 drouter_config->clipAlarmThreshold(),
				 drouter_config->clipAlarmTimeout());
	  }
	  if((drouter_config->silenceAlarmThreshold()<0)&&
	     (drouter_config->silenceAlarmTimeout()>0)) {
	    lwrp->setSilenceMonitor(i,SyLwrpClient::OutputMeter,
				    drouter_config->silenceAlarmThreshold(),
				    drouter_config->silenceAlarmTimeout());
	  }
	}
      }
    }
    LockTables();
    sql=QString("insert into NODES set ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
      "HOST_NAME=\""+lwrp->hostName()+"\","+
      "DEVICE_NAME=\""+lwrp->deviceName()+"\","+
      QString().sprintf("SOURCE_SLOTS=%u,",lwrp->srcSlots())+
      QString().sprintf("DESTINATION_SLOTS=%u,",lwrp->dstSlots())+
      QString().sprintf("GPI_SLOTS=%u,",lwrp->gpis())+
      QString().sprintf("GPO_SLOTS=%u",lwrp->gpos());
    q=new QSqlQuery(sql);
    delete q;
    for(unsigned i=0;i<lwrp->srcSlots();i++) {
      sql=QString("insert into SOURCES set ")+
	"HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	QString().sprintf("SLOT=%u,",i)+
	"HOST_NAME=\""+lwrp->hostName()+"\","+
	"STREAM_ADDRESS=\""+
	Config::normalizedStreamAddress(lwrp->srcAddress(i)).toString()+"\","+
	"NAME=\""+lwrp->srcName(i)+"\","+
	QString().sprintf("STREAM_ENABLED=%u,",lwrp->srcEnabled(i))+
	QString().sprintf("CHANNELS=%u,",lwrp->srcChannels(i))+
	QString().sprintf("BLOCK_SIZE=%u",lwrp->srcPacketSize(i));
      q=new QSqlQuery(sql);
      delete q;
      sql=QString("select last_insert_id() from SOURCES");
      q=new QSqlQuery(sql);
      if(q->first()) {
	last_id=q->value(0).toInt();
      }
      delete q;
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::AudioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Input,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into SA_SOURCES set ")+
	      QString().sprintf("ROUTER_NUMBER=%d,",it.value()->routerNumber())+
	      QString().sprintf("SOURCE_NUMBER=%d,",endpt)+
	      QString().sprintf("SOURCE_ID=%d,",last_id)+
	      "STREAM_ADDRESS=\""+
	      Config::normalizedStreamAddress(lwrp->srcAddress(i)).toString()+"\","+
	      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	      QString().sprintf("SLOT=%d",i);
	    if(!it.value()->name(EndPointMap::Input,endpt).isEmpty()) {
	      sql+=",NAME=\""+it.value()->name(EndPointMap::Input,endpt)+"\"";
	    }
	    q=new QSqlQuery(sql);
	    delete q;
	  }
	}
      }
    }
    for(unsigned i=0;i<lwrp->dstSlots();i++) {
      sql=QString("insert into DESTINATIONS set ")+
	"HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	QString().sprintf("SLOT=%u,",i)+
	"HOST_NAME=\""+lwrp->hostName()+"\","+
	"STREAM_ADDRESS=\""+
	Config::normalizedStreamAddress(lwrp->dstAddress(i)).toString()+"\","+
	"NAME=\""+lwrp->dstName(i)+"\","+
	QString().sprintf("CHANNELS=%u",lwrp->dstChannels(i));
      q=new QSqlQuery(sql);
      delete q;
      sql=QString("select last_insert_id() from DESTINATIONS");
      q=new QSqlQuery(sql);
      if(q->first()) {
	last_id=q->value(0).toInt();
      }
      delete q;
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::AudioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Output,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into SA_DESTINATIONS set ")+
	      QString().sprintf("ROUTER_NUMBER=%d,",it.value()->routerNumber())+
	      QString().sprintf("SOURCE_NUMBER=%d,",endpt)+
	      QString().sprintf("DESTINATION_ID=%d,",last_id)+
	      "STREAM_ADDRESS=\""+
	      Config::normalizedStreamAddress(lwrp->dstAddress(i)).toString()+"\","+
	      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	      QString().sprintf("SLOT=%d",i);
	    if(!it.value()->name(EndPointMap::Output,endpt).isEmpty()) {
	      sql+=",NAME=\""+it.value()->name(EndPointMap::Output,endpt)+"\"";
	    }
	    q=new QSqlQuery(sql);
	    delete q;
	  }
	}
      }
    }
    for(unsigned i=0;i<lwrp->gpis();i++) {
      sql=QString("insert into GPIS set ")+
	"HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	QString().sprintf("SLOT=%u,",i)+
	"HOST_NAME=\""+lwrp->hostName()+"\","+
	"CODE=\""+lwrp->gpiBundle(i)->code()+"\"";
      q=new QSqlQuery(sql);
      delete q;
      sql=QString("select last_insert_id() from GPIS");
      q=new QSqlQuery(sql);
      if(q->first()) {
	last_id=q->value(0).toInt();
      }
      delete q;
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::GpioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Input,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into SA_GPIS set ")+
	      QString().sprintf("ROUTER_NUMBER=%d,",it.value()->routerNumber())+
	      QString().sprintf("SOURCE_NUMBER=%d,",endpt)+
	      QString().sprintf("GPI_ID=%d,",last_id)+
	      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	      QString().sprintf("SLOT=%d",i);
	    if(!it.value()->name(EndPointMap::Input,endpt).isEmpty()) {
	      sql+=",NAME=\""+it.value()->name(EndPointMap::Input,endpt)+"\"";
	    }
	    q=new QSqlQuery(sql);
	    delete q;
	  }
	}
      }
    }
    for(unsigned i=0;i<lwrp->gpos();i++) {
      QString name=lwrp->gpo(i)->name();
      if(name.isEmpty()) {
	name=QString().sprintf("GPO %d",i+1);
      }
      sql=QString("insert into GPOS set ")+
	"HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	QString().sprintf("SLOT=%u,",i)+
	"HOST_NAME=\""+lwrp->hostName()+"\","+
	"CODE=\""+lwrp->gpiBundle(i)->code()+"\","+
	"NAME=\""+name+"\","+
	"SOURCE_ADDRESS=\""+lwrp->gpo(i)->sourceAddress().toString()+"\","+
	QString().sprintf("SOURCE_SLOT=%d",lwrp->gpo(i)->sourceSlot());
      q=new QSqlQuery(sql);
      delete q;
      sql=QString("select last_insert_id() from GPOS");
      q=new QSqlQuery(sql);
      if(q->first()) {
	last_id=q->value(0).toInt();
      }
      delete q;
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::GpioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Output,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into SA_GPOS set ")+
	      QString().sprintf("ROUTER_NUMBER=%d,",it.value()->routerNumber())+
	      QString().sprintf("SOURCE_NUMBER=%d,",endpt)+
	      QString().sprintf("GPO_ID=%d,",last_id)+
	      "SOURCE_ADDRESS=\""+lwrp->gpo(i)->sourceAddress().toString()+"\","+
	      QString().sprintf("SOURCE_SLOT=%d,",lwrp->gpo(i)->sourceSlot())+
	      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\","+
	      QString().sprintf("SLOT=%d",i);
	    if(!it.value()->name(EndPointMap::Output,endpt).isEmpty()) {
	      sql+=",NAME=\""+it.value()->name(EndPointMap::Output,endpt)+"\"";
	    }
	    q=new QSqlQuery(sql);
	    delete q;
	  }
	}
      }
    }

    for(int i=0;i<2;i++) {
      Config::TetherRole role=(Config::TetherRole)i;
      if(lwrp->hostAddress()==drouter_config->tetherGpioIpAddress(role)) {
	drouter_flasher->
	  addGpio(role,lwrp,drouter_config->tetherGpioType(role),
		  drouter_config->tetherGpioSlot(role),
		  drouter_config->tetherGpioCode(role));
      }
    }

    UnlockTables();
    NotifyProtocols("NODEADD",QHostAddress(id).toString());
  }
  else {
    SyLwrpClient *lwrp=node(QHostAddress(id));
    if(lwrp==NULL) {
      syslog(LOG_ERR,"DRouter::nodeConnectedData() - received disconnect signal from unknown node, aborting");
      exit(256);
    }
    Log(drouter_config->nodeLogPriority(),
	"node disconnected from "+QHostAddress(id).toString()+
	" ["+lwrp->hostName()+" / "+lwrp->deviceName()+"]");
    LockTables();
    sql=QString("select ")+
      "SOURCE_SLOTS,"+       // 00
      "DESTINATION_SLOTS,"+  // 01
      "GPI_SLOTS,"+          // 02
      "GPO_SLOTS "+          // 03
      "from NODES where "+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    if(q->first()) {
      NotifyProtocols("NODEDEL",QHostAddress(id).toString(),
		      q->value(0).toInt(),q->value(1).toInt(),
		      q->value(2).toInt(),q->value(3).toInt());
    }
    delete q;
    sql=QString("delete from SA_SOURCES where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from SOURCES where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from SA_DESTINATIONS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from DESTINATIONS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from SA_GPIS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from GPIS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from SA_GPOS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from GPOS where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from NODES where ")+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\"";
    q=new QSqlQuery(sql);
    delete q;
    drouter_nodes.erase(drouter_nodes.find(id));
    UnlockTables();
  }
}


void DRouter::sourceChangedData(unsigned id,int slotnum,const SyNode &node,
				const SySource &src)
{
  QString sql;
  QSqlQuery *q;

  sql=QString("update SOURCES set ")+
    "HOST_NAME=\""+node.hostName()+"\","+
    "STREAM_ADDRESS=\""+
    Config::normalizedStreamAddress(src.streamAddress()).toString()+"\","+
    "NAME=\""+src.name()+"\","+
    QString().sprintf("STREAM_ENABLED=%u,",src.enabled())+
    QString().sprintf("CHANNELS=%u,",src.channels())+
    QString().sprintf("BLOCK_SIZE=%u where ",src.packetSize())+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  sql=QString("update SA_SOURCES set ")+
    "STREAM_ADDRESS=\""+
    Config::normalizedStreamAddress(src.streamAddress()).toString()+"\" where "+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  NotifyProtocols("SRC",QHostAddress(id).toString()+
		  QString().sprintf(":%u",slotnum));
}


void DRouter::destinationChangedData(unsigned id,int slotnum,const SyNode &node,
				     const SyDestination &dst)
{
  QString sql;
  QSqlQuery *q;
  bool xpoint_changed=false;

  sql=QString("select ")+
    "STREAM_ADDRESS from DESTINATIONS where "
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%d",slotnum);
  q=new QSqlQuery(sql);
  if(q->first()) {
    xpoint_changed=QHostAddress(q->value(0).toString())!=dst.streamAddress();
  }
  delete q;

  sql=QString("update DESTINATIONS set ")+
    "HOST_NAME=\""+node.hostName()+"\","+
    "STREAM_ADDRESS=\""+
    Config::normalizedStreamAddress(dst.streamAddress()).toString()+"\","+
    "NAME=\""+dst.name()+"\","+
    QString().sprintf("CHANNELS=%u where ",dst.channels())+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  sql=QString("update SA_DESTINATIONS set ")+
    "STREAM_ADDRESS=\""+
    Config::normalizedStreamAddress(dst.streamAddress()).toString()+"\" where "+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  if(xpoint_changed) {
    NotifyProtocols("DSTX",QHostAddress(id).toString()+
		    QString().sprintf(":%u",slotnum));
  }
  NotifyProtocols("DST",QHostAddress(id).toString()+
		  QString().sprintf(":%u",slotnum));
}


void DRouter::gpiChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpioBundle &gpi)
{
  QString sql;
  QSqlQuery *q;
  bool code_changed=false;

  sql=QString("select ")+
    "CODE "+            // 00
    "from GPIS where "+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%d",slotnum);
  q=new QSqlQuery(sql);
  if(q->first()) {
    code_changed=q->value(0).toString().toLower()!=gpi.code().toLower();
  }
  delete q;

  sql=QString("update GPIS set ")+
    "CODE=\""+gpi.code().toLower()+"\" where "+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  if(code_changed) {
    NotifyProtocols("GPICODE",QHostAddress(id).toString()+
		    QString().sprintf(":%u",slotnum));
  }
  NotifyProtocols("GPI",QHostAddress(id).toString()+
		  QString().sprintf(":%u",slotnum));
}


void DRouter::gpoChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpo &gpo)
{
  QString sql;
  QSqlQuery *q;
  bool xpoint_changed=false;
  bool code_changed=false;

  sql=QString("select ")+
    "SOURCE_ADDRESS,"+  // 00
    "SOURCE_SLOT,"+     // 01
    "CODE "+            // 02
    "from GPOS where "+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%d",slotnum);
  q=new QSqlQuery(sql);
  if(q->first()) {
    xpoint_changed=(QHostAddress(q->value(0).toString())!=gpo.sourceAddress()||
		    q->value(1).toInt()!=gpo.sourceSlot());
    code_changed=q->value(2).toString().toLower()!=gpo.bundle()->code().toLower();
  }
  delete q;

  sql=QString("update GPOS set ")+
    "CODE=\""+gpo.bundle()->code().toLower()+"\","+
    "NAME=\""+gpo.name()+"\","+
    "SOURCE_ADDRESS=\""+gpo.sourceAddress().toString()+"\","+
    QString().sprintf("SOURCE_SLOT=%d where ",gpo.sourceSlot())+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  sql=QString("update SA_GPOS set ")+
    "SOURCE_ADDRESS=\""+gpo.sourceAddress().toString()+"\","+
    QString().sprintf("SOURCE_SLOT=%d where ",gpo.sourceSlot())+
    "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
    QString().sprintf("SLOT=%u",slotnum);
  q=new QSqlQuery(sql);
  delete q;
  if(xpoint_changed) {
    NotifyProtocols("GPOX",QHostAddress(id).toString()+
		    QString().sprintf(":%u",slotnum));
  }
  if(code_changed) {
    NotifyProtocols("GPOCODE",QHostAddress(id).toString()+
		    QString().sprintf(":%u",slotnum));
  }
  NotifyProtocols("GPO",QHostAddress(id).toString()+
		  QString().sprintf(":%u",slotnum));
}


void DRouter::audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
				 unsigned slotnum,int chan,bool state)
{
  QString sql;
  QSqlQuery *q;
  SyLwrpClient *lwrp=NULL;
  QString table="SOURCES";
  QString chan_name="LEFT";

  if(type==SyLwrpClient::OutputMeter) {
    table="DESTINATIONS";
  }
  if(chan==1) {
    chan_name="RIGHT";
  }
  if((lwrp=drouter_nodes[id])!=NULL) {
    sql=QString("update ")+table+" set "+
      chan_name+"_CLIP="+QString().sprintf("%d where ",state)+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
      QString().sprintf("SLOT=%d",slotnum);
    q=new QSqlQuery(sql);
    delete q;
    NotifyProtocols("CLIP",QString().sprintf("%d:%d:",type,chan)+QHostAddress(id).toString()+QString().sprintf(":%d",slotnum));
    //    emit clipAlarmChanged(*(lwrp->node()),(int)slotnum,type,chan,state);
  }
}


void DRouter::audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
				    unsigned slotnum,int chan,bool state)
{
  QString sql;
  QSqlQuery *q;
  SyLwrpClient *lwrp=NULL;
  QString table="SOURCES";
  QString chan_name="LEFT";

  if(type==SyLwrpClient::OutputMeter) {
    table="DESTINATIONS";
  }
  if(chan==1) {
    chan_name="RIGHT";
  }

  if((lwrp=drouter_nodes[id])!=NULL) {
    sql=QString("update ")+table+" set "+
      chan_name+"_SILENCE="+QString().sprintf("%d where ",state)+
      "HOST_ADDRESS=\""+QHostAddress(id).toString()+"\" && "+
      QString().sprintf("SLOT=%d",slotnum);
    q=new QSqlQuery(sql);
    delete q;
    NotifyProtocols("SILENCE",QString().sprintf("%d:%d:",type,chan)+QHostAddress(id).toString()+QString().sprintf(":%d",slotnum));
  }
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
      connect(node,SIGNAL(audioClipAlarm(unsigned,SyLwrpClient::MeterType,
					 unsigned,int,bool)),
	      this,SLOT(audioClipAlarmData(unsigned,SyLwrpClient::MeterType,
					   unsigned,int,bool)));
      connect(node,SIGNAL(audioSilenceAlarm(unsigned,SyLwrpClient::MeterType,
					    unsigned,int,bool)),
	      this,SLOT(audioSilenceAlarmData(unsigned,SyLwrpClient::MeterType,
					      unsigned,int,bool)));
      drouter_nodes[addr.toIPv4Address()]=node;
      node->connectToHost(addr,SWITCHYARD_LWRP_PORT,
			  drouter_config->lwrpPassword(),false);
    }
  }
}


void DRouter::newIpcConnectionData(int listen_sock)
{
  int sock;

  if((sock=accept(listen_sock,NULL,NULL))<0) {
    syslog(LOG_WARNING,"DRouter::newIpcConnectionData - accept failed [%s]",strerror(errno));
    return;
  }
  drouter_ipc_sockets[sock]=new QTcpSocket(this);
  drouter_ipc_accums[sock]=QString();
  drouter_ipc_sockets[sock]->
    setSocketDescriptor(sock,QAbstractSocket::ConnectedState);
  connect(drouter_ipc_sockets[sock],SIGNAL(readyRead()),
	  drouter_ipc_ready_mapper,SLOT(map()));
  drouter_ipc_ready_mapper->
    setMapping(drouter_ipc_sockets[sock],sock);
  syslog(LOG_DEBUG,"opened new IPC connection %d", sock);
}


void DRouter::ipcReadyReadData(int sock)
{
  char data[1501];
  int n;

  while((n=drouter_ipc_sockets[sock]->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(0xFF&data[i]) {
      case 10:
	break;

      case 13:
	if(!ProcessIpcCommand(sock,drouter_ipc_accums[sock])) {
	  return;
	}
	drouter_ipc_accums[sock]="";
	break;

      default:
	drouter_ipc_accums[sock]+=0xFF&data[i];
	break;
      }
    }
  }
}


void DRouter::NotifyProtocols(const QString &type,const QString &id,
			      int srcs,int dsts,int gpis,int gpos)
{
  for(QMap<int,QTcpSocket *>::iterator it=drouter_ipc_sockets.begin();
      it!=drouter_ipc_sockets.end();it++) {
    if(gpos<0) {
      it.value()->write((type+":"+id+"\r\n").toUtf8());
    }
    else {
      it.value()->write((type+":"+id+
			 QString().sprintf(":%d:%d:%d:%d\r\n",
					   srcs,dsts,gpis,gpos)).toUtf8());
    }
  }
}


bool DRouter::StartProtocolIpc(QString *err_msg)
{
  int sock;
  struct sockaddr_un sa;

  //
  // UNIX Server
  //
  if((sock=socket(AF_UNIX,SOCK_SEQPACKET,0))<0) {
    *err_msg=tr("unable to start protocol ipc")+" ["+strerror(errno)+"]";
    return false;
  }
  memset(&sa,0,sizeof(sa));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path+1,DROUTER_IPC_ADDRESS,UNIX_PATH_MAX-1);
  if(bind(sock,(struct sockaddr *)(&sa),sizeof(sa))<0) {
    *err_msg=tr("unable to bind protocol ipc")+" ["+strerror(errno)+"]";
    return false;
  }
  if(listen(sock,3)<0) {
    *err_msg=tr("unable to listen protocol ipc")+" ["+strerror(errno)+"]";
    return false;
  }
  QSocketNotifier *socknotify=
    new QSocketNotifier(sock,QSocketNotifier::Read,this);
  connect(socknotify,SIGNAL(activated(int)),
	  this,SLOT(newIpcConnectionData(int)));

  //
  // Slot Mappers
  //
  drouter_ipc_ready_mapper=new QSignalMapper(this);
  connect(drouter_ipc_ready_mapper,SIGNAL(mapped(int)),
	  this,SLOT(ipcReadyReadData(int)));

  return true;
}


bool DRouter::ProcessIpcCommand(int sock,const QString &cmd)
{
  bool ok=false;

  syslog(LOG_DEBUG,"received IPC cmd: \"%s\" from connection %d",
	 (const char *)cmd.toUtf8(),sock);

  if(cmd=="QUIT") {
    drouter_ipc_sockets[sock]->close();
    drouter_ipc_sockets[sock]->deleteLater();
    drouter_ipc_sockets.remove(sock);
    drouter_ipc_accums.remove(sock);
    syslog(LOG_DEBUG,"closed IPC connection %d",sock);
    return false;
  }

  if(cmd=="SEND_D_SOCK") {
    SendProtoSocket(sock,drouter_proto_socks[0]);
    close(drouter_proto_socks[0]);
    drouter_ipc_sockets[sock]->close();
    drouter_ipc_sockets[sock]->deleteLater();
    drouter_ipc_sockets.remove(sock);
    drouter_ipc_accums.remove(sock);
    syslog(LOG_DEBUG,"closed IPC connection %d",sock);
    return false;
  }

  if(cmd=="SEND_SA_SOCK") {
    SendProtoSocket(sock,drouter_proto_socks[1]);
    close(drouter_proto_socks[1]);
    drouter_ipc_sockets[sock]->close();
    drouter_ipc_sockets[sock]->deleteLater();
    drouter_ipc_sockets.remove(sock);
    drouter_ipc_accums.remove(sock);
    syslog(LOG_DEBUG,"closed IPC connection %d",sock);
    return false;
  }

  //
  // All operations below here require that we be the active instance!
  // (drouter_writeable==true)
  //
  if(!drouter_writeable) {
    return true;
  }

  QStringList cmds=cmd.split(" ");

  if((cmds.at(0)=="ClearCrosspoint")&&(cmds.size()==3)) {
    SyLwrpClient *lwrp=drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned slotnum=cmds.at(2).toUInt(&ok);
    if((lwrp!=NULL)&&ok&&(slotnum<lwrp->dstSlots())) {
      lwrp->setDstAddress(slotnum,QHostAddress(DROUTER_NULL_STREAM_ADDRESS));
    }
  }

  if((cmds.at(0)=="ClearGpioCrosspoint")&&(cmds.size()==3)) {
    SyLwrpClient *lwrp=drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned slotnum=cmds.at(2).toUInt(&ok);
    if((lwrp!=NULL)&&ok&&(slotnum<lwrp->gpos())) {
      lwrp->setGpoSourceAddress(slotnum,QHostAddress(),-1);
    }
  }

  if((cmds.at(0)=="SetCrosspoint")&&(cmds.size()==5)) {
    SyLwrpClient *dst_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned dst_slotnum=cmds.at(2).toUInt(&ok);
    if((dst_lwrp!=NULL)&&ok&&(dst_slotnum<dst_lwrp->dstSlots())) {
      SyLwrpClient *src_lwrp=
	drouter_nodes[QHostAddress(cmds.at(3)).toIPv4Address()];
      unsigned src_slotnum=cmds.at(4).toUInt(&ok);
      if((src_lwrp!=NULL)&&ok&&(src_slotnum<src_lwrp->srcSlots())) {
	dst_lwrp->setDstAddress(dst_slotnum,src_lwrp->srcAddress(src_slotnum));
      }
    }
  }

  if((cmds.at(0)=="SetGpioCrosspoint")&&(cmds.size()==5)) {
    SyLwrpClient *gpo_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned gpo_slotnum=cmds.at(2).toUInt(&ok);
    if((gpo_lwrp!=NULL)&&ok&&(gpo_slotnum<gpo_lwrp->gpos())) {
      SyLwrpClient *gpi_lwrp=
	drouter_nodes[QHostAddress(cmds.at(3)).toIPv4Address()];
      unsigned gpi_slotnum=cmds.at(4).toUInt(&ok);
      if((gpi_lwrp!=NULL)&&ok&&(gpi_slotnum<gpi_lwrp->gpis())) {
	gpo_lwrp->
	  setGpoSourceAddress(gpo_slotnum,gpi_lwrp->hostAddress(),
			      gpi_slotnum);
      }
    }
  }

  if((cmds.at(0)=="SetGpiState")&&(cmds.size()==4)) {
    SyLwrpClient *gpi_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned gpi_slotnum=cmds.at(2).toUInt(&ok);
    if((gpi_lwrp!=NULL)&&ok&&(gpi_slotnum<gpi_lwrp->gpis())) {
      gpi_lwrp->setGpiCode(gpi_slotnum,cmds.at(3));
    }
  }

  if((cmds.at(0)=="SetGpoState")&&(cmds.size()==4)) {
    SyLwrpClient *gpo_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned gpo_slotnum=cmds.at(2).toUInt(&ok);
    if((gpo_lwrp!=NULL)&&ok&&(gpo_slotnum<gpo_lwrp->gpos())) {
      gpo_lwrp->setGpoCode(gpo_slotnum,cmds.at(3));
    }
  }

  return true;
}


bool DRouter::StartDb(QString *err_msg)
{
  QString sql;
  QSqlQuery *q;
  QSqlQuery *q1;

  //
  // Connect to Database
  //
  QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL3");
  db.setHostName("localhost");
  db.setDatabaseName("drouter");
  db.setUserName("drouter");
  db.setPassword("drouter");
  if(!db.open()) {
    *err_msg=tr("unable to open database")+" ["+db.lastError().driverText()+"]";
    return false;
  }

  //
  // Clear Old Data
  //
  sql="show tables";
  q=new QSqlQuery(sql);
  while(q->next()) {
    sql=QString("drop table `")+q->value(0).toString()+"`";
    q1=new QSqlQuery(sql);
    delete q1;
  }
  delete q;

  //
  // Create Schema
  //
  sql=QString("create table if not exists NODES (")+
    "HOST_ADDRESS char(15) not null primary key,"+
    "HOST_NAME char(16),"+
    "DEVICE_NAME char(20),"+
    "SOURCE_SLOTS int,"+
    "DESTINATION_SLOTS int,"+
    "GPI_SLOTS int,"+
    "GPO_SLOTS int) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists SOURCES (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "HOST_NAME char(16),"+
    "STREAM_ADDRESS char(15),"+
    "NAME char(16),"+
    "STREAM_ENABLED int,"+
    "CHANNELS int,"+
    "BLOCK_SIZE int,"+
    "LEFT_CLIP int default 0,"+
    "RIGHT_CLIP int default 0,"+
    "LEFT_SILENCE int default 0,"+
    "RIGHT_SILENCE int default 0,"+
    "unique index SLOT_IDX(HOST_ADDRESS,SLOT),"+
    "index STREAM_ADDRESS_IDX(STREAM_ADDRESS,STREAM_ENABLED)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists DESTINATIONS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "HOST_NAME char(16),"+
    "STREAM_ADDRESS char(15),"+
    "NAME char(16),"+
    "CHANNELS int,"+
    "LEFT_CLIP int default 0,"+
    "RIGHT_CLIP int default 0,"+
    "LEFT_SILENCE int default 0,"+
    "RIGHT_SILENCE int default 0,"+
    "unique index SLOT_IDX(HOST_ADDRESS,SLOT)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists GPIS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "HOST_NAME char(16),"+
    "CODE char(5),"+
    "unique index SLOT_IDX(HOST_ADDRESS,SLOT)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists GPOS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "HOST_NAME char(16),"+
    "CODE char(5),"+
    "NAME char(16),"+
    "SOURCE_ADDRESS char(22),"+
    "SOURCE_SLOT int default -1,"+
    "unique index SLOT_IDX(HOST_ADDRESS,SLOT),"+
    "index SOURCE_ADDRESS_IDX(SOURCE_ADDRESS,SOURCE_SLOT)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists SA_SOURCES (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "ROUTER_NUMBER int not null,"+
    "SOURCE_NUMBER int not null,"
    "SOURCE_ID int not null,"+
    "NAME char(32),"+
    "STREAM_ADDRESS char(15),"+
    "index HOST_ADDRESS_IDX(HOST_ADDRESS,SLOT),"+
    "index STREAM_ADDRESS_IDX(ROUTER_NUMBER,STREAM_ADDRESS),"+
    "index ROUTER_NUMBER_IDX(ROUTER_NUMBER)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists SA_DESTINATIONS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "ROUTER_NUMBER int not null,"+
    "SOURCE_NUMBER int not null,"
    "DESTINATION_ID int not null,"+
    "NAME char(32),"+
    "STREAM_ADDRESS char(15),"+
    "index HOST_ADDRESS_IDX(HOST_ADDRESS,SLOT),"+
    "index ROUTER_NUMBER_IDX(ROUTER_NUMBER)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists SA_GPIS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "ROUTER_NUMBER int not null,"+
    "SOURCE_NUMBER int not null,"
    "GPI_ID int not null,"+
    "NAME char(32),"+
    "index HOST_ADDRESS_IDX(HOST_ADDRESS,SLOT),"+
    "index ROUTER_IDX(ROUTER_NUMBER))"+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists SA_GPOS (")+
    "ID int auto_increment not null primary key,"+
    "HOST_ADDRESS char(15) not null,"+
    "SLOT int not null,"+
    "ROUTER_NUMBER int not null,"+
    "SOURCE_NUMBER int not null,"
    "GPO_ID int not null,"+
    "NAME char(32),"+
    "SOURCE_ADDRESS char(22),"+
    "SOURCE_SLOT int default -1,"+
    "index HOST_ADDRESS_IDX(HOST_ADDRESS,SLOT),"+
    "index ROUTER_IDX(ROUTER_NUMBER),"+
    "index ROUTER_SOURCE_IDX(ROUTER_NUMBER,SOURCE_NUMBER))"+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;

  sql=QString("create table if not exists TETHER (")+
    "IS_ACTIVE enum('N','Y') not null default 'N') "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  q=new QSqlQuery(sql);
  delete q;
  sql=QString("insert into TETHER set IS_ACTIVE='N'");
  q=new QSqlQuery(sql);
  delete q;

  return true;
}


bool DRouter::StartLivewire(QString *err_msg)
{
  //
  // Livewire Advertisement Sockets
  //
  QSignalMapper *mapper=new QSignalMapper(this);
  connect(mapper,SIGNAL(mapped(int)),this,SLOT(advtReadyReadData(int)));
  SyInterfaces *ifaces=new SyInterfaces();
  if(!ifaces->update()) {
    syslog(LOG_ERR,"unable to get network interface information, aborting");
    exit(1);
  }
  for(int i=0;i<ifaces->quantity();i++) {
    drouter_advt_sockets.
      push_back(new SyMcastSocket(SyMcastSocket::ReadOnly,this));
    if(!drouter_advt_sockets.back()->
       bind(ifaces->ipv4Address(i),SWITCHYARD_ADVERTS_PORT)) {
      syslog(LOG_ERR,"unable to bind %s:%d, aborting",
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

  return true;
}


void DRouter::LockTables() const
{
  QString sql=QString("lock tables ")+
    "DESTINATIONS write,"+
    "GPIS write,"+
    "GPOS write,"+
    "NODES write,"+
    "SA_DESTINATIONS write,"+
    "SA_GPIS write,"+
    "SA_GPOS write,"+
    "SA_SOURCES write,"+
    "SOURCES write";
  QSqlQuery *q=new QSqlQuery(sql);
  delete q;
}


void DRouter::UnlockTables() const
{
  QString sql=QString("unlock tables");
  QSqlQuery *q=new QSqlQuery(sql);
  delete q;
}


void DRouter::LoadMaps()
{
  //
  // Load New Maps
  //
  QStringList msgs;
  if(!EndPointMap::loadSet(&drouter_maps,&msgs)) {
    syslog(LOG_ERR,"SA map load error: %s\n",(const char *)msgs.join("\n").toUtf8());
    exit(1);
  }
  for(int i=0;i<msgs.size();i++) {
    syslog(LOG_DEBUG,"%s",(const char *)msgs.at(i).toUtf8());
  }
  syslog(LOG_INFO,"loaded %d SA map(s)",drouter_maps.size());
}


void DRouter::SendProtoSocket(int dest_sock,int proto_sock)
{
  struct msghdr msg;
  struct cmsghdr *cmsg=NULL;
  char buf[CMSG_SPACE(sizeof &proto_sock)];
  int *fdptr;

  memset(&msg,0,sizeof(msg));
  msg.msg_control=buf;
  msg.msg_controllen=sizeof(buf);
  cmsg=CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level=SOL_SOCKET;
  cmsg->cmsg_type=SCM_RIGHTS;
  cmsg->cmsg_len=CMSG_LEN(sizeof(int));
  fdptr=(int *)CMSG_DATA(cmsg);
  memcpy(fdptr,&proto_sock,sizeof(int));
  msg.msg_controllen=cmsg->cmsg_len;

  if(sendmsg(dest_sock,&msg,0)<0) {
    syslog(LOG_ERR,"error sending protocol socket [%s]",strerror(errno));
    exit(1);
  }
}


void DRouter::Log(int prio,const QString &msg) const
{
  if(prio>=0) {
    syslog(prio,msg.toUtf8());
  }
}
