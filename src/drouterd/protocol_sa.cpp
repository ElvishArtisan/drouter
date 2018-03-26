// protocol_sa.cpp
//
// Software Authority protocol handler for DRouter.
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

#include <QStringList>

#include <sy/syrouting.h>

#include "protocol_sa.h"

ProtocolSa::ProtocolSa(QObject *parent)
  : Protocol(parent)
{
  int flags;

  proto_socket=NULL;
  proto_destinations_subscribed=false;
  proto_gpis_subscribed=false;
  proto_gpos_subscribed=false;
  proto_nodes_subscribed=false;
  proto_sources_subscribed=false;
  proto_clips_subscribed=false;
  proto_silences_subscribed=false;

  //
  // The ProtocolSa Server
  //
  proto_server=new QTcpServer(this);
  connect(proto_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));
  proto_server->listen(QHostAddress::Any,9500);
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_server->socketDescriptor(),F_SETFD,&flags))<0) {
    fprintf(stderr,"dprotod: socket error [%s]\n",(const char *)strerror(errno));
    exit(1);
  }

  LoadMaps();
  LoadHelp();
}


void ProtocolSa::newConnectionData()
{
  int flags;
  QString err_msg;

  //
  // Process Server Connection
  //
  proto_socket=proto_server->nextPendingConnection();
  if((flags=fcntl(proto_socket->socketDescriptor(),F_GETFD,NULL))<0) {
    fprintf(stderr,"dprotod: socket error [%s]\n",(const char *)strerror(errno));
    exit(1);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_socket->socketDescriptor(),F_SETFD,&flags))<0) {
    fprintf(stderr,"dprotod: socket error [%s]\n",(const char *)strerror(errno));
    exit(1);
  }
  
  if(fork()==0) {
    proto_server->close();
    proto_server=NULL;

    //
    // Start IPC
    //
    if(!startIpc(&err_msg)) {
      proto_socket->
	write(("unable to bind to drouter service ["+err_msg+"]").toUtf8());
      quit();
    }

    //
    // Initialize Connection
    //
    connect(proto_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
    connect(proto_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  }
  else {
    proto_socket->close();
    delete proto_socket;
    proto_socket=NULL;
  }
}


void ProtocolSa::readyReadData()
{
  char data[1501];
  int n;

  while((n=proto_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(0xFF&data[i]) {
      case 10:
	break;

      case 13:
	ProcessCommand(proto_accum);
	proto_accum="";
	break;

      default:
	proto_accum+=0xFF&data[i];
      }
    }
  }
}


void ProtocolSa::disconnectedData()
{
  quit();
}


void ProtocolSa::destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  sql=RouteStatSqlFields(EndPointMap::AudioRouter)+
    "&& DESTINATIONS.HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
    QString().sprintf("DESTINATIONS.SLOT=%d ",slotnum)+
    "order by SA_DESTINATIONS.SOURCE_NUMBER,SA_DESTINATIONS.ROUTER_NUMBER";
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(RouteStatMessage(q).toUtf8());
  }
  delete q;
}


void ProtocolSa::gpiCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  sql=GPIStatSqlFields()+" where "+
    "GPIS.HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
    QString().sprintf("GPIS.SLOT=%d",slotnum);
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPIStatMessage(q).toUtf8());
  }
  delete q;
}


void ProtocolSa::gpoCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  sql=GPOStatSqlFields()+" where "+
    "GPOS.HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
    QString().sprintf("GPOS.SLOT=%d",slotnum);
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPOStatMessage(q).toUtf8());
  }
  delete q;
}


void ProtocolSa::gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  sql=RouteStatSqlFields(EndPointMap::GpioRouter)+
    "GPOS.HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
    QString().sprintf("GPOS.SLOT=%d ",slotnum)+
    "order by SA_GPOS.SOURCE_NUMBER,SA_GPOS.ROUTER_NUMBER";
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(RouteStatMessage(q).toUtf8());
  }
  delete q;
}


void ProtocolSa::ActivateRoute(unsigned router,unsigned output,unsigned input)
{
  EndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    QHostAddress dst_addr=map->hostAddress(EndPointMap::Output,output);
    int dst_slotnum=map->slot(EndPointMap::Output,output);
    if(!dst_addr.isNull()&&(dst_slotnum>=0)) {
      if(input==0) {
	switch(map->routerType()) {
	case EndPointMap::AudioRouter:
	  clearCrosspoint(dst_addr,dst_slotnum);
	  break;

	case EndPointMap::GpioRouter:
	  clearGpioCrosspoint(dst_addr,dst_slotnum);
	  break;

	case EndPointMap::LastRouter:
	  break;
	}
      }
      else {
	QHostAddress src_addr=map->hostAddress(EndPointMap::Input,input-1);
	int src_slotnum=map->slot(EndPointMap::Input,input-1);
	if(!src_addr.isNull()&&(src_slotnum>=0)) {
	  switch(map->routerType()) {
	  case EndPointMap::AudioRouter:
	    setCrosspoint(dst_addr,dst_slotnum,src_addr,src_slotnum);
	    syslog(LOG_INFO,"activated audio route router: %d  input: %d to output: %d from %s",
		   router+1,output+1,input,
		   (const char *)proto_socket->peerAddress().toString().toUtf8());
	    break;
	  
	  case EndPointMap::GpioRouter:
	    setGpioCrosspoint(dst_addr,dst_slotnum,src_addr,src_slotnum);
	    syslog(LOG_INFO,"activated gpio route router: %d  input: %d to output: %d from %s",
		   router+1,output+1,input,
		   (const char *)proto_socket->peerAddress().toString().toUtf8());
	    break;

	  case EndPointMap::LastRouter:
	    break;
	  }
	}
      }
    }
  }
}


void ProtocolSa::TriggerGpi(unsigned router,unsigned input,unsigned msecs,const QString &code)
{
  EndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==EndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(EndPointMap::Input,input);
      int slotnum=map->slot(EndPointMap::Input,input);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpiState(addr,slotnum,code);
	syslog(LOG_INFO,"set gpi state router: %d  input: %d to state: %s from %s",
	       router+1,input+1,
	       (const char *)code.toUtf8(),
	       (const char *)proto_socket->peerAddress().toString().toUtf8());
      }
    }
  }
}


void ProtocolSa::TriggerGpo(unsigned router,unsigned output,unsigned msecs,const QString &code)
{
  EndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==EndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(EndPointMap::Output,output);
      int slotnum=map->slot(EndPointMap::Output,output);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpoState(addr,slotnum,code);
	syslog(LOG_INFO,"set gpo state router: %d  output: %d to state: %s from %s",
	       router+1,output+1,
	       (const char *)code.toUtf8(),
	       (const char *)proto_socket->peerAddress().toString().toUtf8());
      }
    }
  }
}


void ProtocolSa::SendSnapshotNames(unsigned router)
{
  EndPointMap *map=NULL;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    proto_socket->write(">>",2);
    return;
  }
  proto_socket->write(QString().sprintf("Begin SnapshotNames - %u\r\n",router+1).toUtf8());
  for(int i=0;i<map->snapshotQuantity();i++) {
    proto_socket->write(QString("   "+map->snapshot(i)->name()+"\r\n").toUtf8());
  }
  proto_socket->write(QString().sprintf("End SnapshotNames - %u\r\n",router+1).toUtf8());
}


void ProtocolSa::ActivateSnapshot(unsigned router,const QString &snapshot_name)
{
  EndPointMap *map=NULL;
  Snapshot *ss=NULL;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  proto_socket->write(QString("Snapshot Initiated\r\n").toUtf8());
  if((ss=map->snapshot(snapshot_name))!=NULL) {
    for(int i=0;i<ss->routeQuantity();i++) {
      ActivateRoute(router,ss->routeOutput(i)-1,ss->routeInput(i));
    }
  }
  syslog(LOG_INFO,"activated snapshot %d:%s from %s",router+1,
	 (const char *)snapshot_name.toUtf8(),
	 (const char *)proto_socket->peerAddress().toString().toUtf8());
}


void ProtocolSa::SendSourceInfo(unsigned router)
{
  EndPointMap *map;
  QString sql;
  QSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  if(map->routerType()==EndPointMap::AudioRouter) {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString().sprintf("SA_SOURCES.ROUTER_NUMBER=%u ",router)+
      "order by SA_SOURCES.SOURCE_NUMBER";
  }
  else {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString().sprintf("SA_GPIS.ROUTER_NUMBER=%u ",router)+
      "order by SA_GPIS.SOURCE_NUMBER";
  }
  q=new QSqlQuery(sql);
  proto_socket->write(QString().sprintf("Begin SourceNames - %d\r\n",router+1).toUtf8());
  while(q->next()) {
    proto_socket->write(SourceNamesMessage(map->routerType(),q).toUtf8());
  }
  proto_socket->write(QString().sprintf("End SourceNames - %d\r\n",router+1).toUtf8());
}


QString ProtocolSa::SourceNamesSqlFields(EndPointMap::RouterType type) const
{
  if(type==EndPointMap::AudioRouter) {
    return QString("select ")+
      "SA_SOURCES.SOURCE_NUMBER,"+  // 00
      "SOURCES.NAME,"+              // 01
      "SOURCES.HOST_ADDRESS,"+      // 02
      "SOURCES.HOST_NAME,"+         // 03
      "SOURCES.SLOT,"+              // 04
      "SOURCES.STREAM_ADDRESS "+    // 05
      "from "+"SOURCES left join SA_"+"SOURCES on "+"SOURCES.ID=SA_"+"SOURCES."+"SOURCE_ID ";
  }
  return QString("select ")+
    "SA_GPIS.SOURCE_NUMBER,"+  // 00
    "GPIS.HOST_ADDRESS,"+      // 01
    "GPIS.HOST_NAME,"+         // 02
    "GPIS.SLOT "+              // 03
    "from "+"GPIS left join SA_"+"GPIS on "+"GPIS.ID=SA_"+"GPIS."+"GPI_ID ";
}


QString ProtocolSa::SourceNamesMessage(EndPointMap::RouterType type,QSqlQuery *q)
{
  if(type==EndPointMap::AudioRouter) {
    return QString().sprintf("    %u",q->value(0).toInt()+1)+
      "\t"+q->value(1).toString()+
      "\t"+q->value(1).toString()+" ON "+q->value(3).toString()+
      "\t"+q->value(2).toString()+
      "\t"+q->value(3).toString()+
      "\t"+QString().sprintf("%u",q->value(4).toInt()+1)+
      "\t"+QString().sprintf("%d",SyRouting::livewireNumber(QHostAddress(q->value(5).toString())))+
      "\t"+q->value(5).toString()+
      "\r\n";
  }
  return QString().sprintf("    %u",q->value(0).toInt()+1)+
    "\t"+QString().sprintf("GPI %d",q->value(3).toInt()+1)+
    "\t"+QString().sprintf("GPI %d",q->value(3).toInt()+1)+" ON "+q->value(2).toString()+
    "\t"+q->value(1).toString()+
    "\t"+q->value(2).toString()+
    "\t"+QString().sprintf("%u",q->value(3).toInt()+1)+
    "\t"+q->value(1).toString()+QString().sprintf("/%d",q->value(3).toInt()+1)+
    "\t0"+
    "\r\n";
}


void ProtocolSa::SendDestInfo(unsigned router)
{
  EndPointMap *map;
  QString sql;
  QSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  proto_socket->write(">>",2);
  if(map->routerType()==EndPointMap::AudioRouter) {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString().sprintf("SA_DESTINATIONS.ROUTER_NUMBER=%u ",router)+
      "order by SA_DESTINATIONS.SOURCE_NUMBER";
  }
  else {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString().sprintf("SA_GPOS.ROUTER_NUMBER=%u ",router)+
      "order by SA_GPOS.SOURCE_NUMBER";
  }
  q=new QSqlQuery(sql);
  proto_socket->write(QString().sprintf("Begin DestNames - %d\r\n",router+1).toUtf8());
  while(q->next()) {
    proto_socket->write(DestNamesMessage(map->routerType(),q).toUtf8());
  }
  proto_socket->write(QString().sprintf("End DestNames - %d\r\n",router+1).toUtf8());
}


QString ProtocolSa::DestNamesSqlFields(EndPointMap::RouterType type) const
{
  if(type==EndPointMap::AudioRouter) {
    return QString("select ")+
      "SA_DESTINATIONS.SOURCE_NUMBER,"+  // 00
      "DESTINATIONS.NAME,"+              // 01
      "DESTINATIONS.HOST_ADDRESS,"+      // 02
      "DESTINATIONS.HOST_NAME,"+         // 03
      "DESTINATIONS.SLOT "+              // 04
      "from DESTINATIONS left join SA_DESTINATIONS on DESTINATIONS.ID=SA_DESTINATIONS.DESTINATION_ID ";
  }
  return QString("select ")+
    "SA_GPOS.SOURCE_NUMBER,"+  // 00
    "GPOS.NAME,"+              // 01
    "GPOS.HOST_ADDRESS,"+      // 02
    "GPOS.HOST_NAME,"+         // 03
    "GPOS.SLOT "+              // 04
    "from GPOS left join SA_GPOS on GPOS.ID=SA_GPOS.GPO_ID ";
}


QString ProtocolSa::DestNamesMessage(EndPointMap::RouterType type,QSqlQuery *q)
{
  return QString().sprintf("    %u",q->value(0).toInt()+1)+
    "\t"+q->value(1).toString()+
    "\t"+q->value(1).toString()+" ON "+q->value(3).toString()+
    "\t"+q->value(2).toString()+
    "\t"+q->value(3).toString()+
    "\t"+QString().sprintf("%d",q->value(4).toInt()+1)+
    "\r\n";
}


void ProtocolSa::SendGpiInfo(unsigned router,int input)
{
  EndPointMap *map;
  QString sql;
  QSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Router Does Not exist.\r\n").toUtf8());
    proto_socket->write(">>",2);
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    proto_socket->write(QString("Error - Router is not a GPIO Router.\r\n").toUtf8());
    return;
  }
  if(input<0) {
    sql=GPIStatSqlFields()+"where "+
      QString().sprintf("SA_GPIS.ROUTER_NUMBER=%u ",router)+
      "order by SA_GPIS.SOURCE_NUMBER";
  }
  else {
    sql=GPIStatSqlFields()+"where "+
      QString().sprintf("SA_GPIS.ROUTER_NUMBER=%u && ",router)+
      QString().sprintf("SA_GPIS.SOURCE_NUMBER=%u ",input)+
      "order by SA_GPIS.SOURCE_NUMBER";
  }
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPIStatMessage(q).toUtf8());
  }
}


QString ProtocolSa::GPIStatSqlFields() const
{
  return QString("select ")+
    "SA_GPIS.ROUTER_NUMBER,"+  // 00
    "SA_GPIS.SOURCE_NUMBER,"+  // 01
    "GPIS.CODE "+              // 02
    "from GPIS left join SA_GPIS on GPIS.ID=SA_GPIS.GPI_ID ";
}


QString ProtocolSa::GPIStatMessage(QSqlQuery *q)
{
  return QString().sprintf("GPIStat %d %d ",q->value(0).toInt()+1,q->value(1).toInt()+1)+
    q->value(2).toString()+"\r\n";
}


void ProtocolSa::SendGpoInfo(unsigned router,int output)
{
  EndPointMap *map;
  QString sql;
  QSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Router Does Not exist.\r\n").toUtf8());
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    proto_socket->write(QString("Error - Router is not a GPIO Router.\r\n").toUtf8());
    return;
  }
  proto_socket->write(">>",2);
  if(output<0) {
    sql=GPOStatSqlFields()+"where "+
      QString().sprintf("SA_GPOS.ROUTER_NUMBER=%u ",router)+
      "order by SA_GPOS.SOURCE_NUMBER";
  }
  else {
    sql=GPOStatSqlFields()+"where "+
      QString().sprintf("SA_GPOS.ROUTER_NUMBER=%u && ",router)+
      QString().sprintf("SA_GPOS.SOURCE_NUMBER=%u ",output)+
      "order by SA_GPOS.SOURCE_NUMBER";
  }
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPOStatMessage(q).toUtf8());
  }
}


QString ProtocolSa::GPOStatSqlFields() const
{
  return QString("select ")+
    "SA_GPOS.ROUTER_NUMBER,"+  // 00
    "SA_GPOS.SOURCE_NUMBER,"+  // 01
    "GPOS.CODE "+              // 02
    "from GPOS left join SA_GPOS on GPOS.ID=SA_GPOS.GPO_ID ";
}


QString ProtocolSa::GPOStatMessage(QSqlQuery *q)
{
  return QString().sprintf("GPOStat %d %d ",q->value(0).toInt()+1,q->value(1).toInt()+1)+
    q->value(2).toString()+"\r\n";
}


void ProtocolSa::SendRouteInfo(unsigned router,int output)
{
  EndPointMap *map;
  QString sql;
  QSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  sql=RouteStatSqlFields(map->routerType());
  if(map->routerType()==EndPointMap::AudioRouter) {
    sql+=QString().sprintf("&& SA_DESTINATIONS.ROUTER_NUMBER=%d ",router);
    if(output>=0) {
      sql+=QString().sprintf("&& SA_DESTINATIONS.SOURCE_NUMBER=%d ",output);
    }
    sql+="order by SA_DESTINATIONS.SOURCE_NUMBER";
  }
  else {
    sql+=QString().sprintf("SA_GPOS.ROUTER_NUMBER=%d ",router);
    if(output>=0) {
      sql+=QString().sprintf("&& SA_GPOS.SOURCE_NUMBER=%d ",output);
    }
    sql+="order by SA_GPOS.SOURCE_NUMBER";
  }
  q=new QSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(RouteStatMessage(q).toUtf8());
  }
  delete q;
}


QString ProtocolSa::RouteStatSqlFields(EndPointMap::RouterType type)
{
  if(type==EndPointMap::AudioRouter) {
    return QString("select ")+
      "SA_DESTINATIONS.ROUTER_NUMBER,"+  // 00
      "SA_DESTINATIONS.SOURCE_NUMBER,"+  // 01
      "SA_SOURCES.SOURCE_NUMBER "+       // 02
      "from DESTINATIONS right join SA_DESTINATIONS on DESTINATIONS.ID=SA_DESTINATIONS.DESTINATION_ID "+
      "left join SOURCES on SOURCES.STREAM_ADDRESS=DESTINATIONS.STREAM_ADDRESS "+
      "left join SA_SOURCES on SA_SOURCES.SOURCE_ID=SOURCES.ID where "+
      "(SOURCES.STREAM_ENABLED=1 || SA_SOURCES.SOURCE_NUMBER is null) ";
  }
  else {
    return QString("select ")+
      "SA_GPOS.ROUTER_NUMBER,"+  // 00
      "SA_GPOS.SOURCE_NUMBER,"+  // 01
      "SA_GPIS.SOURCE_NUMBER "+       // 02
      "from GPOS right join SA_GPOS on GPOS.ID=SA_GPOS.GPO_ID "+
      "left join GPIS on GPIS.HOST_ADDRESS=GPOS.SOURCE_ADDRESS && "+
      "GPIS.SLOT=GPOS.SOURCE_SLOT "+
      "left join SA_GPIS on SA_GPIS.GPI_ID=GPIS.ID where ";
  }
  return QString();
}


QString ProtocolSa::RouteStatMessage(QSqlQuery *q)
{
  int input=q->value(2).toInt()+1;
  if(q->value(2).isNull()) {
    input=0;
  }
  return QString().sprintf("RouteStat %d %d %d False\r\n",
			   q->value(0).toInt()+1,
			   q->value(1).toInt()+1,
			   input);
}


void ProtocolSa::ProcessCommand(const QString &cmd)
{
  unsigned cardnum=0;
  unsigned input=0;
  unsigned output=0;
  unsigned msecs=0;
  bool ok=false;
  QStringList cmds=cmd.split(" ");

  if(cmds[0].toLower()=="login") {  // FIXME: We should check the password here!
    proto_socket->write(QString("Login Successful\r\n").toUtf8());
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="exit")||(cmds[0].toLower()=="quit")) {
    quit();
  }

  if((cmds[0].toLower()=="help")||(cmds[0]=="?")) {
    if(cmds.size()==1) {
      proto_socket->write((proto_help_strings[""]+"\r\n\r\n").toUtf8());
    }
    else {
      if(proto_help_strings[cmds[1].toLower()]==NULL) {
	proto_socket->write(QString("\r\n\r\n").toUtf8());
      }
      else {
	proto_socket->write((proto_help_strings[cmds[1].toLower()]+"\r\n\r\n").
			    toUtf8());
      }
    }
    proto_socket->write(">>",2);
  }

  if(cmds[0].toLower()=="routernames") {
    proto_socket->write(QString("Begin RouterNames\r\n").toUtf8());
    for(QMap<int,EndPointMap *>::const_iterator it=proto_maps.begin();
	it!=proto_maps.end();it++) {
      proto_socket->write((QString().sprintf("    %d ",it.value()->routerNumber()+1)+
			   it.value()->routerName()+"\r\n").toUtf8());
    }
    proto_socket->write(QString("End RouterNames\r\n").toUtf8());
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="gpistat")&&(cmds.size()>=2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	SendGpiInfo(cardnum-1,-1);
      }
      else {
	input=cmds[2].toUInt(&ok);
	if(ok) {
	  SendGpiInfo(cardnum-1,input-1);
	}
      }
    }
    else {
      proto_socket->write(QString("Error - Router Does Not exist.\r\n").
			  toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="gpostat")&&(cmds.size()>=2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	SendGpoInfo(cardnum-1,-1);
      }
      else {
	input=cmds[2].toUInt(&ok);
	if(ok) {
	  SendGpoInfo(cardnum-1,input-1);
	}
      }
    }
    else {
      proto_socket->write(QString("Error - Router Does Not exist.\r\n").
			  toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="sourcenames")&&(cmds.size()==2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      SendSourceInfo(cardnum-1);
    }
    else {
      proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="destnames")&&(cmds.size()==2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      SendDestInfo(cardnum-1);
    }
    else {
      proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if(cmds[0].toLower()=="activateroute") {
    if(cmds.size()==4) {
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	output=cmds[2].toUInt(&ok);
	if(ok) {
	  input=cmds[3].toUInt(&ok);
	  if(ok) {
	    ActivateRoute(cardnum-1,output-1,input);
	    //	    emit setRoute(id,cardnum-1,input,output-1);
	  }
	  else {
	    proto_socket->write(QString("Error\r\n").toUtf8());
	  }
	}
	else {
	  proto_socket->write(QString("Error\r\n").toUtf8());
	}
      }
      else {
	proto_socket->write(QString("Error\r\n").toUtf8());
      }
    }
    else {
      proto_socket->write(QString("Error\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if(cmds[0].toLower()=="routestat") {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	SendRouteInfo(cardnum-1,-1);
      }
      else {
	input=cmds[2].toUInt(&ok);
	if(ok) {
	  SendRouteInfo(cardnum-1,input-1);
	}
      }
    }
    else {
      proto_socket->write(QString("Error - Bay Does Not exist.\r\n").
			  toUtf8());
    }
    proto_socket->write(">>",2);
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
	  TriggerGpi(cardnum-1,input-1,msecs,cmds[3]);
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
	input=cmds[2].toUInt(&ok);
	if(cmds[3].length()==5) {
	  TriggerGpo(cardnum-1,input-1,msecs,cmds[3]);
	}
      }
    }
  }

  if((cmds[0].toLower()=="snapshots")&&(cmds.size()==2)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      SendSnapshotNames(cardnum-1);
    }
    else {
      proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if(((cmds[0].toLower()=="activatescene")||
      (cmds[0].toLower()=="activatesnap"))&&(cmds.size()>=3)) {
    cardnum=cmds[1].toUInt(&ok);
    if(ok) {
      QString snapshot="";
      for(int i=2;i<cmds.size();i++) {
	snapshot+=cmds.at(i)+" ";
      }
      ActivateSnapshot(cardnum-1,snapshot.trimmed());
    }
    else {
      proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }
}


void ProtocolSa::LoadMaps()
{
  //
  // Load New Maps
  //
  QStringList msgs;
  if(!EndPointMap::loadSet(&proto_maps,&msgs)) {
    fprintf(stderr,"dprotod: %s\n",(const char *)msgs.join("\n").toUtf8());
    exit(1);
  }
  for(int i=0;i<msgs.size();i++) {
    syslog(LOG_DEBUG,"%s",(const char *)msgs.at(i).toUtf8());
  }
  syslog(LOG_INFO,"loaded %d SA map(s)",proto_maps.size());
}


void ProtocolSa::LoadHelp()
{
  proto_help_strings[""]=QString("ActivateRoute")+
    ", ActivateScene"+
    ", ActivateSnap"+
    ", DestNames"+
    ", Exit"+
    ", GPIStat"+
    ", GPOStat"+
    ", Quit"+
    ", RouterNames"+
    ", RouteStat"+
    ", SnapShots"+
    ", SourceNames"+
    ", TriggerGPI"+
    ", TriggerGPO"+
    "\r\n\r\nEnter \"Help\" or \"?\" followed by the name of the command.";
  proto_help_strings["activateroute"]="ActivateRoute <router> <output> <input>\r\n\r\nRoute <input> to <output> on <router>.";
  proto_help_strings["activatescene"]="ActivateScene <router> <snapshot>\r\n\r\nActivate the specified snapshot.";
  proto_help_strings["activatesnap"]="ActivateSnap <router> <snapshot>\r\n\r\nActivate the specified snapshot.";
  proto_help_strings["destnames"]="DestNames <router>\r\n\r\nReturn names of all outputs on the specified router.";
  proto_help_strings["exit"]="Exit\r\n\r\nClose TCP/IP connection.";
  proto_help_strings["gpistat"]="GPIStat <router> [<gpi-num>]\r\n\r\nQuery the state of one or more GPIs.\r\nIf <gpi-num> is not given, the entire set of GPIs for the specified\r\n<router> will be returned.";
  proto_help_strings["gpostat"]="GPOStat <router> [<gpo-num>]\r\n\r\nQuery the state of one or more GPOs.\r\nIf <gpo-num> is not given, the entire set of GPOs for the specified\r\n<router> will be returned.";
  proto_help_strings["quit"]="Quit\r\n\r\nClose TCP/IP connection.";
  proto_help_strings["routernames"]="RouterNames\r\n\r\nReturn a list of configured matrices.";
  proto_help_strings["routestat"]="RouteStat <router> [<output>]\r\n\r\nReturn the <output> crosspoint's input assignment.\r\nIf not <output> is given, the crosspoint states for all outputs on\r\n<router> will be returned.";
  proto_help_strings["sourcenames"]="SourceNames <router>\r\n\r\nReturn names of all inputs on the specified router.";
  proto_help_strings["triggergpi"]="TriggerGPI <router> <gpi-num> <state> [<duration>]\r\n\r\nSet the specified GPI to <state> for <duration> milliseconds.\r\n(Supported only by virtual GPI devices.)";
  proto_help_strings["triggergpo"]="TriggerGPO <router> <gpo-num> <state> [<duration>]\r\n\r\nSet the specified GPO to <state> for <duration> milliseconds.";
  proto_help_strings["snapshots"]="SnapShots <router>\r\n\r\nReturn list of available snapshots on the specified router.";
}
