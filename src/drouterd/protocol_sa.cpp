// protocol_sa.cpp
//
// Software Authority protocol handler for DRouter.
//
//   (C) Copyright 2018-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QSqlError>
#include <QStringList>

#include <sy5/syrouting.h>

#include "protocol_sa.h"

ProtocolSa::ProtocolSa(int sock,QObject *parent)
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

  openlog("dprotod(SA)",LOG_PID,LOG_DAEMON);

  //
  // The ProtocolSa Server
  //
  proto_server=new QTcpServer(this);
  connect(proto_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));
  if(sock<0) {
    proto_server->listen(QHostAddress::Any,9500);
  }
  else {
    proto_server->setSocketDescriptor(sock);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_server->socketDescriptor(),F_SETFD,&flags))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",(const char *)strerror(errno));
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
    syslog(LOG_ERR,"socket error [%s], aborting",(const char *)strerror(errno));
    exit(1);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_socket->socketDescriptor(),F_SETFD,&flags))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",(const char *)strerror(errno));
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


void ProtocolSa::hostLookupFinishedData(const QHostInfo &info)
{
  QString sql=QString("update `PERM_SA_EVENTS` set ")+
    "`HOSTNAME`='"+SqlQuery::escape(info.hostName())+"' where "+
    QString::asprintf("`ID`=%d",proto_event_id);
  SqlQuery::apply(sql);
}


void ProtocolSa::destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;
 
  if(!proto_routestat_masked) {
    sql=RouteStatSqlFields(EndPointMap::AudioRouter)+
      " `SA_DESTINATIONS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SA_DESTINATIONS`.`SLOT`=%d ",slotnum)+
      "order by `SA_DESTINATIONS`.`SOURCE_NUMBER`,`SA_DESTINATIONS`.`ROUTER_NUMBER`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(RouteStatMessage(q).toUtf8());
      proto_socket->write(">>",2);
    }
    delete q;
  }
}


void ProtocolSa::gpiCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(!proto_gpistat_masked) {
    sql=GPIStatSqlFields()+" where "+
      "`GPIS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`GPIS`.`SLOT`=%d",slotnum);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GPIStatMessage(q).toUtf8());
      proto_socket->write(">>",2);
    }
    delete q;
  }
}


void ProtocolSa::gpoCodeChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(!proto_gpostat_masked) {
    sql=GPOStatSqlFields()+" where "+
      "`GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`GPOS`.`SLOT`=%d",slotnum);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GPOStatMessage(q).toUtf8());
      proto_socket->write(">>",2);
    }
    delete q;
  }
}


void ProtocolSa::gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(!proto_routestat_masked) {
    sql=RouteStatSqlFields(EndPointMap::GpioRouter)+
      " `SA_GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SA_GPOS`.`SLOT`=%d ",slotnum)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`,`SA_GPOS`.`ROUTER_NUMBER`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(RouteStatMessage(q).toUtf8());
      proto_socket->write(">>",2);
    }
    delete q;
  }
}


void ProtocolSa::ActivateRoute(unsigned router,unsigned output,unsigned input)
{
  EndPointMap *map;

  LogEvent(router,output,input-1);
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
  proto_socket->write(QString::asprintf("Begin SnapshotNames - %u\r\n",router+1).toUtf8());
  for(int i=0;i<map->snapshotQuantity();i++) {
    proto_socket->write(QString("   "+map->snapshot(i)->name()+"\r\n").toUtf8());
  }
  proto_socket->write(QString::asprintf("End SnapshotNames - %u\r\n",router+1).toUtf8());
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
  SqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  if(map->routerType()==EndPointMap::AudioRouter) {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_SOURCES`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_SOURCES`.`SOURCE_NUMBER`";
  }
  else {
    sql=SourceNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  q=new SqlQuery(sql);
  proto_socket->write(QString::asprintf("Begin SourceNames - %d\r\n",router+1).toUtf8());
  while(q->next()) {
    proto_socket->write(SourceNamesMessage(map->routerType(),q).toUtf8());
  }
  delete q;
  proto_socket->write(QString::asprintf("End SourceNames - %d\r\n",router+1).toUtf8());
}


QString ProtocolSa::SourceNamesSqlFields(EndPointMap::RouterType type) const
{
  if(type==EndPointMap::AudioRouter) {
    return QString("select ")+
      "`SA_SOURCES`.`SOURCE_NUMBER`,"+  // 00
      "`SOURCES`.`NAME`,"+              // 01
      "`SOURCES`.`HOST_ADDRESS`,"+      // 02
      "`SOURCES`.`HOST_NAME`,"+         // 03
      "`SOURCES`.`SLOT`,"+              // 04
      "`SOURCES`.`STREAM_ADDRESS`,"+    // 05
      "`SA_SOURCES`.`NAME` "+           // 06
      "from "+"`SOURCES` left join `SA_SOURCES` "+
      "on `SOURCES`.`ID`=`SA_SOURCES`."+"`SOURCE_ID` ";
  }
  return QString("select ")+
    "`SA_GPIS`.`SOURCE_NUMBER`,"+  // 00
    "`GPIS`.`HOST_ADDRESS`,"+      // 01
    "`GPIS`.`HOST_NAME`,"+         // 02
    "`GPIS`.`SLOT`,"+              // 03
    "`SA_GPIS`.`NAME` "+           // 04
    "from "+"`GPIS` left join `SA_GPIS` "+
    "on `GPIS`.`ID`=`SA_GPIS`.`GPI_ID` ";
}


QString ProtocolSa::SourceNamesMessage(EndPointMap::RouterType type,SqlQuery *q)
{
  if(type==EndPointMap::AudioRouter) {
    QString name=q->value(1).toString();
    if(!q->value(6).toString().isEmpty()) {
      name=q->value(6).toString();
    }
    return QString::asprintf("    %u",q->value(0).toInt()+1)+
      "\t"+name+
      "\t"+name+" ON "+q->value(3).toString()+
      "\t"+q->value(2).toString()+
      "\t"+q->value(3).toString()+
      "\t"+QString::asprintf("%u",q->value(4).toInt()+1)+
      "\t"+QString::asprintf("%d",SyRouting::livewireNumber(QHostAddress(q->value(5).toString())))+
      "\t"+q->value(5).toString()+
      "\r\n";
  }
  QString name="GPI";
  if(!q->value(4).toString().isEmpty()) {
    name=q->value(4).toString();
  }
  return QString::asprintf("    %u",q->value(0).toInt()+1)+
    "\t"+name+QString::asprintf(" %d",q->value(3).toInt()+1)+
    "\t"+name+QString::asprintf(" %d",q->value(3).toInt()+1)+" ON "+q->value(2).toString()+
    "\t"+q->value(1).toString()+
    "\t"+q->value(2).toString()+
    "\t"+QString::asprintf("%u",q->value(3).toInt()+1)+
    "\t"+q->value(1).toString()+QString::asprintf("/%d",q->value(3).toInt()+1)+
    "\t0"+
    "\r\n";
}


void ProtocolSa::SendDestInfo(unsigned router)
{
  EndPointMap *map;
  QString sql;
  SqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  proto_socket->write(">>",2);
  if(map->routerType()==EndPointMap::AudioRouter) {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_DESTINATIONS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_DESTINATIONS`.`SOURCE_NUMBER`";
  }
  else {
    sql=DestNamesSqlFields(map->routerType())+"where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  q=new SqlQuery(sql);
  proto_socket->write(QString::asprintf("Begin DestNames - %d\r\n",router+1).toUtf8());
  while(q->next()) {
    proto_socket->write(DestNamesMessage(map->routerType(),q).toUtf8());
  }
  delete q;
  proto_socket->write(QString::asprintf("End DestNames - %d\r\n",router+1).toUtf8());
}


QString ProtocolSa::DestNamesSqlFields(EndPointMap::RouterType type) const
{
  if(type==EndPointMap::AudioRouter) {
    return QString("select ")+
      "`SA_DESTINATIONS`.`SOURCE_NUMBER`,"+  // 00
      "`DESTINATIONS`.`NAME`,"+              // 01
      "`DESTINATIONS`.`HOST_ADDRESS`,"+      // 02
      "`DESTINATIONS`.`HOST_NAME`,"+         // 03
      "`DESTINATIONS`.`SLOT`,"+              // 04
      "`SA_DESTINATIONS`.`NAME` "+           // 05
      "from `DESTINATIONS` left join `SA_DESTINATIONS` on `DESTINATIONS`.`ID`=`SA_DESTINATIONS`.`DESTINATION_ID` ";
  }
  return QString("select ")+
    "`SA_GPOS`.`SOURCE_NUMBER`,"+  // 00
    "`GPOS`.`NAME`,"+              // 01
    "`GPOS`.`HOST_ADDRESS`,"+      // 02
    "`GPOS`.`HOST_NAME`,"+         // 03
    "`GPOS`.`SLOT`,"+              // 04
    "`SA_GPOS`.`NAME` "+           // 05
    "from `GPOS` left join `SA_GPOS` on `GPOS`.`ID`=`SA_GPOS`.`GPO_ID` ";
}


QString ProtocolSa::DestNamesMessage(EndPointMap::RouterType type,SqlQuery *q)
{
  QString name=q->value(1).toString();
  if(!q->value(5).toString().isEmpty()) {
    name=q->value(5).toString();
  }
  return QString::asprintf("    %u",q->value(0).toInt()+1)+
    "\t"+name+
    "\t"+name+" ON "+q->value(3).toString()+
    "\t"+q->value(2).toString()+
    "\t"+q->value(3).toString()+
    "\t"+QString::asprintf("%d",q->value(4).toInt()+1)+
    "\r\n";
}


void ProtocolSa::SendGpiInfo(unsigned router,int input)
{
  EndPointMap *map;
  QString sql;
  SqlQuery *q;

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
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  else {
    sql=GPIStatSqlFields()+"where "+
      QString::asprintf("`SA_GPIS`.`ROUTER_NUMBER`=%u && ",router)+
      QString::asprintf("`SA_GPIS`.`SOURCE_NUMBER`=%u ",input)+
      "order by `SA_GPIS`.`SOURCE_NUMBER`";
  }
  q=new SqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPIStatMessage(q).toUtf8());
  }
  delete q;
}


QString ProtocolSa::GPIStatSqlFields() const
{
  return QString("select ")+
    "`SA_GPIS`.`ROUTER_NUMBER`,"+  // 00
    "`SA_GPIS`.`SOURCE_NUMBER`,"+  // 01
    "`GPIS`.`CODE` "+              // 02
    "from `GPIS` right join `SA_GPIS` on `GPIS`.`ID`=`SA_GPIS`.`GPI_ID` ";
}


QString ProtocolSa::GPIStatMessage(SqlQuery *q)
{
  return QString::asprintf("GPIStat %d %d ",q->value(0).toInt()+1,q->value(1).toInt()+1)+
    q->value(2).toString()+"\r\n";
}


void ProtocolSa::SendGpoInfo(unsigned router,int output)
{
  EndPointMap *map;
  QString sql;
  SqlQuery *q;

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
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u ",router)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  else {
    sql=GPOStatSqlFields()+"where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%u && ",router)+
      QString::asprintf("`SA_GPOS`.`SOURCE_NUMBER`=%u ",output)+
      "order by `SA_GPOS`.`SOURCE_NUMBER`";
  }
  q=new SqlQuery(sql);
  while(q->next()) {
    proto_socket->write(GPOStatMessage(q).toUtf8());
  }
  delete q;
}


QString ProtocolSa::GPOStatSqlFields() const
{
  return QString("select ")+
    "`SA_GPOS`.`ROUTER_NUMBER`,"+  // 00
    "`SA_GPOS`.`SOURCE_NUMBER`,"+  // 01
    "`GPOS`.`CODE` "+              // 02
    "from `GPOS` right join `SA_GPOS` on `GPOS`.`ID`=`SA_GPOS`.`GPO_ID` ";
}


QString ProtocolSa::GPOStatMessage(SqlQuery *q)
{
  return QString::asprintf("GPOStat %d %d ",q->value(0).toInt()+1,q->value(1).toInt()+1)+
    q->value(2).toString()+"\r\n";
}


void ProtocolSa::SendRouteInfo(unsigned router,int output)
{
  EndPointMap *map;
  QString sql;
  SqlQuery *q;

  if((map=proto_maps.value(router))==NULL) {
    proto_socket->write(QString("Error - Bay Does Not exist.\r\n").toUtf8());
    return;
  }
  sql=RouteStatSqlFields(map->routerType());
  if(map->routerType()==EndPointMap::AudioRouter) {
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
  q=new SqlQuery(sql);
  q->first();
  if(output<0) {  // Send all crosspoints for the router
    for(int i=0;i<map->quantity(EndPointMap::Output);i++) {
      if(q->isValid()&&q->value(1).toInt()==i) {
	proto_socket->write(RouteStatMessage(q).toUtf8());
	q->next();
      }
      else {
	proto_socket->write(QString::asprintf("RouteStat %d %d 0 False\r\n",
					      router+1,i+1).toUtf8());
      }
    }
  }
  else {  // Send just the requested crosspoint
    if(output<map->quantity(EndPointMap::Output)) {
      if(q->isValid()) {
	proto_socket->write(RouteStatMessage(q).toUtf8());
      }
      else {
	proto_socket->write(QString::asprintf("RouteStat %d %d 0 False\r\n",
					      router+1,output+1).toUtf8());
      }
    }
  }
  delete q;
}


QString ProtocolSa::RouteStatSqlFields(EndPointMap::RouterType type)
{
  if(type==EndPointMap::AudioRouter) {
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


QString ProtocolSa::RouteStatMessage(SqlQuery *q)
{
  int input=q->value(2).toInt()+1;
  if(q->value(2).isNull()) {
    input=0;
  }
  return QString::asprintf("RouteStat %d %d %d False\r\n",
			   q->value(0).toInt()+1,
			   q->value(1).toInt()+1,
			   input);
}


void ProtocolSa::DrouterMaskGpiStat(bool state)
{
  proto_gpistat_masked=state;
}


void ProtocolSa::DrouterMaskGpoStat(bool state)
{
  proto_gpostat_masked=state;
}


void ProtocolSa::DrouterMaskRouteStat(bool state)
{
  proto_routestat_masked=state;
}


void ProtocolSa::DrouterMaskStat(bool state)
{
  proto_gpistat_masked=state;
  proto_gpostat_masked=state;
  proto_routestat_masked=state;
}


void ProtocolSa::ProcessCommand(const QString &cmd)
{
  unsigned cardnum=0;
  unsigned input=0;
  unsigned output=0;
  unsigned msecs=0;
  bool ok=false;
  QStringList cmds=cmd.split(" ");

  if((cmds[0].toLower()=="login")&&(cmds.size()>=2)) {
    proto_username=cmds.at(1);
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
      proto_socket->write((QString::asprintf("    %d ",it.value()->routerNumber()+1)+
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
    if((cmds.size()==2)||(cmds.size()==3)) {
      cardnum=cmds[1].toUInt(&ok);
      if(ok) {
	if(cmds.size()==2) {
	  SendRouteInfo(cardnum-1,-1);
	}
	if(cmds.size()==3) {
	  input=cmds[2].toUInt(&ok);
	  if(ok) {
	    SendRouteInfo(cardnum-1,input-1);
	  }
	  else {
	    proto_socket->write("Error\r\n");
	  }
	}
      }
      else {
	proto_socket->write(QString("Error - Bay Does Not exist.\r\n").
			    toUtf8());
      }
    }
    else {
      proto_socket->write("Error\r\n");
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

  if((cmds[0].toLower()=="droutermaskgpistat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      DrouterMaskGpiStat(cmds.at(1).toLower()=="true");
    }
    else {
      proto_socket->
	write(QString("Error - Invalid boolean value.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="droutermaskgpostat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      DrouterMaskGpoStat(cmds.at(1).toLower()=="true");
    }
    else {
      proto_socket->
	write(QString("Error - Invalid boolean value.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="droutermaskroutestat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      DrouterMaskRouteStat(cmds.at(1).toLower()=="true");
    }
    else {
      proto_socket->
	write(QString("Error - Invalid boolean value.\r\n").toUtf8());
    }
    proto_socket->write(">>",2);
  }

  if((cmds[0].toLower()=="droutermaskstat")&&(cmds.size()==2)) {
    if((cmds.at(1).toLower()=="true")||(cmds.at(1).toLower()=="false")) {
      DrouterMaskStat(cmds.at(1).toLower()=="true");
    }
    else {
      proto_socket->
	write(QString("Error - Invalid boolean value.\r\n").toUtf8());
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
    syslog(LOG_ERR,"map load error: %s, aborting",
	   (const char *)msgs.join("\n").toUtf8());
    exit(1);
  }
}


void ProtocolSa::LoadHelp()
{
  proto_help_strings[""]=QString("ActivateRoute")+
    ", ActivateScene"+
    ", ActivateSnap"+
    ", DestNames"+
    ", DrouterMaskGPIStat"+
    ", DrouterMaskGPOStat"+
    ", DrouterMaskRouteStat"+
    ", DrouterMaskStat"+
    ", Exit"+
    ", GPIStat"+
    ", GPOStat"+
    ", RouteStat"+
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
  proto_help_strings["droutermaskgpistat"]="DrouterMaskGPIStat True | False\r\n\r\nSuppress generation of GPIStat update messages on this connection.";
  proto_help_strings["droutermaskgpostat"]="DrouterMaskGPOStat True | False\r\n\r\nSuppress generation of GPOStat update messages on this connection.";
  proto_help_strings["droutermaskroutestat"]="DrouterMaskRouteStat True | False\r\n\r\nSuppress generation of RouteStat update messages on this connection.";
  proto_help_strings["droutermaskstat"]="DrouterMaskStat True | False\r\n\r\nSuppress generation of all state update messages on this connection.";
  proto_help_strings["droutermaskroutestat"]="DrouterMaskRouteStat True | False\r\n\r\nSuppress generation of RouteStat update messages on this connection.";
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


void ProtocolSa::LogEvent(int router,int output,int input)
{
  QString sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`DATETIME`=now(),"+
    "`ORIGINATING_ADDRESS`='"+proto_socket->peerAddress().toString()+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",router)+
    QString::asprintf("`DESTINATION_NUMBER`=%d,",output)+
    QString::asprintf("`SOURCE_NUMBER`=%d,",input);
  if(proto_username.isEmpty()) {
    sql+="`USERNAME`=NULL";
  }
  else {
    sql+="`USERNAME`='"+SqlQuery::escape(proto_username)+"'";
  }
  proto_event_id=SqlQuery::run(sql).toInt();
  QHostInfo::lookupHost(proto_socket->peerAddress().toString(),
			this,SLOT(hostLookupFinishedData(const QHostInfo &)));
}
