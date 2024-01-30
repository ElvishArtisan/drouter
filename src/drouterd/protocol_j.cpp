// protocol_j.cpp
//
// Protocol J protocol handler for DRouter.
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
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

#include <QJsonDocument>
#include <QSqlError>
#include <QStringList>

#include <sy5/syrouting.h>

#include "protocol_j.h"

ProtocolJ::ProtocolJ(int sock,QObject *parent)
  : Protocol(parent)
{
  int flags;
  QString sql;

  proto_socket=NULL;
  proto_destinations_subscribed=false;
  proto_gpis_subscribed=false;
  proto_gpos_subscribed=false;
  proto_nodes_subscribed=false;
  proto_sources_subscribed=false;
  proto_clips_subscribed=false;
  proto_silences_subscribed=false;

  proto_gpistat_masked=false;
  proto_gpostat_masked=false;
  proto_routestat_masked=false;

  openlog("dprotod(J)",LOG_PID,LOG_DAEMON);

  //
  // The ProtocolJ Server
  //
  proto_server=new QTcpServer(this);
  connect(proto_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));
  if(sock<0) {
    proto_server->listen(QHostAddress::Any,9600);
  }
  else {
    proto_server->setSocketDescriptor(sock);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_server->socketDescriptor(),F_SETFD,&flags))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",strerror(errno));
    exit(1);
  }

  LoadMaps();
  LoadHelp();
}


void ProtocolJ::newConnectionData()
{
  int flags;
  QString err_msg;

  //
  // Process Server Connection
  //
  proto_socket=proto_server->nextPendingConnection();
  if((flags=fcntl(proto_socket->socketDescriptor(),F_GETFD,NULL))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",strerror(errno));
    exit(1);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_socket->socketDescriptor(),F_SETFD,&flags))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",strerror(errno));
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


void ProtocolJ::readyReadData()
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


void ProtocolJ::disconnectedData()
{
  quit();
}


void ProtocolJ::snapshotHostLookupFinishedData(const QHostInfo &info)
{
  QString sql=QString("update `PERM_SA_EVENTS` set ")+
    "`HOSTNAME`='"+DRSqlQuery::escape(info.hostName())+"',"+
    "`STATUS`='Y' where "+
    QString::asprintf("`ID`=%d",proto_event_lookups.value(info.lookupId()));
  DRSqlQuery::apply(sql);
}


void ProtocolJ::routeHostLookupFinishedData(const QHostInfo &info)
{
  QString sql=QString("update `PERM_SA_EVENTS` set ")+
    "`HOSTNAME`='"+DRSqlQuery::escape(info.hostName())+"' where "+
    QString::asprintf("`ID`=%d",proto_event_lookups.value(info.lookupId()));
  DRSqlQuery::apply(sql);
}


void ProtocolJ::destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  DRSqlQuery *q;
 
  if(!proto_routestat_masked) {
    sql=RouteStatSqlFields(DREndPointMap::AudioRouter)+
      " `SA_DESTINATIONS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SA_DESTINATIONS`.`SLOT`=%d ",slotnum)+
      "order by `SA_DESTINATIONS`.`SOURCE_NUMBER`,`SA_DESTINATIONS`.`ROUTER_NUMBER`";
    q=new DRSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(RouteStatMessage(q).toUtf8());
    }
    delete q;
  }
}


void ProtocolJ::gpiCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  DRSqlQuery *q;

  if(!proto_gpistat_masked) {
    sql=GPIStatSqlFields()+" where "+
      "`GPIS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`GPIS`.`SLOT`=%d",slotnum);
    q=new DRSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GPIStatMessage(q).toUtf8());
    }
    delete q;
  }
}


void ProtocolJ::gpoCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  DRSqlQuery *q;

  if(!proto_gpostat_masked) {
    sql=GPOStatSqlFields()+" where "+
      "`GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`GPOS`.`SLOT`=%d",slotnum);
    q=new DRSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GPOStatMessage(q).toUtf8());
    }
    delete q;
  }
}


void ProtocolJ::gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  DRSqlQuery *q;

  if(!proto_routestat_masked) {
    sql=RouteStatSqlFields(DREndPointMap::GpioRouter)+
      " `SA_GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SA_GPOS`.`SLOT`=%d ",slotnum)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`,`SA_GPOS`.`ROUTER_NUMBER`";
    q=new DRSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(RouteStatMessage(q).toUtf8());
    }
    delete q;
  }
}


void ProtocolJ::quitting()
{
  shutdown(proto_socket->socketDescriptor(),SHUT_RDWR);
}


void ProtocolJ::ActivateRoute(unsigned router,unsigned output,unsigned input)
{
  DREndPointMap *map;

  AddRouteEvent(router,output,input-1);
  if((map=proto_maps.value(router))!=NULL) {
    QHostAddress dst_addr=map->hostAddress(DREndPointMap::Output,output);
    int dst_slotnum=map->slot(DREndPointMap::Output,output);
    if(!dst_addr.isNull()&&(dst_slotnum>=0)) {
      if(input==0) {
	switch(map->routerType()) {
	case DREndPointMap::AudioRouter:
	  clearCrosspoint(dst_addr,dst_slotnum);
	  break;

	case DREndPointMap::GpioRouter:
	  clearGpioCrosspoint(dst_addr,dst_slotnum);
	  break;

	case DREndPointMap::LastRouter:
	  break;
	}
      }
      else {
	QHostAddress src_addr=map->hostAddress(DREndPointMap::Input,input-1);
	int src_slotnum=map->slot(DREndPointMap::Input,input-1);
	if(!src_addr.isNull()&&(src_slotnum>=0)) {
	  switch(map->routerType()) {
	  case DREndPointMap::AudioRouter:
	    setCrosspoint(dst_addr,dst_slotnum,src_addr,src_slotnum);
	    syslog(LOG_INFO,"activated audio route router: %d  input: %d to output: %d from %s",
		   router+1,output+1,input,
		   proto_socket->peerAddress().toString().toUtf8().constData());
	    break;
	  
	  case DREndPointMap::GpioRouter:
	    setGpioCrosspoint(dst_addr,dst_slotnum,src_addr,src_slotnum);
	    syslog(LOG_INFO,"activated gpio route router: %d  input: %d to output: %d from %s",
		   router+1,output+1,input,
		   proto_socket->peerAddress().toString().toUtf8().constData());
	    break;

	  case DREndPointMap::LastRouter:
	    break;
	  }
	}
      }
    }
  }
}


void ProtocolJ::TriggerGpi(unsigned router,unsigned input,unsigned msecs,const QString &code)
{
  DREndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==DREndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(DREndPointMap::Input,input);
      int slotnum=map->slot(DREndPointMap::Input,input);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpiState(addr,slotnum,code);
	syslog(LOG_INFO,
	       "set gpi state router: %d  input: %d to state: %s from %s",
	       router+1,input+1,code.toUtf8().constData(),
	       proto_socket->peerAddress().toString().toUtf8().constData());
      }
    }
  }
}


void ProtocolJ::TriggerGpo(unsigned router,unsigned output,unsigned msecs,const QString &code)
{
  DREndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==DREndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(DREndPointMap::Output,output);
      int slotnum=map->slot(DREndPointMap::Output,output);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpoState(addr,slotnum,code);
	syslog(LOG_INFO,
	       "set gpo state router: %d  output: %d to state: %s from %s",
	       router+1,output+1,
	       code.toUtf8().constData(),
	       proto_socket->peerAddress().toString().toUtf8().constData());
      }
    }
  }
}


void ProtocolJ::SendSnapshotNames(unsigned router)
{
  DREndPointMap *map=NULL;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }

  QString json="{\r\n";
  json+="    \"snapshots\": {\r\n";
  json+=JsonField("router",1+router,8,map->snapshotQuantity()==0);
  for(int i=0;i<map->snapshotQuantity();i++) {
    json+=QString::asprintf("        \"snapshot%d\": {\r\n",i);
    json+=JsonField("name",map->snapshot(i)->name(),12,true);
    json+="        "+JsonCloseBlock(i==(map->snapshotQuantity()-1));
  }
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::SendSnapshotRoutes(unsigned router,const QString &snap_name)
{
  DREndPointMap *map=NULL;
  DRSnapshot *ss=NULL;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  if((ss=map->snapshot(snap_name))==NULL) {
    SendError(DRJParser::NoSnapshotError);
    return;
  }
  proto_socket->write((QString("Begin SnapshotRoutes - ")+
		       QString::asprintf("%u ",router+1)+
		       snap_name+"\r\n").toUtf8());
  for(int i=0;i<ss->routeQuantity();i++) {
    proto_socket->write(QString("   ActivateRoute "+
				QString::asprintf("%d ",1+router)+
				QString::asprintf("%d ",1+ss->routeOutput(i))+
				QString::asprintf("%d",1+ss->routeInput(i))+
				"\r\n").toUtf8());
  }
  proto_socket->write((QString("End SnapshotRoutes - ")+
		       QString::asprintf("%u ",router+1)+
		       snap_name+"\r\n").toUtf8());
}


void ProtocolJ::ActivateSnapshot(unsigned router,const QString &snapshot_name)
{
  QString sql;
  DREndPointMap *map=NULL;
  DRSnapshot *ss=NULL;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  proto_socket->write(QString("Snapshot Initiated\r\n").toUtf8());
  if((ss=map->snapshot(snapshot_name))!=NULL) {
    AddSnapEvent(router,snapshot_name);
    for(int i=0;i<ss->routeQuantity();i++) {
      ActivateRoute(router,ss->routeOutput(i)-1,ss->routeInput(i));
    }
  }
  syslog(LOG_INFO,"activated snapshot %d:%s from %s",router+1,
	 snapshot_name.toUtf8().constData(),
	 proto_socket->peerAddress().toString().toUtf8().constData());
}


void ProtocolJ::SendSourceInfo(unsigned router)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  if(map->routerType()==DREndPointMap::AudioRouter) {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_SOURCES`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_SOURCES`.`SOURCE_NUMBER`";
  }
  else {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  q=new DRSqlQuery(sql);
  int quan=0;
  QString json="{\r\n";
  json+="    \"sourcenames\": {\r\n";
  json+=JsonField("router",1+router,8,q->size()<=0);
  while(q->next()) {
    quan++;
    json+=SourceNamesMessage(map->routerType(),q,8,quan==q->size());
  }
  delete q;
  json+="    }\r\n";
  json+="}\r\n";
  proto_socket->write(json.toUtf8());
}


QString ProtocolJ::SourceNamesSqlFields(DREndPointMap::RouterType type) const
{
  if(type==DREndPointMap::AudioRouter) {
    return QString("select ")+
      "`SA_SOURCES`.`SOURCE_NUMBER`,"+  // 00
      "`SOURCES`.`NAME`,"+              // 01
      "`SOURCES`.`HOST_ADDRESS`,"+      // 02
      "`SOURCES`.`HOST_NAME`,"+         // 03
      "`SOURCES`.`SLOT`,"+              // 04
      "`SOURCES`.`STREAM_ADDRESS`,"+    // 05
      "`SA_SOURCES`.`NAME`,"+           // 06
      "`NODES`.`MATRIX_TYPE`,"+         // 07
      "`NODES`.`HOST_DESCRIPTION` "+    // 08
      "from "+"`SOURCES` left join `SA_SOURCES` "+
      "on `SOURCES`.`ID`=`SA_SOURCES`."+"`SOURCE_ID` "+
      "left join `NODES` "+
      "on `NODES`.`HOST_ADDRESS`=`SOURCES`.`HOST_ADDRESS` ";
  }
  return QString("select ")+
    "`SA_GPIS`.`SOURCE_NUMBER`,"+   // 00
    "`GPIS`.`HOST_ADDRESS`,"+       // 01
    "`GPIS`.`HOST_NAME`,"+          // 02
    "`GPIS`.`SLOT`,"+               // 03
    "`SA_GPIS`.`NAME`,"+            // 04
    "`NODES`.`MATRIX_TYPE`,"+       // 05
    "`NODES`.`HOST_DESCRIPTION` "+  // 06
    "from "+"`GPIS` left join `SA_GPIS` "+
    "on `GPIS`.`ID`=`SA_GPIS`.`GPI_ID` "+
    "left join `NODES` "+
    "on `NODES`.`HOST_ADDRESS`=`GPIS`.`HOST_ADDRESS` ";
}


QString ProtocolJ::SourceNamesMessage(DREndPointMap::RouterType type,DRSqlQuery *q,
				      int padding,bool final)
{
  if(type==DREndPointMap::AudioRouter) {
    QString name=q->value(1).toString();
    if(!q->value(6).toString().isEmpty()) {
      name=q->value(6).toString();
    }
    QString json=JsonPadding(padding)+
      QString::asprintf("\"source%d\": {\r\n",q->at());
    json+=JsonField("number",1+q->value(0).toInt(),4+padding);
    json+=JsonField("name",name,4+padding);
    json+=JsonField("hostDescription",q->value(8).toString(),4+padding,
		    q->value(7).toUInt()!=DRConfig::LwrpMatrix);
    if(q->value(7).toUInt()==DRConfig::LwrpMatrix) {
      json+=JsonField("hostAddress",q->value(2).toString(),4+padding);
      json+=JsonField("hostName",q->value(3).toString(),4+padding);
      json+=JsonField("slot",1+q->value(4).toInt(),4+padding);
      json+=JsonField("sourceNumber",
	       SyRouting::livewireNumber(QHostAddress(q->value(5).toString())),
		    4+padding);
      json+=JsonField("streamAddress",q->value(5).toString(),4+padding,true);
    }
    json+=JsonPadding(padding)+"}";
    if(!final) {
      json+=",";
    }
    json+="\r\n";

    return json;
  }
  QString name="GPI";
  if(!q->value(4).toString().isEmpty()) {
    name=q->value(4).toString();
  }
  QString json=JsonPadding(padding)+
    QString::asprintf("\"source%d\": {\r\n",q->at());
  json+=JsonField("number",1+q->value(0).toInt(),4+padding);
  json+=JsonField("name",name,4+padding);
  json+=JsonField("hostDescription",q->value(6).toString(),4+padding,
		  q->value(5).toUInt()!=DRConfig::LwrpMatrix);
  if(q->value(5).toUInt()==DRConfig::LwrpMatrix) {
    json+=JsonField("hostAddress",q->value(1).toString(),4+padding);
    json+=JsonField("hostName",q->value(2).toString(),4+padding);
    json+=JsonField("slot",1+q->value(3).toInt(),4+padding);
    json+=JsonField("gpioAddress",q->value(1).toString()+
		    QString::asprintf("/%d",q->value(3).toInt()+1),4+padding,
		    true);
  }
  json+=JsonPadding(padding)+"}";
  if(!final) {
    json+=",";
  }
  json+="\r\n";
  
  return json;
}


void ProtocolJ::SendDestInfo(unsigned router)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  if(map->routerType()==DREndPointMap::AudioRouter) {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_DESTINATIONS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_DESTINATIONS`.`SOURCE_NUMBER`";
  }
  else {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  q=new DRSqlQuery(sql);
  int quan=0;
  QString json="{\r\n";
  json+="    \"destnames\": {\r\n";
  json+=JsonField("router",1+router,8,q->size()<=0);
  while(q->next()) {
    quan++;
    json+=DestNamesMessage(map->routerType(),q,8,quan==q->size());
  }
  delete q;
  json+="    }\r\n";
  json+="}\r\n";
  proto_socket->write(json.toUtf8());
}


QString ProtocolJ::DestNamesSqlFields(DREndPointMap::RouterType type) const
{
  if(type==DREndPointMap::AudioRouter) {
    return QString("select ")+
      "`SA_DESTINATIONS`.`SOURCE_NUMBER`,"+  // 00
      "`DESTINATIONS`.`NAME`,"+              // 01
      "`DESTINATIONS`.`HOST_ADDRESS`,"+      // 02
      "`DESTINATIONS`.`HOST_NAME`,"+         // 03
      "`DESTINATIONS`.`SLOT`,"+              // 04
      "`SA_DESTINATIONS`.`NAME`,"+           // 05
      "`NODES`.`MATRIX_TYPE`,"+              // 06
      "`NODES`.`HOST_DESCRIPTION` "+         // 07
      "from `DESTINATIONS` left join `SA_DESTINATIONS` on `DESTINATIONS`.`ID`=`SA_DESTINATIONS`.`DESTINATION_ID` "+
      "left join `NODES` on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
  }
  return QString("select ")+
    "`SA_GPOS`.`SOURCE_NUMBER`,"+   // 00
    "`GPOS`.`NAME`,"+               // 01
    "`GPOS`.`HOST_ADDRESS`,"+       // 02
    "`GPOS`.`HOST_NAME`,"+          // 03
    "`GPOS`.`SLOT`,"+               // 04
    "`SA_GPOS`.`NAME`,"+            // 05
    "`NODES`.`MATRIX_TYPE`,"+       // 06
    "`NODES`.`HOST_DESCRIPTION` "+  // 07
    "from `GPOS` left join `SA_GPOS` on `GPOS`.`ID`=`SA_GPOS`.`GPO_ID` "+
    "left join `NODES` on `GPOS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
}


QString ProtocolJ::DestNamesMessage(DREndPointMap::RouterType type,DRSqlQuery *q,
				    int padding,bool final)
{
  QString name=q->value(1).toString();
  if(!q->value(5).toString().isEmpty()) {
    name=q->value(5).toString();
  }

  QString json=JsonPadding(padding)+
    QString::asprintf("\"destination%d\": {\r\n",q->at());
  json+=JsonField("number",1+q->value(0).toInt(),4+padding);
  json+=JsonField("name",name,4+padding);
  json+=JsonField("hostDescription",q->value(7).toString(),4+padding,
		  q->value(6).toUInt()!=DRConfig::LwrpMatrix);
  if(q->value(6).toUInt()==DRConfig::LwrpMatrix) {
    json+=JsonField("hostAddress",q->value(2).toString(),4+padding);
    json+=JsonField("hostName",q->value(3).toString(),4+padding);
    json+=JsonField("slot",1+q->value(4).toInt(),4+padding,true);
  }
  json+=JsonPadding(padding)+"}";
  if(!final) {
    json+=",";
  }
  json+="\r\n";

  return json;
}


void ProtocolJ::SendGpiInfo(unsigned router,int input)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  if(map->routerType()!=DREndPointMap::GpioRouter) {
    SendError(DRJParser::NotGpioRouterError);
    return;
  }
  if(input<0) {
    sql=GPIStatSqlFields()+"where "+
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  else {
    sql=GPIStatSqlFields()+"where "+
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u && ",router)+
      QString::asprintf("`SA_GPIS`.`SOURCE_NUMBER`=%u ",input)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  q=new DRSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPIStatMessage(q).toUtf8());
  }
  delete q;
}


QString ProtocolJ::GPIStatSqlFields() const
{
  return QString("select ")+
    "`SA_GPIS`.`ROUTER_NUMBER`,"+  // 00
    "`SA_GPIS`.`SOURCE_NUMBER`,"+  // 01
    "`GPIS`.`CODE` "+              // 02
    "from `GPIS` right join `SA_GPIS` on `GPIS`.`ID`=`SA_GPIS`.`GPI_ID` ";
}


QString ProtocolJ::GPIStatMessage(DRSqlQuery *q)
{
  QString json="{\r\n";
  json+="    \"gpistat\": {\r\n";
  json+=JsonField("router",1+q->value(0).toInt(),8);
  json+=JsonField("source",1+q->value(1).toInt(),8);
  json+=JsonField("code",q->value(2).toString(),8,true);
  json+="    }\r\n";
  json+="}\r\n";

  return json;
}


void ProtocolJ::SendGpoInfo(unsigned router,int output)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  if(map->routerType()!=DREndPointMap::GpioRouter) {
    SendError(DRJParser::NotGpioRouterError);
    return;
  }
  if(output<0) {
    sql=GPOStatSqlFields()+"where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  else {
    sql=GPOStatSqlFields()+"where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u && ",router)+
      QString::asprintf("`SA_GPOS`.`SOURCE_NUMBER`=%u ",output)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  q=new DRSqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPOStatMessage(q).toUtf8());
  }
  delete q;
}


QString ProtocolJ::GPOStatSqlFields() const
{
  return QString("select ")+
    "`SA_GPOS`.`ROUTER_NUMBER`,"+  // 00
    "`SA_GPOS`.`SOURCE_NUMBER`,"+  // 01
    "`GPOS`.`CODE` "+              // 02
    "from `GPOS` right join `SA_GPOS` on `GPOS`.`ID`=`SA_GPOS`.`GPO_ID` ";
}


QString ProtocolJ::GPOStatMessage(DRSqlQuery *q)
{
  QString json="{\r\n";
  json+="    \"gpostat\": {\r\n";
  json+=JsonField("router",1+q->value(0).toInt(),8);
  json+=JsonField("destination",1+q->value(1).toInt(),8);
  json+=JsonField("code",q->value(2).toString(),8,true);
  json+="    }\r\n";
  json+="}\r\n";

  return json;
}


void ProtocolJ::SendRouteInfo(unsigned router,int output)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  sql=RouteStatSqlFields(map->routerType());
  if(map->routerType()==DREndPointMap::AudioRouter) {
    sql+=QString::asprintf("`SA_DESTINATIONS`.`ROUTER_NUMBER`=%d ",router);
    if(output>=0) {
      sql+=QString::asprintf("&& `SA_DESTINATIONS`.`SOURCE_NUMBER`=%d ",output);
    }
    sql+="&& `SA_SOURCES`.`STREAM_ADDRESS`!='"+DROUTER_NULL_STREAM_ADDRESS+"' ";
    sql+="&& `SA_SOURCES`.`STREAM_ADDRESS`!='0.0.0.0' ";
    sql+="&& `SA_SOURCES`.`STREAM_ADDRESS`!='255.255.255.255' ";
    sql+="&& `SA_SOURCES`.`STREAM_ADDRESS`!='' ";
    sql+="&& `SA_SOURCES`.`STREAM_ADDRESS` is not null ";
    sql+="order by `SA_DESTINATIONS`.`SOURCE_NUMBER`";
  }
  else {
    sql+=QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%d ",router);
    if(output>=0) {
      sql+=QString::asprintf("&& `SA_GPOS`.`SOURCE_NUMBER`=%d ",output);
    }
    sql+="order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  q=new DRSqlQuery(sql);
  q->first();
  if(output<0) {  // Send all crosspoints for the router
    for(int i=0;i<map->quantity(DREndPointMap::Output);i++) {
      if(q->isValid()&&q->value(1).toInt()==i) {
	proto_socket->write(RouteStatMessage(q).toUtf8());
	q->next();
      }
      else {
	proto_socket->write(RouteStatMessage(router,i,-1).toUtf8());
      }
    }
  }
  else {  // Send just the requested crosspoint
    if(output<map->quantity(DREndPointMap::Output)) {
      if(q->isValid()) {
	proto_socket->write(RouteStatMessage(q).toUtf8());
      }
      else {
	proto_socket->write(RouteStatMessage(router,output,-1).toUtf8());
      }
    }
  }
  delete q;
}


QString ProtocolJ::RouteStatSqlFields(DREndPointMap::RouterType type)
{
  if(type==DREndPointMap::AudioRouter) {
    return QString("select ")+
      "`SA_DESTINATIONS`.`ROUTER_NUMBER`,"+  // 00
      "`SA_DESTINATIONS`.`SOURCE_NUMBER`,"+  // 01
      "`SA_SOURCES`.`SOURCE_NUMBER` "+       // 02
      "from `SA_DESTINATIONS` left join `SA_SOURCES` "+
      "on `SA_DESTINATIONS`.`STREAM_ADDRESS`=`SA_SOURCES`.`STREAM_ADDRESS` && "+
      "`SA_SOURCES`.`ROUTER_NUMBER`=`SA_DESTINATIONS`.`ROUTER_NUMBER` where ";
  }
  else {
    return QString("select ")+
      "`SA_GPOS`.`ROUTER_NUMBER`,"+  // 00
      "`SA_GPOS`.`SOURCE_NUMBER`,"+  // 01
      "`SA_GPIS`.`SOURCE_NUMBER` "+  // 02
      "from `SA_GPOS` left join `SA_GPIS` "+
      "on `SA_GPOS`.`SOURCE_ADDRESS`=`SA_GPIS`.`HOST_ADDRESS` && "+
      "`SA_GPOS`.`SOURCE_SLOT`=`SA_GPIS`.`SLOT` && "+
      "`SA_GPIS`.`ROUTER_NUMBER`=`SA_GPOS`.`ROUTER_NUMBER` where ";
  }
  return QString();
}


QString ProtocolJ::RouteStatMessage(int router,int output,int input)
{
  QString json="{\r\n";
  json+="    \"routestat\": {\r\n";
  json+=JsonField("router",1+router,8);
  json+=JsonField("destination",1+output,8);
  json+=JsonField("source",input,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  return json;
}


QString ProtocolJ::RouteStatMessage(DRSqlQuery *q)
{
  int input=q->value(2).toInt()+1;
  if(q->value(2).isNull()) {
    input=-1;
  }
  return RouteStatMessage(q->value(0).toInt(),q->value(1).toInt(),input);
}


void ProtocolJ::MaskGpiStat(bool state)
{
  proto_gpistat_masked=state;

  QString json="{\r\n";
  json+="    \"maskgpistat\": {\r\n";
  json+=JsonField("state",state,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::MaskGpoStat(bool state)
{
  proto_gpostat_masked=state;

  QString json="{\r\n";
  json+="    \"maskgpostat\": {\r\n";
  json+=JsonField("state",state,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::MaskRouteStat(bool state)
{
  proto_routestat_masked=state;

  QString json="{\r\n";
  json+="    \"maskroutestat\": {\r\n";
  json+=JsonField("state",state,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::MaskStat(bool state)
{
  MaskGpiStat(state);
  MaskGpoStat(state);
  MaskRouteStat(state);
}


void ProtocolJ::HelpMessage(const QString &keyword)
{
  if((!keyword.isEmpty())&&(!proto_help_comments.contains(keyword))) {
    SendError(DRJParser::NoCommandError,proto_help_patterns.value(""));
  }
  else {
    QString json="{\r\n";
    json+="    \"help\": {\r\n";
    if(keyword.isEmpty()) {
      json+=JsonNullField("keyword",8);
      json+=JsonField("pattern",proto_help_patterns.value(""),8,true);
      json+=JsonField("comment",proto_help_comments.value(""),8,true);
    }
    else {
      if(proto_help_patterns.value(keyword).isEmpty()) {
	json+=JsonNullField("pattern",8);
      }
      else {
	json+=JsonField("pattern",proto_help_patterns.value(keyword),8);
      }
      if(proto_help_comments.value(keyword).isEmpty()) {
	json+=JsonNullField("comment",8);
      }
      else {
	json+=JsonField("comment",proto_help_comments.value(keyword),8,true);
      }
    }
    json+="    }\r\n";
    json+="}\r\n";

    proto_socket->write(json.toUtf8());
  }
}


void ProtocolJ::SendPingResponse()
{
  QString json="{\r\n";
  json+="    \"pong\": {\r\n";
  json+=JsonField("datetime",QDateTime::currentDateTime(),8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::ProcessCommand(const QString &cmd)
{
  unsigned cardnum=0;
  unsigned input=0;
  unsigned output=0;
  unsigned msecs=0;
  bool ok=false;
  QStringList cmds=cmd.split(" ");

  if((cmds.at(0).toLower()=="exit")||(cmds.at(0).toLower()=="quit")) {
    syslog(LOG_DEBUG,"exiting normally");
    quit();
  }

  if((cmds.at(0).toLower()=="help")||(cmds.at(0)=="?")) {
    if(cmds.size()==1) {
      HelpMessage(QString());
    }
    else {
      HelpMessage(cmds.at(1).toLower());
    }
  }

  if(cmds.at(0).toLower()=="routernames") {
    int count=0;
    QString json="{\r\n";
    json+="    \"routernames\": {\r\n";
    for(QMap<int,DREndPointMap *>::const_iterator it=proto_maps.begin();
	it!=proto_maps.end();it++) {
      json+=QString::asprintf("        \"router%d\": {\r\n",count++);
      json+=JsonField("number",1+it.value()->routerNumber(),12);
      json+=JsonField("name",it.value()->routerName(),12);
      json+=JsonField("type",
	      DREndPointMap::routerTypeString(it.value()->routerType()),12,true);
      json+="        "+JsonCloseBlock((it+1)==proto_maps.end());
    }
    json+="    }\r\n";
    json+="}\r\n";
    proto_socket->write(json.toUtf8());
  }

  if(cmds.at(0).toLower()=="ping") {
    SendPingResponse();
  }

  if((cmds.at(0).toLower()=="gpistat")&&(cmds.size()>=2)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	SendGpiInfo(cardnum-1,-1);
      }
      else {
	input=cmds.at(2).toUInt(&ok);
	if(ok) {
	  SendGpiInfo(cardnum-1,input-1);
	}
      }
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if((cmds.at(0).toLower()=="gpostat")&&(cmds.size()>=2)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      if(cmds.size()==2) {
	SendGpoInfo(cardnum-1,-1);
      }
      else {
	input=cmds.at(2).toUInt(&ok);
	if(ok) {
	  SendGpoInfo(cardnum-1,input-1);
	}
      }
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if((cmds.at(0).toLower()=="sourcenames")&&(cmds.size()==2)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      SendSourceInfo(cardnum-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if((cmds.at(0).toLower()=="destnames")&&(cmds.size()==2)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      SendDestInfo(cardnum-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if(cmds.at(0).toLower()=="activateroute") {
    if(cmds.size()==4) {
      cardnum=cmds.at(1).toUInt(&ok);
      if(ok) {
	output=cmds.at(2).toUInt(&ok);
	if(ok) {
	  input=cmds.at(3).toUInt(&ok);
	  if(ok) {
	    ActivateRoute(cardnum-1,output-1,input);
	  }
	  else {
	    SendError(DRJParser::ParameterError,
		      tr("\"activateroute\" <router> <output> <input>"));
	  }
	}
	else {
	  SendError(DRJParser::ParameterError,
		    tr("\"activateroute\" <router> <output> <input>"));
	}
      }
      else {
	SendError(DRJParser::ParameterError,
		  tr("\"activateroute\" <router> <output> <input>"));
      }
    }
    else {
      SendError(DRJParser::ParameterError,
		proto_help_patterns.value("activateroute"));
    }
  }

  if(cmds.at(0).toLower()=="routestat") {
    if((cmds.size()==2)||(cmds.size()==3)) {
      cardnum=cmds.at(1).toUInt(&ok);
      if(ok) {
	if(cmds.size()==2) {
	  SendRouteInfo(cardnum-1,-1);
	}
	if(cmds.size()==3) {
	  input=cmds.at(2).toUInt(&ok);
	  if(ok) {
	    SendRouteInfo(cardnum-1,input-1);
	  }
	  else {
	    SendError(DRJParser::NoSourceError);
	  }
	}
      }
      else {
	SendError(DRJParser::NoRouterError);
      }
    }
    else {
      SendError(DRJParser::ParameterError,proto_help_patterns.value("routestat"));
    }
  }

  if(cmds.at(0).toLower()=="triggergpi") {
    if((cmds.size()==4)||(cmds.size()==5)) {
      if(cmds.size()==5) {
	msecs=cmds.at(4).toUInt();
      }
      cardnum=cmds.at(1).toUInt(&ok);
      if(ok) {
	input=cmds.at(2).toUInt(&ok);
	if(cmds.at(3).length()==5) {
	  TriggerGpi(cardnum-1,input-1,msecs,cmds.at(3));
	}
      }
    }
  }

  if(cmds.at(0).toLower()=="triggergpo") {
    if((cmds.size()==4)||(cmds.size()==5)) {
      if(cmds.size()==5) {
	msecs=cmds.at(4).toUInt();
      }
      cardnum=cmds.at(1).toUInt(&ok);
      if(ok) {
	input=cmds.at(2).toUInt(&ok);
	if(cmds.at(3).length()==5) {
	  TriggerGpo(cardnum-1,input-1,msecs,cmds.at(3));
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="snapshots")&&(cmds.size()==2)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      SendSnapshotNames(cardnum-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if((cmds.at(0).toLower()=="snapshotnames")&&(cmds.size()==3)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      SendSnapshotRoutes(cardnum-1,cmds.at(2));
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if(((cmds.at(0).toLower()=="activatescene")||
      (cmds.at(0).toLower()=="activatesnap"))&&(cmds.size()>=3)) {
    cardnum=cmds.at(1).toUInt(&ok);
    if(ok) {
      QString snapshot="";
      for(int i=2;i<cmds.size();i++) {
	snapshot+=cmds.at(i)+" ";
      }
      ActivateSnapshot(cardnum-1,snapshot.trimmed());
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
  }

  if((cmds.at(0).toLower()=="maskgpistat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      MaskGpiStat(cmds.at(1).toLower()=="true");
    }
    else {
      SendError(DRJParser::ParameterError,"maskgpistat true|false");
    }
  }

  if((cmds.at(0).toLower()=="maskgpostat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      MaskGpoStat(cmds.at(1).toLower()=="true");
    }
    else {
      SendError(DRJParser::ParameterError,"maskgpostate true|false");
    }
  }

  if((cmds.at(0).toLower()=="maskroutestat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      MaskRouteStat(cmds.at(1).toLower()=="true");
    }
    else {
      SendError(DRJParser::ParameterError,"maskroutestat true|false");
    }
  }

  if((cmds.at(0).toLower()=="maskstat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      MaskStat(cmds.at(1).toLower()=="true");
    }
    else {
      SendError(DRJParser::ParameterError,"maskstat true|false");
    }
  }
}


void ProtocolJ::LoadMaps()
{
  //
  // Load New Maps
  //
  QStringList msgs;
  if(!DREndPointMap::loadSet(&proto_maps,&msgs)) {
    syslog(LOG_ERR,"map load error: %s, aborting",
	   msgs.join("\n").toUtf8().constData());
    exit(1);
  }
}


void ProtocolJ::LoadHelp()
{
  proto_help_patterns[""]=QString("ActivateRoute")+
    ", ActivateSnap"+
    ", DestNames"+
    ", MaskGPIStat"+
    ", MaskGPOStat"+
    ", MaskRouteStat"+
    ", MaskStat"+
    ", Exit"+
    ", GPIStat"+
    ", GPOStat"+
    ", RouteStat"+
    ", RouterNames"+
    ", RouteStat"+
    ", SnapShots"+
    ", SnapShotRoutes"+
    ", SourceNames"+
    ", TriggerGPI"+
    ", TriggerGPO";
  proto_help_comments[""]=
    "Enter 'help' or '?' followed by the name of the command. Command keywords are case insensitive.";
  proto_help_patterns["activateroute"]=
    "ActivateRoute <router> <output> <input>";
  proto_help_comments["activateroute"]="Route <input> to <output> on <router>.";

  proto_help_patterns["activatesnap"]="ActivateSnap <router> <snapshot>";
  proto_help_comments["activatesnap"]="Activate the specified snapshot.";

  proto_help_patterns["destnames"]="DestNames <router>";
  proto_help_comments["destnames"]=
    "Return names of all outputs on the specified router.";

  proto_help_patterns["maskgpistat"]="MaskGPIStat True | False";
  proto_help_comments["maskgpistat"]=
    "Suppress generation of GPIStat update messages on this connection.";

  proto_help_patterns["maskgpostat"]="MaskGPOStat True | False";
  proto_help_comments["maskgpostat"]=
    "Suppress generation of GPOStat update messages on this connection.";

  proto_help_patterns["maskroutestat"]="MaskRouteStat True | False";
  proto_help_comments["maskroutestat"]=
    "Suppress generation of RouteStat update messages on this connection.";

  proto_help_patterns["maskstat"]="MaskStat True | False";
  proto_help_comments["maskstat"]=
    "Suppress generation of all state update messages on this connection.";

  proto_help_patterns["maskroutestat"]="MaskRouteStat True | False";
  proto_help_comments["maskroutestat"]=
    "Suppress generation of RouteStat update messages on this connection.";

  proto_help_patterns["exit"]="Exit";
  proto_help_comments["exit"]="Close TCP/IP connection.";

  proto_help_patterns["gpistat"]="GPIStat <router> [<gpi-num>]";
  proto_help_comments["gpistat"]=
    "Query the state of one or more GPIs. If <gpi-num> is not given, the entire set of GPIs for the specified <router> will be returned.";

  proto_help_patterns["gpostat"]="GPOStat <router> [<gpo-num>]";
  proto_help_comments["gpostat"]=
    "Query the state of one or more GPOs. If <gpo-num> is not given, the entire set of GPOs for the specified <router> will be returned.";

  proto_help_patterns["routernames"]="RouterNames";
  proto_help_comments["routernames"]="Return a list of configured routers.";

  proto_help_patterns["routestat"]="RouteStat <router> [<output>]";
  proto_help_comments["routestat"]=
    "Return the <output> crosspoint's input assignment. If no <output> is given, the crosspoint states for all outputs on <router> will be returned.";

  proto_help_patterns["sourcenames"]="SourceNames <router>";
  proto_help_comments["sourcenames"]=
    "Return names of all inputs on the specified router.";

  proto_help_patterns["triggergpi"]=
    "TriggerGPI <router> <gpi-num> <state> [<duration>]";
  proto_help_comments["triggergpi"]=
    "Set the specified GPI to <state> for <duration> milliseconds. (Supported only by virtual GPI devices.)";

  proto_help_patterns["triggergpo"]=
    "TriggerGPO <router> <gpo-num> <state> [<duration>]";
  proto_help_comments["triggergpo"]=
    "Set the specified GPO to <state> for <duration> milliseconds.";

  proto_help_patterns["snapshots"]="SnapShots <router>";
  proto_help_comments["snapshots"]=
    "Return list of available snapshots on the specified router.";

  proto_help_patterns["snapshotroutes"]="SnapShotRoutes <router> <snap-name>";
  proto_help_comments["snapshotroutes"]=
    "Return list of routes on the specified snapshot.";
}


void ProtocolJ::AddRouteEvent(int router,int output,int input)
{
  QString sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`DATETIME`=now(),"+
    "`TYPE`='R',"+
    "`ORIGINATING_ADDRESS`='"+proto_socket->peerAddress().toString()+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",router)+
    QString::asprintf("`DESTINATION_NUMBER`=%d,",output)+
    QString::asprintf("`SOURCE_NUMBER`=%d,",input);
  if(proto_username.isEmpty()) {
    sql+="`USERNAME`=NULL";
  }
  else {
    sql+="`USERNAME`='"+DRSqlQuery::escape(proto_username)+"'";
  }
  proto_event_lookups
    [QHostInfo::lookupHost(proto_socket->peerAddress().toString(),
     this,SLOT(routeHostLookupFinishedData(const QHostInfo &)))]=
    DRSqlQuery::run(sql).toInt();
}


void ProtocolJ::AddSnapEvent(int router,const QString &name)
{
  QString sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`DATETIME`=now(),"+
    "`STATUS`='Y',"+
    "`TYPE`='S',"+
    "`ORIGINATING_ADDRESS`='"+proto_socket->peerAddress().toString()+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",router)+
    "`COMMENT`='"+tr("Executing snapshot")+" "+
    DRSqlQuery::escape("<strong>"+name+"</strong>")+" - "+
    tr("Router")+": "+QString::asprintf("<strong>%d</strong>",1+router)+"',";
  if(proto_username.isEmpty()) {
    sql+="`USERNAME`=NULL";
  }
  else {
    sql+="`USERNAME`='"+DRSqlQuery::escape(proto_username)+"'";
  }
  proto_event_lookups
    [QHostInfo::lookupHost(proto_socket->peerAddress().toString(),
     this,SLOT(snapshotHostLookupFinishedData(const QHostInfo &)))]=
    DRSqlQuery::run(sql).toInt();
}


void ProtocolJ::SendError(DRJParser::ErrorType etype,const QString &remarks)
{
  QString json="{\r\n";
  json+="    \"error\": {\r\n";
  json+=JsonField("type",(int)etype,8);
  json+=JsonField("description",DRJParser::errorString(etype),8,
		  remarks.isEmpty());
  if(!remarks.isEmpty()) {
    json+=JsonField("remarks",remarks,8,true);
  }
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


QString ProtocolJ::JsonPadding(int padding)
{
  QString ret="";

  for(int i=0;i<padding;i++) {
    ret+=" ";
  }
  return ret;
}


QString ProtocolJ::JsonEscape(const QString &str)
{
  QString ret;

  for(int i=0;i<str.length();i++) {
    QChar c=str.at(i);
    switch(c.category()) {
    case QChar::Other_Control:
      ret+=QString::asprintf("\\u%04X",c.unicode());
      break;

    default:
      switch(c.unicode()) {
      case 0x22:   // Quote
	ret+="\\\"";
	break;

      case 0x5C:   // Backslash
	ret+="\\\\";
	break;

      default:
	ret+=c;
	break;
      }
      break;
    }
  }

  return ret;
}


QString ProtocolJ::JsonNullField(const QString &name,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return ProtocolJ::JsonPadding(padding)+"\""+name+"\": null"+comma+"\r\n";
}


QString ProtocolJ::JsonField(const QString &name,bool value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  if(value) {
    return ProtocolJ::JsonPadding(padding)+"\""+name+"\": true"+comma+"\r\n";
  }
  return ProtocolJ::JsonPadding(padding)+"\""+name+"\": false"+comma+"\r\n";
}


QString ProtocolJ::JsonField(const QString &name,int value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return ProtocolJ::JsonPadding(padding)+"\""+name+"\": "+QString::asprintf("%d",value)+
    comma+"\r\n";
}


QString ProtocolJ::JsonField(const QString &name,unsigned value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return ProtocolJ::JsonPadding(padding)+"\""+name+"\": "+QString::asprintf("%u",value)+
    comma+"\r\n";
}


QString ProtocolJ::JsonField(const QString &name,const QString &value,int padding,
		    bool final)
{
  QString ret;
  QString comma=",";

  if(final) {
    comma="";
  }

  ret=JsonEscape(value);

  return JsonPadding(padding)+"\""+name+"\": \""+ret+"\""+comma+"\r\n";
}


QString ProtocolJ::JsonField(const QString &name,const QDateTime &value,int padding,
		    bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  if(!value.isValid()) {
    return JsonNullField(name,padding,final);
  }
  return JsonPadding(padding)+"\""+name+"\": \""+
    WriteXmlDateTime(value)+"\""+
    comma+"\r\n";
}


QString ProtocolJ::JsonCloseBlock(bool final)
{
  if(final) {
    return QString("}\r\n");
  }
  return QString("},\r\n");
}



//
// XML xs:date format
//
QDate ProtocolJ::ParseXmlDate(const QString &str,bool *ok)
{
  QDate ret=QDate::fromString(str,"yyyy-MM-dd");
  if(ok!=NULL) {
    *ok=ret.isValid();
  }
  return ret;
}


QString ProtocolJ::WriteXmlDate(const QDate &date)
{
  return date.toString("yyyy-MM-dd");
}


//
// XML xs:time format
//
QTime ProtocolJ::ParseXmlTime(const QString &str,bool *ok,int *day_offset)
{
  QTime ret;
  QStringList f0;
  QStringList f1;
  QStringList f2;
  int tz=0;
  QTime time;
  QTime tztime;

  if(ok!=NULL) {
    *ok=false;
  }
  if(day_offset!=NULL) {
    *day_offset=0;
  }
  f0=str.trimmed().split(" ");
  if(f0.size()!=1) {
    if(ok!=NULL) {
      *ok=false;
    }
    return ret;
  }

  if(f0[0].right(1).toLower()=="z") {  // GMT
    tz=-TimeZoneOffset();
    f0[0]=f0[0].left(f0[0].length()-1);
    f2=f0[0].split(":");
  }
  else {
    f1=f0[0].split("+");
    if(f1.size()==2) {   // GMT+
      f2=f1[1].split(":");
      if(f2.size()==2) {
	tztime=QTime(f2[0].toInt(),f2[1].toInt(),0);
	if(tztime.isValid()) {
	  tz=-TimeZoneOffset()-QTime(0,0,0).secsTo(tztime);
	}
      }
      else {
	if(ok!=NULL) {
	  *ok=false;
	}
	return QTime();
      }
    }
    else {
      f1=f0[0].split("-");
      if(f1.size()==2) {   // GMT-
	f2=f1[1].split(":");
	if(f2.size()==2) {
	  tztime=QTime(f2[0].toInt(),f2[1].toInt(),0);
	  if(tztime.isValid()) {
	    tz=-TimeZoneOffset()+QTime(0,0,0).secsTo(tztime);
	  }
	}
	else {
	  if(ok!=NULL) {
	    *ok=false;
	  }
	  return QTime();
	}
      }
    }
    f2=f1[0].split(":");
  }
  if(f2.size()==3) {
    QStringList f3=f2[2].split(".");
    time=QTime(f2[0].toInt(),f2[1].toInt(),f2[2].toInt());
    if(time.isValid()) {
      ret=time.addSecs(tz);
      if(day_offset!=NULL) {
	if((tz<0)&&((3600*time.hour()+60*time.minute()+time.second())<(-tz))) {
	  *day_offset=-1;
	}
	if((tz>0)&&(86400-((3600*time.hour()+60*time.minute()+time.second()))<tz)) {
	  *day_offset=1;
	}
      }
      if(ok!=NULL) {
	*ok=true;
      }
    }
  }
  return ret;
}


QString ProtocolJ::WriteXmlTime(const QTime &time)
{
  int utc_off=TimeZoneOffset();
  QString tz_str="-";
  if(utc_off<0) {
    tz_str="+";
  }
  tz_str+=QString().
    sprintf("%02d:%02d",utc_off/3600,(utc_off-3600*(utc_off/3600))/60);

  return time.toString("hh:mm:ss")+tz_str;
}


//
// XML xs:dateTime format
//
QDateTime ProtocolJ::ParseXmlDateTime(const QString &str,bool *ok)
{
  QDateTime ret;
  QStringList list;
  QStringList f0;
  QStringList f1;
  QStringList f2;
  int day;
  int month;
  int year;
  QTime time;
  bool lok=false;
  int day_offset=0;

  if(ok!=NULL) {
    *ok=false;
  }

  f0=str.trimmed().split(" ");
  if(f0.size()!=1) {
    if(ok!=NULL) {
      *ok=false;
    }
    return ret;
  }
  f1=f0[0].split("T");
  if(f1.size()<=2) {
    f2=f1[0].split("-");
    if(f2.size()==3) {
      year=f2[0].toInt(&lok);
      if(lok&&(year>0)) {
	month=f2[1].toInt(&lok);
	if(lok&&(month>=1)&&(month<=12)) {
	  day=f2[2].toInt(&lok);
	  if(lok&&(day>=1)&&(day<=31)) {
	    if(f1.size()==2) {
	      time=ParseXmlTime(f1[1],&lok,&day_offset);
	      if(lok) {
		ret=QDateTime(QDate(year,month,day),time).addDays(day_offset);
		if(ok!=NULL) {
		  *ok=true;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return ret;
}


QString ProtocolJ::WriteXmlDateTime(const QDateTime &dt)
{
  return WriteXmlDate(dt.date())+"T"+WriteXmlTime(dt.time());
}


int ProtocolJ::TimeZoneOffset()
{
  time_t t=time(&t);
  struct tm *tm=localtime(&t);
  time_t local_time=3600*tm->tm_hour+60*tm->tm_min+tm->tm_sec;
  tm=gmtime(&t);
  time_t gmt_time=3600*tm->tm_hour+60*tm->tm_min+tm->tm_sec;

  int offset=gmt_time-local_time;
  if(offset>43200) {
    offset=offset-86400;
  }
  if(offset<-43200) {
    offset=offset+86400;
  }

  return offset;
}
