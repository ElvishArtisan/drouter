// drouter.cpp
//
// Dynamic router database component for Drouter
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
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
#include <QVariant>

#include <sy5/syconfig.h>
#include <sy5/syinterfaces.h>

#include "matrix_factory.h"
#include "drouter.h"
#include "protoipc.h"
#include "sqlquery.h"

DRouter::DRouter(int *proto_socks,QObject *parent)
  : QObject(parent)
{
  drouter_proto_socks=proto_socks;
  drouter_writeable=false;

  drouter_config=new Config();
  drouter_config->load();

  drouter_flasher=new GpioFlasher(this);

  drouter_finalize_timer=new QTimer(this);
  drouter_finalize_timer->setSingleShot(false);
  connect(drouter_finalize_timer,SIGNAL(timeout()),
	  this,SLOT(finalizeEventsData()));

  drouter_purge_events_timer=new QTimer(this);
  connect(drouter_purge_events_timer,SIGNAL(timeout()),
	  this,SLOT(purgeEventsData()));
  if(drouter_config->retainEventRecordsDuration()>0) {
    drouter_purge_events_timer->
      start(drouter_config->retainEventRecordsDuration());
  }
}


DRouter::~DRouter()
{
  WriteCommentEvent(tr("Stopping Drouter service"));
}


QList<QHostAddress> DRouter::nodeHostAddresses() const
{
  QList<QHostAddress> addrs;

  for(QMap<unsigned,Matrix *>::const_iterator it=drouter_nodes.constBegin();
      it!=drouter_nodes.constEnd();it++) {
    if(it.value()->isConnected()) {
      addrs.push_back(it.value()->hostAddress());
    }
  }
  return addrs;
}


Matrix *DRouter::node(const QHostAddress &hostaddr)
{
  try {
    return drouter_nodes.value(hostaddr.toIPv4Address());
  }
  catch(...) {
    return NULL;
  }
}


Matrix *DRouter::nodeBySrcStream(const QHostAddress &strmaddress,
				       int *slot)
{
  if((0xFFFF&strmaddress.toIPv4Address())==0) {
    return NULL;
  }
  for(QMap<unsigned,Matrix *>::const_iterator it=drouter_nodes.begin();
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
  for(QMap<unsigned,Matrix *>::const_iterator it=drouter_nodes.constBegin();
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
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->srcSlots()) {
      return lwrp->src(slot);
    }
  }
  return NULL;
}


SyDestination *DRouter::dst(const QHostAddress &hostaddr,int slot) const
{
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
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
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    return lwrp->clipAlarmActive(slot,type,chan);
  }
  return false;
}


bool DRouter::silenceAlarmActive(const QHostAddress &hostaddr,int slot,
				 SyLwrpClient::MeterType type,int chan) const
{
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    return lwrp->silenceAlarmActive(slot,type,chan);
  }
  return false;
}


SyGpioBundle *DRouter::gpi(const QHostAddress &hostaddr,int slot) const
{
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
  if(lwrp!=NULL) {
    if(slot<(int)lwrp->gpis()) {
      return lwrp->gpiBundle(slot);
    }
  }
  return NULL;
}


SyGpo *DRouter::gpo(const QHostAddress &hostaddr,int slot) const
{
  Matrix *lwrp=drouter_nodes.value(hostaddr.toIPv4Address());
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
  if(!StartStaticMatrices(err_msg)) {
    return false;
  }
  if(!StartLivewire(err_msg)) {
    return false;
  }
  WriteCommentEvent(tr("Started drouter service"));

  return true;
}


bool DRouter::isWriteable() const
{
  return drouter_writeable;
}


void DRouter::setWriteable(bool state)
{
  QString sql;
  QString comment;

  if(drouter_writeable!=state) {
    drouter_flasher->setActive(state);

    //
    // Update Protocols
    //
    QString letter;
    if(state) {
      letter="Y";
      drouter_finalize_timer->start(1000);
      comment=tr("This instance is now active.");
    }
    else {
      letter="N";
      drouter_finalize_timer->stop();
      comment=tr("This instance is no longer active.");
    }
    sql=QString("update `TETHER` set `IS_ACTIVE`='"+letter+"'");
    SqlQuery::apply(sql);
    drouter_writeable=state;
    NotifyProtocols("TETHER",letter);

    WriteCommentEvent(comment);
  }
}


void DRouter::nodeConnectedData(unsigned id,bool state)
{
  printf("nodeConnectedData(%u,%u)\n",id,state);
  QString sql;
  SqlQuery *q;
  int endpt;
  int last_id=0;

  if(state) {
    if(node(QHostAddress(id))==NULL) {
      syslog(LOG_ERR,"DRouter::nodeConnectedData() - received connect signal from unknown node, aborting");
      exit(256);
    }
    Matrix *mtx=node(QHostAddress(id));
    Log(drouter_config->nodeLogPriority(),
	"node connected from "+QHostAddress(id).toString()+
	" ["+mtx->hostName()+" / "+mtx->deviceName()+"]");
    if(drouter_config->configureAudioAlarms(mtx->deviceName())) {
      for(unsigned i=0;i<mtx->srcSlots();i++) {
	if(mtx->src(i)->exists()) {
	  if((drouter_config->clipAlarmThreshold()<0)&&
	     (drouter_config->clipAlarmTimeout()>0)) {
	    mtx->setClipMonitor(i,SyLwrpClient::InputMeter,
				 drouter_config->clipAlarmThreshold(),
				 drouter_config->clipAlarmTimeout());
	  }
	  if((drouter_config->silenceAlarmThreshold()<0)&&
	     (drouter_config->silenceAlarmTimeout()>0)) {
	    mtx->setSilenceMonitor(i,SyLwrpClient::InputMeter,
				    drouter_config->silenceAlarmThreshold(),
				    drouter_config->silenceAlarmTimeout());
	  }
	}
      }
      for(unsigned i=0;i<mtx->dstSlots();i++) {
	if(mtx->dst(i)->exists()) {
	  if((drouter_config->clipAlarmThreshold()<0)&&
	     (drouter_config->clipAlarmTimeout()>0)) {
	    mtx->setClipMonitor(i,SyLwrpClient::OutputMeter,
				 drouter_config->clipAlarmThreshold(),
				 drouter_config->clipAlarmTimeout());
	  }
	  if((drouter_config->silenceAlarmThreshold()<0)&&
	     (drouter_config->silenceAlarmTimeout()>0)) {
	    mtx->setSilenceMonitor(i,SyLwrpClient::OutputMeter,
				    drouter_config->silenceAlarmThreshold(),
				    drouter_config->silenceAlarmTimeout());
	  }
	}
      }
    }
    LockTables();
    sql=QString("insert into `NODES` set ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
      "`HOST_NAME`='"+SqlQuery::escape(mtx->hostName())+"',"+
      "`DEVICE_NAME`='"+SqlQuery::escape(mtx->deviceName())+"',"+
      QString::asprintf("`SOURCE_SLOTS`=%u,",mtx->srcSlots())+
      QString::asprintf("`DESTINATION_SLOTS`=%u,",mtx->dstSlots())+
      QString::asprintf("`GPI_SLOTS`=%u,",mtx->gpis())+
      QString::asprintf("`GPO_SLOTS`=%u",mtx->gpos());
    SqlQuery::apply(sql);
    for(unsigned i=0;i<mtx->srcSlots();i++) {
      sql=QString("insert into `SOURCES` set ")+
	"`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	QString::asprintf("`SLOT`=%u,",i)+
	"`HOST_NAME`='"+SqlQuery::escape(mtx->hostName())+"',"+
	"`STREAM_ADDRESS`='"+
	Config::normalizedStreamAddress(mtx->srcAddress(i)).toString()+"',"+
	"`NAME`='"+SqlQuery::escape(mtx->srcName(i))+"',"+
	QString::asprintf("`STREAM_ENABLED`=%u,",mtx->srcEnabled(i))+
	QString::asprintf("`CHANNELS`=%u,",mtx->srcChannels(i))+
	QString::asprintf("`BLOCK_SIZE`=%u",mtx->srcPacketSize(i));
      last_id=SqlQuery::run(sql).toInt();
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::AudioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Input,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into `SA_SOURCES` set ")+
	      QString::asprintf("`ROUTER_NUMBER`=%d,",it.value()->routerNumber())+
	      QString::asprintf("`SOURCE_NUMBER`=%d,",endpt)+
	      QString::asprintf("`SOURCE_ID`=%d,",last_id)+
	      "`STREAM_ADDRESS`='"+
	      Config::normalizedStreamAddress(mtx->srcAddress(i)).toString()+"',"+
	      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	      QString::asprintf("SLOT=%d",i);
	    // *************************
	    if(!it.value()->nameIsCustom(EndPointMap::Input,endpt)) {
	      it.value()->setName(EndPointMap::Input,endpt,mtx->srcName(i));
	    }
	    // *************************
	    sql+=",`NAME`='"+
	      SqlQuery::escape(it.value()->name(EndPointMap::Input,endpt))+"'";
	    SqlQuery::apply(sql);
	  }
	}
      }
    }
    for(unsigned i=0;i<mtx->dstSlots();i++) {
      sql=QString("insert into `DESTINATIONS` set ")+
	"`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	QString::asprintf("`SLOT`=%u,",i)+
	"`HOST_NAME`='"+SqlQuery::escape(mtx->hostName())+"',"+
	"`STREAM_ADDRESS`='"+
	Config::normalizedStreamAddress(mtx->dstAddress(i)).toString()+"',"+
	"`NAME`='"+SqlQuery::escape(mtx->dstName(i))+"',"+
	QString::asprintf("`CHANNELS`=%u",mtx->dstChannels(i));
      last_id=SqlQuery::run(sql).toInt();
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::AudioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Output,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into `SA_DESTINATIONS` set ")+
	      QString::asprintf("`ROUTER_NUMBER`=%d,",it.value()->routerNumber())+
	      QString::asprintf("`SOURCE_NUMBER`=%d,",endpt)+
	      QString::asprintf("`DESTINATION_ID`=%d,",last_id)+
	      "`STREAM_ADDRESS`='"+
	      Config::normalizedStreamAddress(mtx->dstAddress(i)).toString()+"',"+
	      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	      QString::asprintf("`SLOT`=%d",i);
	    if(!it.value()->nameIsCustom(EndPointMap::Output,endpt)) {
	      it.value()->setName(EndPointMap::Output,endpt,mtx->dstName(i));
	    }
	    sql+=",`NAME`='"+
	      SqlQuery::escape(it.value()->name(EndPointMap::Output,endpt))+"'";
	    SqlQuery::apply(sql);
	  }
	}
      }
    }
    for(unsigned i=0;i<mtx->gpis();i++) {
      sql=QString("insert into `GPIS` set ")+
	"`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	QString::asprintf("`SLOT`=%u,",i)+
	"`HOST_NAME`='"+SqlQuery::escape(mtx->hostName())+"',"+
	"`CODE`='"+mtx->gpiBundle(i)->code()+"'";
      last_id=SqlQuery::run(sql).toInt();
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::GpioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Input,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into `SA_GPIS` set ")+
	      QString::asprintf("`ROUTER_NUMBER`=%d,",it.value()->routerNumber())+
	      QString::asprintf("`SOURCE_NUMBER`=%d,",endpt)+
	      QString::asprintf("`GPI_ID`=%d,",last_id)+
	      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	      QString::asprintf("`SLOT`=%d",i);
	    if(!it.value()->nameIsCustom(EndPointMap::Input,endpt)) {
	      it.value()->setName(EndPointMap::Input,endpt,QString::asprintf("GPI-%d",i+1));
	    }
	    sql+=",`NAME`='"+
	      SqlQuery::escape(it.value()->name(EndPointMap::Input,endpt))+"'";
	    SqlQuery::run(sql);
	  }
	}
      }
    }
    for(unsigned i=0;i<mtx->gpos();i++) {
      QString name=mtx->gpo(i)->name();
      if(name.isEmpty()) {
	name=QString::asprintf("GPO %d",i+1);
      }
      sql=QString("insert into `GPOS` set ")+
	"`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	QString::asprintf("`SLOT`=%u,",i)+
	"`HOST_NAME`='"+SqlQuery::escape(mtx->hostName())+"',"+
	"`CODE`='"+mtx->gpiBundle(i)->code()+"',"+
	"`NAME`='"+SqlQuery::escape(name)+"',"+
	"`SOURCE_ADDRESS`='"+mtx->gpo(i)->sourceAddress().toString()+"',"+
	QString::asprintf("`SOURCE_SLOT`=%d",mtx->gpo(i)->sourceSlot());
      last_id=SqlQuery::run(sql).toInt();
      for(QMap<int,EndPointMap *>::const_iterator it=drouter_maps.begin();it!=drouter_maps.end();it++) {
	if(it.value()->routerType()==EndPointMap::GpioRouter) {
	  if((endpt=it.value()->endPoint(EndPointMap::Output,QHostAddress(id).toString(),i))>=0) {
	    sql=QString("insert into `SA_GPOS` set ")+
	      QString::asprintf("`ROUTER_NUMBER`=%d,",it.value()->routerNumber())+
	      QString::asprintf("`SOURCE_NUMBER`=%d,",endpt)+
	      QString::asprintf("`GPO_ID`=%d,",last_id)+
	      "`SOURCE_ADDRESS`='"+mtx->gpo(i)->sourceAddress().toString()+"',"+
	      QString::asprintf("`SOURCE_SLOT`=%d,",mtx->gpo(i)->sourceSlot())+
	      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"',"+
	      QString::asprintf("`SLOT`=%d",i);
	    if(!it.value()->nameIsCustom(EndPointMap::Output,endpt)) {
	      it.value()->
		setName(EndPointMap::Output,endpt,mtx->gpo(i)->name());
	    }
	    sql+=",`NAME`='"+
	      SqlQuery::escape(it.value()->name(EndPointMap::Output,endpt))+"'";
	    SqlQuery::apply(sql);
	  }
	}
      }
    }

    for(int i=0;i<2;i++) {
      Config::TetherRole role=(Config::TetherRole)i;
      if(mtx->hostAddress()==drouter_config->tetherGpioIpAddress(role)) {
	/*  FIXME!
	drouter_flasher->
	  addGpio(role,mtx,drouter_config->tetherGpioType(role),
		  drouter_config->tetherGpioSlot(role),
		  drouter_config->tetherGpioCode(role));
	*/
      }
    }

    UnlockTables();

    //
    // Send Startup LWRP
    //
    QStringList lwrp_lines=
      drouter_config->nodesStartupLwrp(mtx->hostAddress());
    for(int i=0;i<lwrp_lines.size();i++) {
      mtx->sendRawLwrp(lwrp_lines.at(i));
      syslog(LOG_DEBUG,"sending \"%s\" to node at %s",
	     lwrp_lines.at(i).toUtf8().constData(),
	     mtx->hostAddress().toString().toUtf8().constData());
    }

    NotifyProtocols("NODEADD",QHostAddress(id).toString());
  }
  else {
    printf("HostAddress: %s\n",QHostAddress(id).toString().toUtf8().constData());
    Matrix *mtx=node(QHostAddress(id));
    if(mtx==NULL) {
      syslog(LOG_ERR,"DRouter::nodeConnectedData() - received disconnect signal from unknown node, aborting");
      exit(256);
    }
    Log(drouter_config->nodeLogPriority(),
	"node disconnected from "+QHostAddress(id).toString()+
	" ["+mtx->hostName()+" / "+mtx->deviceName()+"]");
    LockTables();
    sql=QString("select ")+
      "`SOURCE_SLOTS`,"+       // 00
      "`DESTINATION_SLOTS`,"+  // 01
      "`GPI_SLOTS`,"+          // 02
      "`GPO_SLOTS` "+          // 03
      "from `NODES` where "+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    q=new SqlQuery(sql);
    if(q->first()) {
      NotifyProtocols("NODEDEL",QHostAddress(id).toString(),
		      q->value(0).toInt(),q->value(1).toInt(),
		      q->value(2).toInt(),q->value(3).toInt());
    }
    delete q;
    sql=QString("delete from `SA_SOURCES` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `SOURCES` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `SA_DESTINATIONS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `DESTINATIONS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `SA_GPIS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `GPIS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `SA_GPOS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `GPOS` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    sql=QString("delete from `NODES` where ")+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"'";
    SqlQuery::apply(sql);
    UnlockTables();
  }
}


void DRouter::sourceChangedData(unsigned id,int slotnum,const SyNode &node,
				const SySource &src)
{
  QString sql;

  sql=QString("update `SOURCES` set ")+
    "`HOST_NAME`='"+SqlQuery::escape(node.hostName())+"',"+
    "`STREAM_ADDRESS`='"+
    Config::normalizedStreamAddress(src.streamAddress()).toString()+"',"+
    "`NAME`='"+SqlQuery::escape(src.name())+"',"+
    QString::asprintf("`STREAM_ENABLED`=%u,",src.enabled())+
    QString::asprintf("`CHANNELS`=%u,",src.channels())+
    QString::asprintf("`BLOCK_SIZE`=%u where ",src.packetSize())+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  sql=QString("update `SA_SOURCES` set ")+
    "`STREAM_ADDRESS`='"+
    Config::normalizedStreamAddress(src.streamAddress()).toString()+"' where "+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  NotifyProtocols("SRC",QHostAddress(id).toString()+
		  QString::asprintf(":%u",slotnum));
}


void DRouter::destinationChangedData(unsigned id,int slotnum,const SyNode &node,
				     const SyDestination &dst)
{
  QString sql;
  SqlQuery *q;
  bool xpoint_changed=false;

  sql=QString("select ")+
    "`STREAM_ADDRESS` from `DESTINATIONS` where "
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%d",slotnum);
  q=new SqlQuery(sql);
  if(q->first()) {
    xpoint_changed=QHostAddress(q->value(0).toString())!=dst.streamAddress();
  }
  delete q;

  sql=QString("update `DESTINATIONS` set ")+
    "`HOST_NAME`='"+SqlQuery::escape(node.hostName())+"',"+
    "`STREAM_ADDRESS`='"+
    Config::normalizedStreamAddress(dst.streamAddress()).toString()+"',"+
    "`NAME`='"+SqlQuery::escape(dst.name())+"',"+
    QString::asprintf("`CHANNELS`=%u where ",dst.channels())+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  sql=QString("update `SA_DESTINATIONS` set ")+
    "`STREAM_ADDRESS`='"+
    Config::normalizedStreamAddress(dst.streamAddress()).toString()+"' where "+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  if(xpoint_changed) {
    NotifyProtocols("DSTX",QHostAddress(id).toString()+
		    QString::asprintf(":%u",slotnum));
  }
  NotifyProtocols("DST",QHostAddress(id).toString()+
		  QString::asprintf(":%u",slotnum));
}


void DRouter::gpiChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpioBundle &gpi)
{
  QString sql;
  SqlQuery *q;
  bool code_changed=false;

  sql=QString("select ")+
    "`CODE` "+            // 00
    "from `GPIS` where "+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%d",slotnum);
  q=new SqlQuery(sql);
  if(q->first()) {
    code_changed=q->value(0).toString().toLower()!=gpi.code().toLower();
  }
  delete q;

  sql=QString("update `GPIS` set ")+
    "`CODE`='"+gpi.code().toLower()+"' where "+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  if(code_changed) {
    NotifyProtocols("GPICODE",QHostAddress(id).toString()+
		    QString::asprintf(":%u",slotnum));
  }
  NotifyProtocols("GPI",QHostAddress(id).toString()+
		  QString::asprintf(":%u",slotnum));
}


void DRouter::gpoChangedData(unsigned id,int slotnum,const SyNode &node,
			     const SyGpo &gpo)
{
  QString sql;
  SqlQuery *q;
  bool xpoint_changed=false;
  bool code_changed=false;

  sql=QString("select ")+
    "`SOURCE_ADDRESS`,"+  // 00
    "`SOURCE_SLOT`,"+     // 01
    "`CODE` "+            // 02
    "from `GPOS` where "+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%d",slotnum);
  q=new SqlQuery(sql);
  if(q->first()) {
    xpoint_changed=(QHostAddress(q->value(0).toString())!=gpo.sourceAddress()||
		    q->value(1).toInt()!=gpo.sourceSlot());
    code_changed=q->value(2).toString().toLower()!=gpo.bundle()->code().toLower();
  }
  delete q;

  sql=QString("update `GPOS` set ")+
    "`CODE`='"+gpo.bundle()->code().toLower()+"',"+
    "`NAME`='"+SqlQuery::escape(gpo.name())+"',"+
    "`SOURCE_ADDRESS`='"+gpo.sourceAddress().toString()+"',"+
    QString::asprintf("`SOURCE_SLOT`=%d where ",gpo.sourceSlot())+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  sql=QString("update `SA_GPOS` set ")+
    "`SOURCE_ADDRESS`='"+gpo.sourceAddress().toString()+"',"+
    QString::asprintf("`SOURCE_SLOT`=%d where ",gpo.sourceSlot())+
    "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
    QString::asprintf("`SLOT`=%u",slotnum);
  SqlQuery::apply(sql);
  if(xpoint_changed) {
    NotifyProtocols("GPOX",QHostAddress(id).toString()+
		    QString::asprintf(":%u",slotnum));
  }
  if(code_changed) {
    NotifyProtocols("GPOCODE",QHostAddress(id).toString()+
		    QString::asprintf(":%u",slotnum));
  }
  NotifyProtocols("GPO",QHostAddress(id).toString()+
		  QString::asprintf(":%u",slotnum));
}


void DRouter::audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
				 unsigned slotnum,int chan,bool state)
{
  QString sql;
  Matrix *lwrp=NULL;
  QString table="SOURCES";
  QString chan_name="LEFT";

  if(type==SyLwrpClient::OutputMeter) {
    table="DESTINATIONS";
  }
  if(chan==1) {
    chan_name="RIGHT";
  }
  if((lwrp=drouter_nodes[id])!=NULL) {
    sql=QString("update `")+table+"` set "+
      "`"+chan_name+"_CLIP`="+QString::asprintf("%d where ",state)+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
      QString::asprintf("`SLOT`=%d",slotnum);
    SqlQuery::apply(sql);
    NotifyProtocols("CLIP",QString::asprintf("%d:%d:",type,chan)+QHostAddress(id).toString()+QString::asprintf(":%d",slotnum));
    //    emit clipAlarmChanged(*(lwrp->node()),(int)slotnum,type,chan,state);
  }
}


void DRouter::audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
				    unsigned slotnum,int chan,bool state)
{
  QString sql;
  Matrix *lwrp=NULL;
  QString table="SOURCES";
  QString chan_name="LEFT";

  if(type==SyLwrpClient::OutputMeter) {
    table="DESTINATIONS";
  }
  if(chan==1) {
    chan_name="RIGHT";
  }

  if((lwrp=drouter_nodes[id])!=NULL) {
    sql=QString("update `")+table+"` set "+
      "`"+chan_name+"_SILENCE`="+QString::asprintf("%d where ",state)+
      "`HOST_ADDRESS`='"+QHostAddress(id).toString()+"' && "+
      QString::asprintf("`SLOT`=%d",slotnum);
    SqlQuery::apply(sql);
    NotifyProtocols("SILENCE",QString::asprintf("%d:%d:",type,chan)+QHostAddress(id).toString()+QString::asprintf(":%d",slotnum));
  }
}


void DRouter::advtReadyReadData(int ifnum)
{
  QHostAddress addr;
  char data[1501];
  int n;

  while((n=drouter_advt_sockets.at(ifnum)->readDatagram(data,1500,&addr))>0) {
    if(node(addr)==NULL) {
      Matrix *mtx=StartMatrix(Config::LwrpMatrix,addr.toIPv4Address());
      if(mtx==NULL) {
	syslog(LOG_WARNING,
	       "failed to initialize matrix client for LWRP node at %s",
	       addr.toString().toUtf8().constData());
      }
      else {
	mtx->connectToHost(addr,SWITCHYARD_LWRP_PORT,
			   drouter_config->lwrpPassword(),false);
      }
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


void DRouter::finalizeEventsData()
{
  QString sql;
  SqlQuery *q;
  EndPointMap *map=NULL;

  sql=QString("select ")+
    "`ID`,"+                  // 00
    "`ROUTER_NUMBER`,"+       // 01
    "`DESTINATION_NUMBER`,"+  // 02
    "`SOURCE_NUMBER` "+       // 03
    "from `PERM_SA_EVENTS` where "+
    "`STATUS`='O' && "+
    "`DATETIME`<'"+QDateTime::currentDateTime().addSecs(-1).
    toString("yyyy-MM-dd hh:mm:ss")+"'";
  q=new SqlQuery(sql);
  while(q->next()) {
    if((map=drouter_maps.value(q->value(1).toInt()))==NULL) {
      FinalizeSARouteEvent(q->value(0).toInt(),false);
    }
    else {
      switch(drouter_maps.value(q->value(1).toInt())->routerType()) {
      case EndPointMap::AudioRouter:
	FinalizeSAAudioRoute(q->value(0).toInt(),q->value(1).toInt(),
			     q->value(2).toInt(),q->value(3).toInt());
	break;

      case EndPointMap::GpioRouter:
	FinalizeSAGpioRoute(q->value(0).toInt(),q->value(1).toInt(),
			    q->value(2).toInt(),q->value(3).toInt());
	break;

      case EndPointMap::LastRouter:
	break;
      }
    }
  }
  delete q;
}


void DRouter::purgeEventsData()
{
  QString sql=QString("delete from `PERM_SA_EVENTS` where ")+
    "`DATETIME`<'"+QDateTime::currentDateTime().
    addSecs(-3600*drouter_config->retainEventRecordsDuration()).
    toString("yyyy-MM-dd hh:mm:ss")+"'";
  SqlQuery::apply(sql);
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
			 QString::asprintf(":%d:%d:%d:%d\r\n",
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
  unlink(DROUTER_IPC_ADDRESS);
  if((sock=socket(AF_UNIX,SOCK_SEQPACKET,0))<0) {
    *err_msg=tr("unable to start protocol ipc")+" ["+strerror(errno)+"]";
    return false;
  }
  memset(&sa,0,sizeof(sa));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path,DROUTER_IPC_ADDRESS,UNIX_PATH_MAX-1);
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

  syslog(LOG_DEBUG,"received proto->core IPC cmd: \"%s\" from connection %d",
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
    Matrix *lwrp=drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned slotnum=cmds.at(2).toUInt(&ok);
    if((lwrp!=NULL)&&ok&&(slotnum<lwrp->dstSlots())) {
      lwrp->setDstAddress(slotnum,QHostAddress(DROUTER_NULL_STREAM_ADDRESS));
    }
  }

  if((cmds.at(0)=="ClearGpioCrosspoint")&&(cmds.size()==3)) {
    Matrix *lwrp=drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned slotnum=cmds.at(2).toUInt(&ok);
    if((lwrp!=NULL)&&ok&&(slotnum<lwrp->gpos())) {
      lwrp->setGpoSourceAddress(slotnum,QHostAddress(),-1);
    }
  }

  if((cmds.at(0)=="SetCrosspoint")&&(cmds.size()==5)) {
    Matrix *dst_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned dst_slotnum=cmds.at(2).toUInt(&ok);
    if((dst_lwrp!=NULL)&&ok&&(dst_slotnum<dst_lwrp->dstSlots())) {
      Matrix *src_lwrp=
	drouter_nodes[QHostAddress(cmds.at(3)).toIPv4Address()];
      unsigned src_slotnum=cmds.at(4).toUInt(&ok);
      if((src_lwrp!=NULL)&&ok&&(src_slotnum<src_lwrp->srcSlots())) {
	dst_lwrp->setDstAddress(dst_slotnum,src_lwrp->srcAddress(src_slotnum));
      }
    }
  }

  if((cmds.at(0)=="SetGpioCrosspoint")&&(cmds.size()==5)) {
    Matrix *gpo_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned gpo_slotnum=cmds.at(2).toUInt(&ok);
    if((gpo_lwrp!=NULL)&&ok&&(gpo_slotnum<gpo_lwrp->gpos())) {
      Matrix *gpi_lwrp=
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
    Matrix *gpi_lwrp=
      drouter_nodes[QHostAddress(cmds.at(1)).toIPv4Address()];
    unsigned gpi_slotnum=cmds.at(2).toUInt(&ok);
    if((gpi_lwrp!=NULL)&&ok&&(gpi_slotnum<gpi_lwrp->gpis())) {
      gpi_lwrp->setGpiCode(gpi_slotnum,cmds.at(3));
    }
  }

  if((cmds.at(0)=="SetGpoState")&&(cmds.size()==4)) {
    Matrix *gpo_lwrp=
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
  SqlQuery *q;
  SqlQuery *q1;
  int schema_ver=0;

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
  sql=QString::asprintf("set max_heap_table_size=%d",
			drouter_config->maxHeapTableSize());
  SqlQuery::apply(sql);

  //
  // Clear Old Data
  //
  sql="show tables";
  q=new SqlQuery(sql);
  while(q->next()) {
    if(q->value(0).toString()=="PERM_VERSION") {
      schema_ver=1;
      sql=QString("select ")+
	"`DB` "+  // 00
	"from `PERM_VERSION`";
      q1=new SqlQuery(sql);
      if(q1->first()) {
	schema_ver=q1->value(0).toInt();
      }
      delete q1;
    }
    if(q->value(0).toString().left(5)!="PERM_") {
      sql=QString("drop table `")+q->value(0).toString()+"`";
      SqlQuery::apply(sql);
    }
  }
  delete q;

  //
  // Permanent Tables
  //
  if(schema_ver<1) {
    sql=QString("create table if not exists `PERM_VERSION` (")+
      "`DB` int not null primary key"+
      ") "+
      "engine InnoDB character set utf8 collate utf8_general_ci";
    SqlQuery::apply(sql);

    schema_ver=1;
    sql=QString("insert into `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  if(schema_ver<2) {
    sql=QString("create table if not exists `PERM_SA_EVENTS` (")+
      "`ID` int auto_increment not null primary key,"+
      "`DATETIME` datetime not null,"+
      "`STATUS` enum('O','N','Y') not null default 'O',"+
      "`ORIGINATING_ADDRESS` varchar(22) not null,"+
      "`ROUTER_NUMBER` int not null,"+
      "`SOURCE_NUMBER` int not null,"+
      "`DESTINATION_NUMBER` int not null,"+
      "index STATUS_IDX(`STATUS`)) "+
      "engine InnoDB character set utf8 collate utf8_general_ci";
    SqlQuery::apply(sql);

    schema_ver=2;
    sql=QString("update `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  if(schema_ver<3) {
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `USERNAME` varchar(32) after `STATUS`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `HOSTNAME` varchar(32) after `USERNAME`";
    SqlQuery::apply(sql);

    schema_ver=3;
    sql=QString("update `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  if(schema_ver<4) {
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `TYPE` enum('C','R') after `ID`";
    SqlQuery::apply(sql);
    sql=QString("update `PERM_SA_EVENTS` set ")+
      "`TYPE`='R'";
    SqlQuery::apply(sql);
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `TYPE` enum('C','R') not null after `ID`";
    SqlQuery::apply(sql);
    
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `COMMENT` text after `DESTINATION_NUMBER`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `ORIGINATING_ADDRESS` varchar(22) after `HOSTNAME`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `ROUTER_NUMBER` int after `ORIGINATING_ADDRESS`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `SOURCE_NUMBER` int after `ROUTER_NUMBER`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `DESTINATION_NUMBER` int after `SOURCE_NUMBER`";
    SqlQuery::apply(sql);

    schema_ver=4;
    sql=QString("update `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  if(schema_ver<5) {
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `ROUTER_NAME` varchar(191) after `ROUTER_NUMBER`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `SOURCE_NAME` varchar(191) after `SOURCE_NUMBER`";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "add column `DESTINATION_NAME` varchar(191) after `DESTINATION_NUMBER`";
    SqlQuery::apply(sql);

    schema_ver=5;
    sql=QString("update `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  if(schema_ver<6) {
    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `TYPE` enum('C','R','S') not null";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `USERNAME` varchar(191)";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `HOSTNAME` varchar(191)";
    SqlQuery::apply(sql);

    sql=QString("alter table `PERM_SA_EVENTS` ")+
      "modify column `ORIGINATING_ADDRESS` varchar(45)";
    SqlQuery::apply(sql);

    schema_ver=6;
    sql=QString("update `PERM_VERSION` set ")+
      QString::asprintf("`DB`=%d",schema_ver);
    SqlQuery::apply(sql);
    syslog(LOG_DEBUG,"applied schema version %d",schema_ver);
  }

  // New schema updates go here


  syslog(LOG_INFO,"using DB schema version %d",schema_ver);

  //
  // Ephemeral Tables
  //
  sql=QString("create table if not exists `NODES` (")+
    "`HOST_ADDRESS` char(15) not null primary key,"+
    "`HOST_NAME` char(191),"+
    "`DEVICE_NAME` char(20),"+
    "`SOURCE_SLOTS` int,"+
    "`DESTINATION_SLOTS` int,"+
    "`GPI_SLOTS` int,"+
    "`GPO_SLOTS` int) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::run(sql);

  sql=QString("create table if not exists `SOURCES` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`HOST_NAME` char(191),"+
    "`STREAM_ADDRESS` char(15),"+
    "`NAME` char(191),"+
    "`STREAM_ENABLED` int,"+
    "`CHANNELS` int,"+
    "`BLOCK_SIZE` int,"+
    "`LEFT_CLIP` int default 0,"+
    "`RIGHT_CLIP` int default 0,"+
    "`LEFT_SILENCE` int default 0,"+
    "`RIGHT_SILENCE` int default 0,"+
    "unique index SLOT_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index STREAM_ADDRESS_IDX(`STREAM_ADDRESS`,`STREAM_ENABLED`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `DESTINATIONS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`HOST_NAME` char(191),"+
    "`STREAM_ADDRESS` char(15),"+
    "`NAME` char(191),"+
    "`CHANNELS` int,"+
    "`LEFT_CLIP` int default 0,"+
    "`RIGHT_CLIP` int default 0,"+
    "`LEFT_SILENCE` int default 0,"+
    "`RIGHT_SILENCE` int default 0,"+
    "unique index SLOT_IDX(`HOST_ADDRESS`,`SLOT`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `GPIS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`HOST_NAME` char(191),"+
    "`CODE` char(5),"+
    "unique index SLOT_IDX(`HOST_ADDRESS`,`SLOT`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `GPOS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`HOST_NAME` char(191),"+
    "`CODE` char(5),"+
    "`NAME` char(191),"+
    "`SOURCE_ADDRESS` char(22),"+
    "`SOURCE_SLOT` int default -1,"+
    "unique index SLOT_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index SOURCE_ADDRESS_IDX(`SOURCE_ADDRESS`,`SOURCE_SLOT`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `SA_SOURCES` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`ROUTER_NUMBER` int not null,"+
    "`SOURCE_NUMBER` int not null,"
    "`SOURCE_ID` int not null,"+
    "`NAME` char(191),"+
    "`STREAM_ADDRESS` char(15),"+
    "index HOST_ADDRESS_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index STREAM_ADDRESS_IDX(`ROUTER_NUMBER`,`STREAM_ADDRESS`),"+
    "index ROUTER_NUMBER_IDX(`ROUTER_NUMBER`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `SA_DESTINATIONS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`ROUTER_NUMBER` int not null,"+
    "`SOURCE_NUMBER` int not null,"
    "`DESTINATION_ID` int not null,"+
    "`NAME` char(191),"+
    "`STREAM_ADDRESS` char(15),"+
    "index HOST_ADDRESS_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index ROUTER_NUMBER_IDX(`ROUTER_NUMBER`)) "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `SA_GPIS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`ROUTER_NUMBER` int not null,"+
    "`SOURCE_NUMBER` int not null,"
    "`GPI_ID` int not null,"+
    "`NAME` char(191),"+
    "index HOST_ADDRESS_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index ROUTER_IDX(`ROUTER_NUMBER`))"+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `SA_GPOS` (")+
    "`ID` int auto_increment not null primary key,"+
    "`HOST_ADDRESS` char(15) not null,"+
    "`SLOT` int not null,"+
    "`ROUTER_NUMBER` int not null,"+
    "`SOURCE_NUMBER` int not null,"
    "`GPO_ID` int not null,"+
    "`NAME` char(191),"+
    "`SOURCE_ADDRESS` char(22),"+
    "`SOURCE_SLOT` int default -1,"+
    "index HOST_ADDRESS_IDX(`HOST_ADDRESS`,`SLOT`),"+
    "index ROUTER_IDX(`ROUTER_NUMBER`),"+
    "index ROUTER_SOURCE_IDX(`ROUTER_NUMBER`,`SOURCE_NUMBER`))"+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);

  sql=QString("create table if not exists `TETHER` (")+
    "`IS_ACTIVE` enum('N','Y') not null default 'N') "+
    "engine MEMORY character set utf8 collate utf8_general_ci";
  SqlQuery::apply(sql);
  sql=QString("insert into `TETHER` set `IS_ACTIVE`='N'");
  SqlQuery::apply(sql);

  return true;
}


bool DRouter::StartStaticMatrices(QString *err_msg)
{
  for(int i=0;i<drouter_config->matrixQuantity();i++) {
    Matrix *mtx=
      StartMatrix(drouter_config->matrixType(i),
		  drouter_config->matrixHostAddress(i).toIPv4Address());
    if(mtx==NULL) {
      syslog(LOG_WARNING,
	     "static matrix [Matrix%d]: unrecognized device type",i+1);
    }
    else {
      mtx->connectToHost(drouter_config->matrixHostAddress(i),
			 drouter_config->matrixPort(i),"",false);
      drouter_nodes[drouter_config->matrixHostAddress(i).toIPv4Address()]=mtx;
      syslog(LOG_INFO,"Initialized %s device at %s:%u",
	     mtx->deviceName().toUtf8().constData(),
	     drouter_config->matrixHostAddress(i).
	     toString().toUtf8().constData(),
	     0xffff&drouter_config->matrixPort(i));
    }
  }

  return true;
}


bool DRouter::StartLivewire(QString *err_msg)
{
  if(drouter_config->livewireIsEnabled()) {
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
  }
  else {
    Log(LOG_INFO,"Livewire support has been disabled in the configuration");
  }

  return true;
}


Matrix *DRouter::StartMatrix(Config::MatrixType type,unsigned id)
{
  Matrix *mtx=MatrixFactory(type,id,drouter_config,this);
  connect(mtx,SIGNAL(connected(unsigned,bool)),
	  this,SLOT(nodeConnectedData(unsigned,bool)));
  connect(mtx,
	  SIGNAL(sourceChanged(unsigned,int,const SyNode,const SySource &)),
	  this,SLOT(sourceChangedData(unsigned,int,const SyNode,
				      const SySource &)));
  connect(mtx,SIGNAL(destinationChanged(unsigned,int,const SyNode &,
					 const SyDestination &)),
	  this,SLOT(destinationChangedData(unsigned,int,const SyNode &,
					   const SyDestination &)));
  connect(mtx,SIGNAL(gpiChanged(unsigned,int,const SyNode &,
				 const SyGpioBundle &)),
	  this,SLOT(gpiChangedData(unsigned,int,const SyNode &,
				   const SyGpioBundle &)));
  connect(mtx,
	  SIGNAL(gpoChanged(unsigned,int,const SyNode &,const SyGpo &)),
	  this,
	  SLOT(gpoChangedData(unsigned,int,const SyNode &,const SyGpo &)));
  connect(mtx,SIGNAL(audioClipAlarm(unsigned,SyLwrpClient::MeterType,
				     unsigned,int,bool)),
	  this,SLOT(audioClipAlarmData(unsigned,SyLwrpClient::MeterType,
				       unsigned,int,bool)));
  connect(mtx,SIGNAL(audioSilenceAlarm(unsigned,SyLwrpClient::MeterType,
					unsigned,int,bool)),
	  this,SLOT(audioSilenceAlarmData(unsigned,SyLwrpClient::MeterType,
					  unsigned,int,bool)));
  drouter_nodes[id]=mtx;

  return mtx;
}


void DRouter::LockTables() const
{
  QString sql=QString("lock tables ")+
    "`DESTINATIONS` write,"+
    "`GPIS` write,"+
    "`GPOS` write,"+
    "`NODES` write,"+
    "`SA_DESTINATIONS` write,"+
    "`SA_GPIS` write,"+
    "`SA_GPOS` write,"+
    "`SA_SOURCES` write,"+
    "`SOURCES` write";
  SqlQuery::apply(sql);
}


void DRouter::UnlockTables() const
{
  QString sql=QString("unlock tables");
  SqlQuery::apply(sql);
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
    syslog(prio,"%s",msg.toUtf8().constData());
  }
}


void DRouter::FinalizeSAAudioRoute(int event_id,int router,int output,int input)
{
  QString sql;
  SqlQuery *q;

  if(input<0) {  // No route --i.e. destination is "OFF"
    sql=QString("select ")+
      "`STREAM_ADDRESS` "+  // 00
      "from `SA_DESTINATIONS` where "+
      QString::asprintf("`ROUTER_NUMBER`=%d && ",router)+
      QString::asprintf("`SOURCE_NUMBER`=%d",output);
    q=new SqlQuery(sql);
    if(q->first()) {
      FinalizeSARouteEvent(event_id,
			   q->value(0).toString()==DROUTER_NULL_STREAM_ADDRESS);
    }
    else {
      FinalizeSARouteEvent(event_id,false);
    }
    delete q;
  }
  else {
    sql=QString("select ")+
      "`SA_DESTINATIONS`.`ID` "+  // 00
      "from `SA_DESTINATIONS` left join `SA_SOURCES` "+
      "on `SA_DESTINATIONS`.`STREAM_ADDRESS`=`SA_SOURCES`.`STREAM_ADDRESS` && "+
      "`SA_SOURCES`.`ROUTER_NUMBER`=`SA_DESTINATIONS`.`ROUTER_NUMBER` "+
      "where "+
      QString::asprintf("`SA_DESTINATIONS`.`ROUTER_NUMBER`=%d && ",
			router)+
      QString::asprintf("`SA_SOURCES`.`SOURCE_NUMBER`=%d && ",
			input)+
      QString::asprintf("`SA_DESTINATIONS`.`SOURCE_NUMBER`=%d",
			output);
    //printf("finalize SQL: %s\n",sql.toUtf8().constData());
    q=new SqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
}


void DRouter::FinalizeSAGpioRoute(int event_id,int router,int output,int input)
{
  QString sql;
  SqlQuery *q;

  if(input<0) {  // No route --i.e. destination is "OFF"
    sql=QString("select ")+
      "`SOURCE_SLOT` "+  // 00
      "from `SA_GPOS` where "+
      QString::asprintf("`ROUTER_NUMBER`=%d && ",router)+
      QString::asprintf("`SOURCE_NUMBER`=%d && ",output)+
      "`SOURCE_SLOT`<0";
    q=new SqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
  else {
    sql=QString("select ")+
      "`SA_GPOS`.`ID` "+  // 00
      "from `SA_GPOS` left join `SA_GPIS` "+
      "on `SA_GPOS`.`SOURCE_ADDRESS`=`SA_GPIS`.`HOST_ADDRESS` && "+
      "`SA_GPOS`.`SOURCE_SLOT`=`SA_GPIS`.`SLOT` && "+
      "`SA_GPIS`.`ROUTER_NUMBER`=`SA_GPOS`.`ROUTER_NUMBER` "+
      "where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%d && ",
			router)+
      QString::asprintf("`SA_GPIS`.`SOURCE_NUMBER`=%d && ",
			input)+
      QString::asprintf("`SA_GPOS`.`SOURCE_NUMBER`=%d",
			output);
    q=new SqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
}


void DRouter::FinalizeSARouteEvent(int event_id,bool status) const
{
  QString comment="";
  QString sql;
  SqlQuery *q=NULL;
  QString router_name;
  QString input_name;
  QString output_name;

  sql=QString("select ")+
    "`STATUS`,"+              // 00
    "`ROUTER_NUMBER`,"+       // 01
    "`DESTINATION_NUMBER`,"+  // 02
    "`SOURCE_NUMBER`,"+       // 03
    "`USERNAME`,"+            // 04
    "`HOSTNAME`,"+            // 05
    "`ORIGINATING_ADDRESS` "+ // 06
    "from `PERM_SA_EVENTS` where "+
    QString::asprintf("`ID`=%d",event_id);
  q=new SqlQuery(sql);
  if(q->first()) {
    EndPointMap *map=drouter_maps.value(q->value(1).toInt());
    if(map!=NULL) {
      router_name=map->routerName();
      output_name=map->name(EndPointMap::Output,q->value(2).toInt());
      input_name=map->name(EndPointMap::Input,q->value(3).toInt());
    }
  }
  delete q;

  if(status) {
    sql=QString("update `PERM_SA_EVENTS` set ")+
      "`STATUS`='Y',"+
      "`ROUTER_NAME`='"+SqlQuery::escape(router_name)+"',"+
      "`DESTINATION_NAME`='"+SqlQuery::escape(output_name)+"',"+
      "`SOURCE_NAME`='"+SqlQuery::escape(input_name)+"' "+
      QString::asprintf("where `ID`=%d",event_id);
  }
  else {
    sql=QString("update `PERM_SA_EVENTS` set ")+
      "`STATUS`='N',"+
      "`ROUTER_NAME`='"+SqlQuery::escape(router_name)+"',"+
      "`DESTINATION_NAME`='"+SqlQuery::escape(output_name)+"',"+
      "`SOURCE_NAME`='"+SqlQuery::escape(input_name)+"' "+
      QString::asprintf("where `ID`=%d",event_id);
  }
  SqlQuery::apply(sql);
}


void DRouter::WriteCommentEvent(const QString &str) const
{
  QString sql;

  sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`TYPE`='C',"+
    "`DATETIME`=now(),"+
    "`STATUS`='Y',"+
    "`COMMENT`='"+SqlQuery::escape(str)+"'";
  SqlQuery::apply(sql);
}

