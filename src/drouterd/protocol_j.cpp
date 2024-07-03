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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
  proto_accum_quoted=false;
  proto_accum_level=0;
  proto_destinations_subscribed=false;
  proto_gpis_subscribed=false;
  proto_gpos_subscribed=false;
  proto_nodes_subscribed=false;
  proto_sources_subscribed=false;
  proto_clips_subscribed=false;
  proto_silences_subscribed=false;
  proto_eventstat_subscribeds=false;
  proto_gpistat_masked=false;
  proto_gpostat_masked=false;
  proto_routestat_masked=false;
  //  proto_gpistat_masked=true;
  //  proto_gpostat_masked=true;
  //  proto_routestat_masked=true;

  openlog("dprotod(J)",LOG_PID,LOG_DAEMON);

  //
  // Event Logger
  //
  proto_logger=new LoggerFront(this);
  connect(proto_logger,SIGNAL(eventAdded(int)),this,SLOT(eventAddedData(int)));

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


void ProtocolJ::eventAddedData(int evt_id)
{
  addEvent(evt_id);
}


void ProtocolJ::disconnectedData()
{
  quit();
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


void ProtocolJ::actionChanged(int action_id)
{
  //  printf("ProtocolJ:actionChanged(id: %d)\n",action_id);

  QString sql;
  DRSqlQuery *q;
  int router=-1;

  //
  // FIXME: Get rid of this kludgy double lookup!
  //
  sql=QString("select ")+
    "`ROUTER_NUMBER` "+  // 00
    "from `PERM_SA_ACTIONS` where "+
    QString::asprintf("`ID`=%d",action_id);
  q=new DRSqlQuery(sql);
  if(q->first()) {
    router=q->value(0).toInt();
  }
  else {
    QJsonObject jo0;
    jo0.insert("id",action_id);

    QJsonObject jo1;
    jo1.insert("actiondelete",jo0);
    QJsonDocument jdoc;
    jdoc.setObject(jo1);

    proto_socket->write(jdoc.toJson());
    return;
  }

  sql=ActionListSqlFields(proto_maps.value(router)->routerType())+"where "+
    QString::asprintf("`PERM_SA_ACTIONS`.`ID`=%d ",action_id)+
    "order by `PERM_SA_ACTIONS`.`DESTINATION_NUMBER`";
  q=new DRSqlQuery(sql);
  if(q->first()) {
    if(proto_maps.value(q->value(10).toInt())!=NULL) {
      QJsonObject jo0;
      jo0.insert("router",QJsonValue((int)(1+router)));
      jo0.insert(QString::asprintf("action%d",q->at()),ActionListMessage(q));

      QJsonObject jo1;
      jo1.insert("actionlist",jo0);
      QJsonDocument jdoc;
      jdoc.setObject(jo1);

      proto_socket->write(jdoc.toJson());
    }
  }
}


void ProtocolJ::nextActionsChanged(int router,const QList<int> &action_ids)
{
  if(proto_actionstat_subscribeds.value(router)) {
    SendActionStat(router,action_ids);
  }
}


void ProtocolJ::eventAdded(int evt_id)
{
  if(proto_eventstat_subscribeds) {
    QString sql=EventStatSqlFields()+
      QString::asprintf("where `PERM_SA_EVENTS`.`ID`=%d",evt_id);
    DRSqlQuery *q=new DRSqlQuery(sql);
    while(q->next()) {
      SendEventStat(q);
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
  QString err_msg;

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

  if(jdoc.object().contains("actiondelete")) {
    QVariantMap map=jdoc.object().value("actiondelete").toObject().
      toVariantMap();
    int id=map.value("id").toInt();
    if(id>0) {
      DeleteAction(id);
    }
    return;
  }

  if(jdoc.object().contains("actionedit")) {
    QVariantMap map=jdoc.object().value("actionedit").toObject().toVariantMap();

    //
    // Validate Time
    //
    QTime time=FromJsonString(map.value("time").toString());
    if(time.isNull()) {
      SendError(DRJParser::TimeError);
      return;
    }

    QString sql;
    bool ok=false;
    int id=map.value("id").toInt();
    if(id<0) {  // Add New Action
      sql=QString("insert into `PERM_SA_ACTIONS` set ")+
	ActionEditSqlFields(map,time);
      int new_id=DRSqlQuery::run(sql,&ok).toInt();
      if(ok) {
	refreshAction(new_id);
      }
      else {
	SendError(DRJParser::DatabaseError,err_msg);
      }
    }
    else {  // Modify Existing Action
      sql=QString("update `PERM_SA_ACTIONS` set ")+
	ActionEditSqlFields(map,time)+
	QString::asprintf("where `ID`=%d ",map.value("id").toInt());
      if(DRSqlQuery::apply(sql,&err_msg)) {
	refreshAction(map.value("id").toInt());
      }
      else {
	SendError(DRJParser::DatabaseError,err_msg);
      }
    }
    return;
  }

  if(jdoc.object().contains("actionstat")) {
    QVariantMap map=jdoc.object().value("actionstat").toObject().
      toVariantMap();
    int router=map.value("router").toInt();
    proto_actionstat_subscribeds[router-1]=map.value("sendUpdates").toBool();
    SendActionStatInfo(router-1);
    return;
  }

  if(jdoc.object().contains("eventstat")) {
    QVariantMap map=jdoc.object().value("eventstat").toObject().
      toVariantMap();
    proto_eventstat_subscribeds=map.value("sendUpdates").toBool();
    SendEventStatInfo();
    return;
  }
  /*
  if(jdoc.object().contains("routernames")) {
    int count=0;
    QJsonObject jo1;
    for(QMap<int,DREndPointMap *>::const_iterator it=proto_maps.begin();
	it!=proto_maps.end();it++) {
      QJsonObject jo0;
      
      jo0.insert("number",QJsonValue((int)(1+it.value()->routerNumber())));
      jo0.insert("name",QJsonValue(it.value()->routerName()));
      jo0.insert("type",QJsonValue(DREndPointMap::routerTypeString(it.value()->
				   routerType())));
      jo1.insert(QString::asprintf("router%d",count),jo0);
      count++;
    }
    QJsonObject jo2;
    jo2.insert("routernames",jo1);
    QJsonDocument jdoc;
    jdoc.setObject(jo2);
    proto_socket->write(jdoc.toJson());

    return;
  }
  */
  if(jdoc.object().contains("routernames")) {
    QJsonArray ja0;
    for(QMap<int,DREndPointMap *>::const_iterator it=proto_maps.begin();
	it!=proto_maps.end();it++) {
      QJsonObject jo0;
      jo0.insert("number",QJsonValue((int)(1+it.value()->routerNumber())));
      jo0.insert("name",QJsonValue(it.value()->routerName()));
      jo0.insert("type",QJsonValue(DREndPointMap::routerTypeString(it.value()->
				   routerType())));
      ja0.append(jo0);
    }
    QJsonObject jo1;
    jo1.insert("routernames",ja0);
    QJsonDocument jdoc;
    jdoc.setObject(jo1);
    proto_socket->write(jdoc.toJson());

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

  //  AddRouteEvent(router,output,input-1);
  proto_logger->addRouteEvent(proto_socket->peerAddress(),proto_username,
			      router,output,input-1);
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


void ProtocolJ::TriggerGpi(unsigned router,unsigned input,unsigned duration,
			   const QString &code)
{
  DREndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==DREndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(DREndPointMap::Input,input);
      int slotnum=map->slot(DREndPointMap::Input,input);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpiState(addr,slotnum,code,duration);
	syslog(LOG_INFO,
	       "set gpi state router: %d  input: %d to state: %s from %s",
	       router+1,input+1,code.toUtf8().constData(),
	       proto_socket->peerAddress().toString().toUtf8().constData());
      }
    }
  }
}


void ProtocolJ::TriggerGpo(unsigned router,unsigned output,unsigned duration,
			   const QString &code)
{
  DREndPointMap *map;

  if((map=proto_maps.value(router))!=NULL) {
    if(map->routerType()==DREndPointMap::GpioRouter) {
      QHostAddress addr=map->hostAddress(DREndPointMap::Output,output);
      int slotnum=map->slot(DREndPointMap::Output,output);
      if((!addr.isNull())&&(slotnum>=0)) {
	setGpoState(addr,slotnum,code,duration);
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

  QJsonObject jo1;
  jo1.insert("router",QJsonValue((int)(1+router)));
  for(int i=0;i<map->snapshotQuantity();i++) {
    QJsonObject jo0;
    jo0.insert("name",QJsonValue(map->snapshot(i)->name()));
    jo1.insert(QString::asprintf("snapshot%d",i),jo0);
  }
  QJsonObject jo2;
  jo2.insert("snapshots",jo1);
  QJsonDocument jdoc;
  jdoc.setObject(jo2);

  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::SendSnapshotRoutes(unsigned router,const QString &snap_name)
{
  /*
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
  */
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
    proto_logger->addSnapEvent(proto_socket->peerAddress(),proto_username,
			       router,snapshot_name);
    for(int i=0;i<ss->routeQuantity();i++) {
      ActivateRoute(router,ss->routeOutput(i)-1,ss->routeInput(i));
    }
  }
  syslog(LOG_INFO,"activated snapshot %d:%s from %s",router+1,
	 snapshot_name.toUtf8().constData(),
	 proto_socket->peerAddress().toString().toUtf8().constData());
}

/*
void ProtocolJ::SendSourceInfo(unsigned router)
{
  QJsonObject jo0;
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
  jo0.insert("router",QJsonValue((int)(1+router)));
  q=new DRSqlQuery(sql);
  while(q->next()) {
    jo0.insert(QString::asprintf("source%d",q->at()),
	       SourceNamesMessage(map->routerType(),q));
  }
  QJsonObject jo1;
  jo1.insert("sourcenames",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}
*/

void ProtocolJ::SendSourceInfo(unsigned router)
{
  QJsonObject jo0;
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
  jo0.insert("router",QJsonValue((int)(1+router)));
  QJsonArray ja0;
  q=new DRSqlQuery(sql);
  while(q->next()) {
    ja0.append(SourceNamesMessage(map->routerType(),q));
  }
  jo0.insert("sourcenames",ja0);

  QJsonDocument jdoc;
  jdoc.setObject(jo0);

  proto_socket->write(jdoc.toJson());
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


QJsonObject ProtocolJ::SourceNamesMessage(DREndPointMap::RouterType type,
					  DRSqlQuery *q)
{
  QString json;
  QJsonObject jo0;

  if(type==DREndPointMap::AudioRouter) {
    QString name=q->value(1).toString();
    if(!q->value(6).toString().isEmpty()) {
      name=q->value(6).toString();
    }
    jo0.insert("number",(int)(1+q->value(0).toInt()));
    jo0.insert("name",name);
    jo0.insert("hostDescription",q->value(8).toString());
    if(q->value(7).toUInt()==Config::LwrpMatrix) {
      jo0.insert("hostAddress",q->value(2).toString());
      jo0.insert("hostName",q->value(3).toString());
      jo0.insert("slot",1+q->value(4).toInt());
      jo0.insert("sourceNumber",
		 (int)SyRouting::livewireNumber(QHostAddress(q->value(5).
							     toString())));
      jo0.insert("streamAddress",q->value(5).toString());
    }
  }
  else {
    QString name="GPI";
    if(!q->value(4).toString().isEmpty()) {
      name=q->value(4).toString();
    }
    jo0.insert("number",(int)(1+q->value(0).toInt()));
    jo0.insert("name",name);
    jo0.insert("hostDescription",q->value(6).toString());
    if(q->value(5).toUInt()==Config::LwrpMatrix) {
      jo0.insert("hostAddress",q->value(1).toString());
      jo0.insert("hostName",q->value(2).toString());
      jo0.insert("slot",1+q->value(3).toInt());
      jo0.insert("gpioAddress",q->value(1).toString()+
		 QString::asprintf("/%d",q->value(3).toInt()+1));
    }
  }

  return jo0;
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
  QJsonObject jo0;
  q=new DRSqlQuery(sql);
  while(q->next()) {
    jo0.insert(QString::asprintf("destination%d",q->at()),
	       DestNamesMessage(map->routerType(),q));
  }
  QJsonObject jo1;
  jo0.insert("router",QJsonValue((int)(1+router)));
  jo1.insert("destnames",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
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


QJsonObject ProtocolJ::DestNamesMessage(DREndPointMap::RouterType type,
					DRSqlQuery *q)
{
  QString json;
  QJsonObject jo0;
  QString name=q->value(1).toString();
  if(!q->value(5).toString().isEmpty()) {
    name=q->value(5).toString();
  }
  jo0.insert("number",(int)(1+q->value(0).toInt()));
  jo0.insert("name",name);
  jo0.insert("hostDescription",q->value(7).toString());
  if(q->value(6).toUInt()==Config::LwrpMatrix) {
    jo0.insert("hostAddress",q->value(2).toString());
    jo0.insert("hostName",q->value(3).toString());
    jo0.insert("slot",1+q->value(4).toInt());
  }

  return jo0;
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
  sql=ActionListSqlFields(map->routerType())+"where "+
    QString::asprintf("`PERM_SA_ACTIONS`.`ROUTER_NUMBER`=%u ",router)+
    "order by `PERM_SA_ACTIONS`.`DESTINATION_NUMBER`";
  q=new DRSqlQuery(sql);
  QJsonObject jo0;
  jo0.insert("router",QJsonValue((int)(1+router)));
  while(q->next()) {
    jo0.insert(QString::asprintf("action%d",q->at()),ActionListMessage(q));
  }
  QJsonObject jo1;
  jo1.insert("actionlist",jo0);
  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


QString ProtocolJ::ActionListSqlFields(DREndPointMap::RouterType type) const
{
  if(type==DREndPointMap::AudioRouter) {
    return QString("select ")+
      "`PERM_SA_ACTIONS`.`ID`,"+                  // 00
      "`PERM_SA_ACTIONS`.`IS_ACTIVE`,"+           // 01
      "`PERM_SA_ACTIONS`.`TIME`,"+                // 02
      "`PERM_SA_ACTIONS`.`SUN`,"+                 // 03
      "`PERM_SA_ACTIONS`.`MON`,"+                 // 04
      "`PERM_SA_ACTIONS`.`TUE`,"+                 // 05
      "`PERM_SA_ACTIONS`.`WED`,"+                 // 06
      "`PERM_SA_ACTIONS`.`THU`,"+                 // 07
      "`PERM_SA_ACTIONS`.`FRI`,"+                 // 08
      "`PERM_SA_ACTIONS`.`SAT`,"+                 // 09
      "`PERM_SA_ACTIONS`.`ROUTER_NUMBER`,"+       // 10
      "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`,"+  // 11
      "`SA_DESTINATIONS`.`NAME`,"+                // 12
      "`SA_DESTINATIONS`.`HOST_ADDRESS`,"+        // 13
      "`DST_NODES`.`HOST_NAME`,"+                 // 14
      "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`,"+       // 15
      "`SA_SOURCES`.`NAME`,"+                     // 16
      "`SA_SOURCES`.`HOST_ADDRESS`,"+             // 17
      "`SRC_NODES`.`HOST_NAME`,"+                 // 18
      "`PERM_SA_ACTIONS`.`COMMENT` "+             // 19
      "from `PERM_SA_ACTIONS` "+
      "left join `SA_SOURCES` "+
      "on `PERM_SA_ACTIONS`.`ROUTER_NUMBER`=`SA_SOURCES`.`ROUTER_NUMBER` && "+
      "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`=`SA_SOURCES`.`SOURCE_NUMBER` "+
      "left join `SA_DESTINATIONS` "+
      "on `PERM_SA_ACTIONS`.`ROUTER_NUMBER`=`SA_DESTINATIONS`.`ROUTER_NUMBER` && "+
      "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`=`SA_DESTINATIONS`.`SOURCE_NUMBER` "+
      "left join `NODES` as `SRC_NODES` "+
      "on `SA_SOURCES`.`HOST_ADDRESS`=`SRC_NODES`.`HOST_ADDRESS` "+
      "left join `NODES` as `DST_NODES` "+
      "on `SA_DESTINATIONS`.`HOST_ADDRESS`=`DST_NODES`.`HOST_ADDRESS` ";
  }
  else {
    return QString("select ")+
      "`PERM_SA_ACTIONS`.`ID`,"+                  // 00
      "`PERM_SA_ACTIONS`.`IS_ACTIVE`,"+           // 01
      "`PERM_SA_ACTIONS`.`TIME`,"+                // 02
      "`PERM_SA_ACTIONS`.`SUN`,"+                 // 03
      "`PERM_SA_ACTIONS`.`MON`,"+                 // 04
      "`PERM_SA_ACTIONS`.`TUE`,"+                 // 05
      "`PERM_SA_ACTIONS`.`WED`,"+                 // 06
      "`PERM_SA_ACTIONS`.`THU`,"+                 // 07
      "`PERM_SA_ACTIONS`.`FRI`,"+                 // 08
      "`PERM_SA_ACTIONS`.`SAT`,"+                 // 09
      "`PERM_SA_ACTIONS`.`ROUTER_NUMBER`,"+       // 10
      "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`,"+  // 11
      "`SA_GPOS`.`NAME`,"+                        // 12
      "`SA_GPOS`.`HOST_ADDRESS`,"+                // 13
      "`DST_NODES`.`HOST_NAME`,"+                 // 14
      "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`,"+       // 15
      "`SA_GPIS`.`NAME`,"+                        // 16
      "`SA_GPIS`.`HOST_ADDRESS`,"+                // 17
      "`SRC_NODES`.`HOST_NAME`,"+                 // 18
      "`PERM_SA_ACTIONS`.`COMMENT` "+             // 19
      "from `PERM_SA_ACTIONS` "+
      "left join `SA_GPIS` "+
      "on `PERM_SA_ACTIONS`.`ROUTER_NUMBER`=`SA_GPIS`.`ROUTER_NUMBER` && "+
      "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`=`SA_GPIS`.`SOURCE_NUMBER` "+
      "left join `SA_GPOS` "+
      "on `PERM_SA_ACTIONS`.`ROUTER_NUMBER`=`SA_GPOS`.`ROUTER_NUMBER` && "+
      "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`=`SA_GPOS`.`SOURCE_NUMBER` "+
      "left join `NODES` as `SRC_NODES` "+
      "on `SA_GPIS`.`HOST_ADDRESS`=`SRC_NODES`.`HOST_ADDRESS` "+
      "left join `NODES` as `DST_NODES` "+
      "on `SA_GPOS`.`HOST_ADDRESS`=`DST_NODES`.`HOST_ADDRESS` ";
  }
}



QJsonObject ProtocolJ::ActionListMessage(DRSqlQuery *q)
{
  QString json;
  QJsonObject jo0;
  jo0.insert("id",q->value(0).toInt());
  jo0.insert("isActive",q->value(1).toString()=="Y");
  jo0.insert("time",q->value(2).toTime().toString("hh:mm:ss"));
  jo0.insert("sunday",q->value(3).toString()=="Y");
  jo0.insert("monday",q->value(4).toString()=="Y");
  jo0.insert("tuesday",q->value(5).toString()=="Y");
  jo0.insert("wednesday",q->value(6).toString()=="Y");
  jo0.insert("thursday",q->value(7).toString()=="Y");
  jo0.insert("friday",q->value(8).toString()=="Y");
  jo0.insert("saturday",q->value(9).toString()=="Y");
  jo0.insert("destination",1+q->value(11).toInt());
  jo0.insert("destinationName",q->value(12).toString());
  jo0.insert("destinationHostAddress",q->value(13).toString());
  jo0.insert("destinationHostName",q->value(14).toString());
  jo0.insert("source",1+q->value(15).toInt());
  jo0.insert("sourceName",q->value(16).toString());
  jo0.insert("sourceHostAddress",q->value(17).toString());
  jo0.insert("sourceHostName",q->value(18).toString());
  jo0.insert("comment",q->value(19).toString());

  return jo0;
}


QString ProtocolJ::ActionEditSqlFields(const QVariantMap &fields,
				       const QTime &time) const
{
  QString sql="`IS_ACTIVE`='"+ToYesNo(fields.value("isActive").toBool())+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",fields.value("router").toInt()-1)+
    QString::asprintf("`DESTINATION_NUMBER`=%d,",
		      fields.value("destination").toInt()-1)+
    QString::asprintf("`SOURCE_NUMBER`=%d,",fields.value("source").toInt()-1)+
    "`TIME`='"+DRSqlQuery::escape(time.toString("hh:mm:ss"))+"',"+
    "`COMMENT`='"+DRSqlQuery::escape(fields.value("comment").toString())+"',"+
    "`SUN`='"+ToYesNo(fields.value("sunday").toBool())+"',"+
    "`MON`='"+ToYesNo(fields.value("monday").toBool())+"',"+
    "`TUE`='"+ToYesNo(fields.value("tuesday").toBool())+"',"+
    "`WED`='"+ToYesNo(fields.value("wednesday").toBool())+"',"+
    "`THU`='"+ToYesNo(fields.value("thursday").toBool())+"',"+
    "`FRI`='"+ToYesNo(fields.value("friday").toBool())+"',"+
    "`SAT`='"+ToYesNo(fields.value("saturday").toBool())+"' ";
  return sql;
}


void ProtocolJ::SendActionStatInfo(int router)
{
  QList<int> ids;
  QString sql=ActionStatSqlFields()+
    "where "+
    QString::asprintf("`SA_NEXT_ACTIONS`.`ROUTER_NUMBER`=%d ",router);
  DRSqlQuery *q=new DRSqlQuery(sql);
  while(q->next()) {
    ids.push_back(q->value(0).toInt());
  }
  delete q;
  SendActionStat(router,ids);
}

QString ProtocolJ::ActionStatSqlFields() const
{
  return QString("select ")+
    "`SA_NEXT_ACTIONS`.`ACTION_ID` "+  // 00
    "from `SA_NEXT_ACTIONS` ";
}


void ProtocolJ::SendActionStat(int router,const QList<int> &action_ids)
{
  QJsonObject jo0;
  jo0.insert("router",1+router);
  jo0.insert("sendUpdates",proto_actionstat_subscribeds.value(router));
  QJsonArray ids;
  for(int i=0;i<action_ids.size();i++) {
    ids.append(action_ids.at(i));
  }
  jo0.insert("nextId",ids);

  QJsonObject jo1;
  jo1.insert("actionstat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);
  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::SendEventStatInfo()
{
  QString sql=EventStatSqlFields()+
    "order by `PERM_SA_EVENTS`.`DATETIME`";
  DRSqlQuery *q=new DRSqlQuery(sql);
  while(q->next()) {
    SendEventStat(q);
  }
  delete q;
}


QString ProtocolJ::EventStatSqlFields() const
{
  return QString("select ")+
    "`PERM_SA_EVENTS`.`ID`,"+                   // 00
    "`PERM_SA_EVENTS`.`STATUS`,"+               // 01
    "`PERM_SA_EVENTS`.`TYPE`,"+                 // 02
    "`PERM_SA_EVENTS`.`DATETIME`,"+             // 03
    "`PERM_SA_EVENTS`.`USERNAME`,"+             // 04
    "`PERM_SA_EVENTS`.`HOSTNAME`,"+             // 05
    "`PERM_SA_EVENTS`.`ORIGINATING_ADDRESS`,"+  // 06
    "`PERM_SA_EVENTS`.`COMMENT`,"+              // 07
    "`PERM_SA_EVENTS`.`ROUTER_NUMBER`,"+        // 08
    "`PERM_SA_EVENTS`.`ROUTER_NAME`,"+          // 09
    "`PERM_SA_EVENTS`.`SOURCE_NUMBER`,"+        // 10
    "`PERM_SA_EVENTS`.`SOURCE_NAME`,"+          // 11
    "`PERM_SA_EVENTS`.`DESTINATION_NUMBER`,"+   // 12
    "`PERM_SA_EVENTS`.`DESTINATION_NAME` "+     // 13
    "from `PERM_SA_EVENTS` ";
}


void ProtocolJ::SendEventStat(DRSqlQuery *q)
{
  DRJParser::EventType type=
    (DRJParser::EventType)q->value(2).toString().at(0).cell();

  QJsonObject jo0;
  jo0.insert("sendUpdates",proto_eventstat_subscribeds);
  jo0.insert("id",q->value(0).toInt());
  jo0.insert("datetime",
	     q->value(3).toDateTime().toString("yyyy-MM-ddThh:mm:ss"));
  jo0.insert("type",DRJParser::eventTypeString(type));
  jo0.insert("status",q->value(1).toString()=="Y");
  jo0.insert("comment",q->value(7).toString());
  if(type==DRJParser::CommentEvent) {
    jo0.insert("router",QJsonValue());
    jo0.insert("routerName",QJsonValue());
    jo0.insert("source",QJsonValue());
    jo0.insert("sourceName",QJsonValue());
    jo0.insert("destination",QJsonValue());
    jo0.insert("destinationName",QJsonValue());
  }
  else {
    jo0.insert("router",1+q->value(8).toInt());
    jo0.insert("routerName",q->value(9).toString());
    jo0.insert("source",1+q->value(10).toInt());
    jo0.insert("sourceName",q->value(11).toString());
    jo0.insert("destination",1+q->value(12).toInt());
    jo0.insert("destinationName",q->value(13).toString());
  }

  QJsonObject jo1;
  jo1.insert("eventstat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::DeleteAction(int id)
{
  QString sql=QString("delete from `PERM_SA_ACTIONS` where ")+
    QString::asprintf("`ID`=%d",id);
  if(DRSqlQuery::apply(sql)) {
    refreshAction(id);
  }
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
  QJsonObject jo0;
  jo0.insert("router",QJsonValue((int)(1+q->value(0).toInt())));
  jo0.insert("source",QJsonValue((int)(1+q->value(1).toInt())));
  jo0.insert("code",QJsonValue(q->value(2).toString()));

  QJsonObject jo1;
  jo1.insert("gpistat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  return jdoc.toJson();
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
  QJsonObject jo0;
  jo0.insert("router",QJsonValue((int)(1+q->value(0).toInt())));
  jo0.insert("destination",QJsonValue((int)(1+q->value(1).toInt())));
  jo0.insert("code",QJsonValue(q->value(2).toString()));

  QJsonObject jo1;
  jo1.insert("gpostat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  return jdoc.toJson();
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
  QJsonObject jo0;

  jo0.insert("router",QJsonValue((int)(1+router)));
  jo0.insert("destination",QJsonValue((int)(1+output)));
  jo0.insert("source",QJsonValue((int)input));

  QJsonObject jo1;
  jo1.insert("routestat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  return jdoc.toJson();
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

  QJsonObject jo0;
  jo0.insert("state",QJsonValue(state));

  QJsonObject jo1;
  jo1.insert("maskgpistat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::MaskGpoStat(bool state)
{
  proto_gpostat_masked=state;

  QJsonObject jo0;
  jo0.insert("state",QJsonValue(state));

  QJsonObject jo1;
  jo1.insert("maskgpostat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::MaskRouteStat(bool state)
{
  proto_routestat_masked=state;

  QJsonObject jo0;
  jo0.insert("state",QJsonValue(state));

  QJsonObject jo1;
  jo1.insert("maskroutestat",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


void ProtocolJ::MaskStat(bool state)
{
  MaskGpiStat(state);
  MaskGpoStat(state);
  MaskRouteStat(state);
}


void ProtocolJ::SendPingResponse()
{
  QJsonObject jo0;
  jo0.insert("datetime",QJsonValue(QDateTime::currentDateTime().
				   toString("yyyy-MM-ddThh:mm:ss")));

  QJsonObject jo1;
  jo1.insert("pong",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
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


void ProtocolJ::SendError(DRJParser::ErrorType etype,const QString &remarks)
{
  QString desc=QJsonValue(DRJParser::errorString(etype)).toString();
  if(!remarks.isEmpty()) {
    desc+="\n\n"+remarks;
  }

  QJsonObject jo0;
  jo0.insert("type",QJsonValue((int)etype));
  jo0.insert("description",desc);

  QJsonObject jo1;
  jo1.insert("error",jo0);

  QJsonDocument jdoc;
  jdoc.setObject(jo1);

  proto_socket->write(jdoc.toJson());
}


QTime ProtocolJ::FromJsonString(const QString &str) const
{
  return QTime::fromString(str.left(8),"hh:mm:ss");
}


QString ProtocolJ::ToYesNo(bool state) const
{
  if(state) {
    return "Y";
  }
  return "N";
}
  
