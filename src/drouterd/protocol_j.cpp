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
#include <QJsonObject>
#include <QSqlError>
#include <QStringList>

#include <sy5/syrouting.h>

#include <drjson.h>

#include "protocol_j.h"

ProtocolJ::ProtocolJ(int sock,QObject *parent)
  : Protocol(parent)
{
  int flags;
  QString sql;

  proto_socket=NULL;
  proto_accum_quoted=false;
  proto_accum_level=0;
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
  //  proto_gpistat_masked=true;
  //  proto_gpostat_masked=true;
  //  proto_routestat_masked=true;

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
  QByteArray data;

  data=proto_socket->readAll();
  for(int i=0;i<data.length();i++) {
    switch(0xFFF&data[i]) {
    case '"':
      proto_accum_quoted=!proto_accum_quoted;
      proto_accum+=data[i];
      break;

    case '{':
      if(!proto_accum_quoted) {
	proto_accum_level++;
      }
      proto_accum+=data[i];
      break;

    case '}':
      proto_accum+=data[i];
      if(!proto_accum_quoted) {
	if(--proto_accum_level==0) {
	  QJsonDocument jdoc=QJsonDocument::fromJson(proto_accum);
	  if(jdoc.isNull()) {
	    emit parserError(DRJParser::JsonError,QString::fromUtf8(proto_accum));
	  }
	  else {
	    DispatchMessage(jdoc);
	  }
	  proto_accum.clear();
	}
      }
      break;

    default:
      proto_accum+=data[i];
      break;
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


void ProtocolJ::DispatchMessage(const QJsonDocument &jdoc)
{
  QString cmd;

  if(jdoc.object().contains("actionlist")) {
    QJsonObject jo0=jdoc.object().value("actionlist").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      SendActionInfo(router-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("activateroute")) {
    QJsonObject jo0=jdoc.object().value("activateroute").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int output=jo0.value("destination").toInt();
      if(output>0) {
	int input=jo0.value("source").toInt();
	if(input>0) {
	  ActivateRoute(router-1,output-1,input);
	}
	else {
	  SendError(DRJParser::ParameterError,
		    "missing/invalid \"source\" value");
	}
      }
      else {
	SendError(DRJParser::ParameterError,
		  "missing/invalid \"destination\" value");
      }
    }
    else {
      SendError(DRJParser::ParameterError,
		"missing/invalid \"router\" value");
    }
    return;
  }

  if(jdoc.object().contains("activatesnap")) {
    QJsonObject jo0=jdoc.object().value("activatesnap").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      QString snapshot=jo0.value("snapshot").toString();
      ActivateSnapshot(router-1,snapshot.trimmed());
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("destnames")) {
    QJsonObject jo0=jdoc.object().value("destnames").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      SendDestInfo(router-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("gpistat")) {
    QJsonObject jo0=jdoc.object().value("gpistat").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int number=jo0.value("number").toInt();
      if(number<=0) {
	SendGpiInfo(router-1,-1);
      }
      else {
	SendGpiInfo(router-1,number-1);
      }
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("gpostat")) {
    QJsonObject jo0=jdoc.object().value("gpostat").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int number=jo0.value("number").toInt();
      if(number<=0) {
	SendGpoInfo(router-1,-1);
      }
      else {
	SendGpoInfo(router-1,number-1);
      }
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("ping")) {
    SendPingResponse();
    return;
  }

  if(jdoc.object().contains("maskgpistat")) {
    QJsonObject jo0=jdoc.object().value("maskgpistat").toObject();
    MaskGpiStat(jo0.value("state").toBool());
    return;
  }

  if(jdoc.object().contains("maskgpostat")) {
    QJsonObject jo0=jdoc.object().value("maskgpostat").toObject();
    MaskGpoStat(jo0.value("state").toBool());
    return;
  }

  if(jdoc.object().contains("maskroutestat")) {
    QJsonObject jo0=jdoc.object().value("maskroutestat").toObject();
    MaskRouteStat(jo0.value("state").toBool());
    return;
  }

  if(jdoc.object().contains("maskstat")) {
    QJsonObject jo0=jdoc.object().value("maskroutestat").toObject();
    MaskStat(jo0.value("state").toBool());
    return;
  }

  if(jdoc.object().contains("routernames")) {
    QJsonObject jo0=jdoc.object().value("routernames").toObject();
    int count=0;
    QString json="{\r\n";
    json+="    \"routernames\": {\r\n";
    for(QMap<int,DREndPointMap *>::const_iterator it=proto_maps.begin();
	it!=proto_maps.end();it++) {
      json+=QString::asprintf("        \"router%d\": {\r\n",count++);
      json+=DRJsonField("number",1+it.value()->routerNumber(),12);
      json+=DRJsonField("name",it.value()->routerName(),12);
      json+=DRJsonField("type",
	      DREndPointMap::routerTypeString(it.value()->routerType()),12,true);
      json+="        "+DRJsonCloseBlock((it+1)==proto_maps.end());
    }
    json+="    }\r\n";
    json+="}\r\n";
    proto_socket->write(json.toUtf8());
    return;
  }

  if(jdoc.object().contains("routestat")) {
    QJsonObject jo0=jdoc.object().value("routestat").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int output=jo0.value("destination").toInt();
      if(output<=0) {
	SendRouteInfo(router-1,-1);
      }
      else {
	SendRouteInfo(router-1,output-1);
      }
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("snapshots")) {
    QJsonObject jo0=jdoc.object().value("snapshots").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      SendSnapshotNames(router-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("sourcenames")) {
    QJsonObject jo0=jdoc.object().value("sourcenames").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      SendSourceInfo(router-1);
    }
    else {
      SendError(DRJParser::NoRouterError);
    }
    return;
  }

  if(jdoc.object().contains("triggergpi")) {
    QJsonObject jo0=jdoc.object().value("triggergpi").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int number=jo0.value("number").toInt();
      if(number>0) {
	QString code=jo0.value("code").toString();
	if(code.length()==5) {
	  int duration=jo0.value("duration").toInt();
	  TriggerGpi(router-1,number-1,duration,code);
	}
	else {
	  SendError(DRJParser::ParameterError,
		    "missing/invalid \"code\" value");
	}
      }
      else {
	SendError(DRJParser::ParameterError,
		  "missing/invalid \"number\" value");
      }
    }
    else {
      SendError(DRJParser::ParameterError,
		"missing/invalid \"router\" value");
    }
    return;
  }

  if(jdoc.object().contains("triggergpo")) {
    QJsonObject jo0=jdoc.object().value("triggergpo").toObject();
    int router=jo0.value("router").toInt();
    if(router>0) {
      int number=jo0.value("number").toInt();
      if(number>0) {
	QString code=jo0.value("code").toString();
	if(code.length()==5) {
	  int duration=jo0.value("duration").toInt();
	  TriggerGpo(router-1,number-1,duration,code);
	}
	else {
	  SendError(DRJParser::ParameterError,
		    "missing/invalid \"code\" value");
	}
      }
      else {
	SendError(DRJParser::ParameterError,
		  "missing/invalid \"number\" value");
      }
    }
    else {
      SendError(DRJParser::ParameterError,
		"missing/invalid \"router\" value");
    }
    return;
  }

  SendError(DRJParser::NoCommandError);
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


void ProtocolJ::TriggerGpo(unsigned router,unsigned output,unsigned msecs,
			   const QString &code)
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
  json+=DRJsonField("router",1+router,8,map->snapshotQuantity()==0);
  for(int i=0;i<map->snapshotQuantity();i++) {
    json+=QString::asprintf("        \"snapshot%d\": {\r\n",i);
    json+=DRJsonField("name",map->snapshot(i)->name(),12,true);
    json+="        "+DRJsonCloseBlock(i==(map->snapshotQuantity()-1));
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
  json+=DRJsonField("router",1+router,8,q->size()<=0);
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
    QString json=DRJsonPadding(padding)+
      QString::asprintf("\"source%d\": {\r\n",q->at());
    json+=DRJsonField("number",1+q->value(0).toInt(),4+padding);
    json+=DRJsonField("name",name,4+padding);
    json+=DRJsonField("hostDescription",q->value(8).toString(),4+padding,
		    q->value(7).toUInt()!=DRConfig::LwrpMatrix);
    if(q->value(7).toUInt()==DRConfig::LwrpMatrix) {
      json+=DRJsonField("hostAddress",q->value(2).toString(),4+padding);
      json+=DRJsonField("hostName",q->value(3).toString(),4+padding);
      json+=DRJsonField("slot",1+q->value(4).toInt(),4+padding);
      json+=DRJsonField("sourceNumber",
	       SyRouting::livewireNumber(QHostAddress(q->value(5).toString())),
		    4+padding);
      json+=DRJsonField("streamAddress",q->value(5).toString(),4+padding,true);
    }
    json+=DRJsonPadding(padding)+"}";
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
  QString json=DRJsonPadding(padding)+
    QString::asprintf("\"source%d\": {\r\n",q->at());
  json+=DRJsonField("number",1+q->value(0).toInt(),4+padding);
  json+=DRJsonField("name",name,4+padding);
  json+=DRJsonField("hostDescription",q->value(6).toString(),4+padding,
		  q->value(5).toUInt()!=DRConfig::LwrpMatrix);
  if(q->value(5).toUInt()==DRConfig::LwrpMatrix) {
    json+=DRJsonField("hostAddress",q->value(1).toString(),4+padding);
    json+=DRJsonField("hostName",q->value(2).toString(),4+padding);
    json+=DRJsonField("slot",1+q->value(3).toInt(),4+padding);
    json+=DRJsonField("gpioAddress",q->value(1).toString()+
		    QString::asprintf("/%d",q->value(3).toInt()+1),4+padding,
		    true);
  }
  json+=DRJsonPadding(padding)+"}";
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
  json+=DRJsonField("router",1+router,8,q->size()<=0);
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

  QString json=DRJsonPadding(padding)+
    QString::asprintf("\"destination%d\": {\r\n",q->at());
  json+=DRJsonField("number",1+q->value(0).toInt(),4+padding);
  json+=DRJsonField("name",name,4+padding);
  json+=DRJsonField("hostDescription",q->value(7).toString(),4+padding,
		  q->value(6).toUInt()!=DRConfig::LwrpMatrix);
  if(q->value(6).toUInt()==DRConfig::LwrpMatrix) {
    json+=DRJsonField("hostAddress",q->value(2).toString(),4+padding);
    json+=DRJsonField("hostName",q->value(3).toString(),4+padding);
    json+=DRJsonField("slot",1+q->value(4).toInt(),4+padding,true);
  }
  json+=DRJsonPadding(padding)+"}";
  if(!final) {
    json+=",";
  }
  json+="\r\n";

  return json;
}


void ProtocolJ::SendActionInfo(unsigned router)
{
  DREndPointMap *map;
  QString sql;
  DRSqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    SendError(DRJParser::NoRouterError);
    return;
  }
  sql=ActionListSqlFields()+"where "+
    QString::asprintf("`PERM_SA_ACTIONS`.`ROUTER_NUMBER`=%u ",router)+
    "order by `PERM_SA_ACTIONS`.`DESTINATION_NUMBER`";
  q=new DRSqlQuery(sql);
  int quan=0;
  QString json="{\r\n";
  json+="    \"actionlist\": {\r\n";
  json+=DRJsonField("router",1+router,8,q->size()<=0);
  while(q->next()) {
    quan++;
    json+=ActionListMessage(q,8,quan==q->size());
  }
  delete q;
  json+="    }\r\n";
  json+="}\r\n";
  proto_socket->write(json.toUtf8());
}


QString ProtocolJ::ActionListSqlFields() const
{
  return QString("select ")+
    "`PERM_SA_ACTIONS`.`ID`,"+                  // 00
    "`PERM_SA_ACTIONS`.`TIME`,"+                // 01
    "`PERM_SA_ACTIONS`.`SUN`,"+                 // 02
    "`PERM_SA_ACTIONS`.`MON`,"+                 // 03
    "`PERM_SA_ACTIONS`.`TUE`,"+                 // 04
    "`PERM_SA_ACTIONS`.`WED`,"+                 // 05
    "`PERM_SA_ACTIONS`.`THU`,"+                 // 06
    "`PERM_SA_ACTIONS`.`FRI`,"+                 // 07
    "`PERM_SA_ACTIONS`.`SAT`,"+                 // 08
    "`PERM_SA_ACTIONS`.`ROUTER_NUMBER`,"+       // 09
    "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`,"+  // 10
    "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`,"+       // 11
    "`PERM_SA_ACTIONS`.`COMMENT` "+             // 12
    "from `PERM_SA_ACTIONS` ";
}


QString ProtocolJ::ActionListMessage(DRSqlQuery *q,int padding,bool final)
{
  QString json=DRJsonPadding(padding)+
    QString::asprintf("\"action%d\": {\r\n",q->at());
  json+=DRJsonField("id",q->value(0).toInt(),4+padding);
  json+=DRJsonField("time",q->value(1).toTime(),0,4+padding);
  json+=DRJsonField("sunday",q->value(2).toString()=="Y",4+padding);
  json+=DRJsonField("monday",q->value(3).toString()=="Y",4+padding);
  json+=DRJsonField("tuesday",q->value(4).toString()=="Y",4+padding);
  json+=DRJsonField("wednesday",q->value(5).toString()=="Y",4+padding);
  json+=DRJsonField("thursday",q->value(6).toString()=="Y",4+padding);
  json+=DRJsonField("friday",q->value(7).toString()=="Y",4+padding);
  json+=DRJsonField("saturday",q->value(8).toString()=="Y",4+padding);
  json+=DRJsonField("destination",1+q->value(10).toInt(),4+padding);
  json+=DRJsonField("source",1+q->value(11).toInt(),4+padding);
  json+=DRJsonField("comment",q->value(12).toString(),4+padding,true);
  json+=DRJsonPadding(padding)+"}";
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
  json+=DRJsonField("router",1+q->value(0).toInt(),8);
  json+=DRJsonField("source",1+q->value(1).toInt(),8);
  json+=DRJsonField("code",q->value(2).toString(),8,true);
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
  json+=DRJsonField("router",1+q->value(0).toInt(),8);
  json+=DRJsonField("destination",1+q->value(1).toInt(),8);
  json+=DRJsonField("code",q->value(2).toString(),8,true);
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
  json+=DRJsonField("router",1+router,8);
  json+=DRJsonField("destination",1+output,8);
  json+=DRJsonField("source",input,8,true);
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
  json+=DRJsonField("state",state,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::MaskGpoStat(bool state)
{
  proto_gpostat_masked=state;

  QString json="{\r\n";
  json+="    \"maskgpostat\": {\r\n";
  json+=DRJsonField("state",state,8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}


void ProtocolJ::MaskRouteStat(bool state)
{
  proto_routestat_masked=state;

  QString json="{\r\n";
  json+="    \"maskroutestat\": {\r\n";
  json+=DRJsonField("state",state,8,true);
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
      json+=DRJsonNullField("keyword",8);
      json+=DRJsonField("pattern",proto_help_patterns.value(""),8,true);
      json+=DRJsonField("comment",proto_help_comments.value(""),8,true);
    }
    else {
      if(proto_help_patterns.value(keyword).isEmpty()) {
	json+=DRJsonNullField("pattern",8);
      }
      else {
	json+=DRJsonField("pattern",proto_help_patterns.value(keyword),8);
      }
      if(proto_help_comments.value(keyword).isEmpty()) {
	json+=DRJsonNullField("comment",8);
      }
      else {
	json+=DRJsonField("comment",proto_help_comments.value(keyword),8,true);
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
  json+=DRJsonField("datetime",QDateTime::currentDateTime(),8,true);
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
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
  proto_help_patterns[""]=QString("ActionList")+
    ", ActivateRoute"+
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

  proto_help_patterns["actionlist"]="ActionList <router>";
  proto_help_comments["actionlist"]=
    "Return a list of scheduled crosspoint changes for the specified router.";

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
  json+=DRJsonField("type",(int)etype,8);
  json+=DRJsonField("description",DRJParser::errorString(etype),8,
		  remarks.isEmpty());
  if(!remarks.isEmpty()) {
    json+=DRJsonField("remarks",remarks,8,true);
  }
  json+="    }\r\n";
  json+="}\r\n";

  proto_socket->write(json.toUtf8());
}
