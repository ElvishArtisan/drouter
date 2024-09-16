// protocol_d.cpp
//
// Protocol D handler for DRouter.
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

#include <QStringList>

#include "protocol_d.h"
#include "sqlquery.h"

ProtocolD::ProtocolD(int sock,QObject *parent)
  : Protocol(parent)
{
  int flags;

  proto_socket=NULL;
  proto_tether_subscribed=false;
  proto_destinations_subscribed=false;
  proto_gpis_subscribed=false;
  proto_gpos_subscribed=false;
  proto_nodes_subscribed=false;
  proto_sources_subscribed=false;
  proto_clips_subscribed=false;
  proto_silences_subscribed=false;

  openlog("dprotod(D)",LOG_PID,LOG_DAEMON);

  //
  // The ProtocolD Server
  //
  proto_server=new QTcpServer(this);
  connect(proto_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));
  if(sock<0) {
    proto_server->listen(QHostAddress::Any,23883);
  }
  else {
    proto_server->setSocketDescriptor(sock);
  }
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_server->socketDescriptor(),F_SETFD,&flags))<0) {
    syslog(LOG_ERR,"socket error [%s], aborting",strerror(errno));
    exit(1);
  }
}


void ProtocolD::newConnectionData()
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


void ProtocolD::readyReadData()
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


void ProtocolD::disconnectedData()
{
  quit();
}


void ProtocolD::tetherStateUpdated(bool state)
{
  if(proto_tether_subscribed) {
    if(state) {
      proto_socket->write("TETHER\tY\r\n");
    }
    else {
      proto_socket->write("TETHER\tN\r\n");
    }
  }
}


void ProtocolD::nodeAdded(const QHostAddress &host_addr)
{
  QString sql;
  SqlQuery *q;

  if(proto_nodes_subscribed) {
    sql=NodeSqlFields()+"where "+
      "`NODES`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODEADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_sources_subscribed) {
    sql=SourceSqlFields()+"where "+
      "`SOURCES`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRCADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_destinations_subscribed) {
    sql=DestinationSqlFields()+"where "+
      "`DESTINATIONS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DSTADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_gpis_subscribed) {
    sql=GpiSqlFields()+"where "+
      "`GPIS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPIS`.`HOST_ADDRESS`,`GPIS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPIADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_gpos_subscribed) {
    sql=GpoSqlFields()+"where "+
      "`GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPOS`.`HOST_ADDRESS`,`GPOS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPOADD",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::nodeRemoved(const QHostAddress &host_addr,
			    int srcs,int dsts,int gpis,int gpos)
{
  QString sql;
  SqlQuery *q;

  sql=NodeSqlFields()+"where "+
    "`NODES`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
    QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
  q=new SqlQuery(sql);
  if(q->first()) {
    if(proto_gpos_subscribed) {
      for(int i=0;i<gpos;i++) {
	proto_socket->write(("GPODEL\t"+host_addr.toString()+"\t"+
			     QString::asprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_gpis_subscribed) {
      for(int i=0;i<gpis;i++) {
	proto_socket->write(("GPIDEL\t"+host_addr.toString()+"\t"+
			     QString::asprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_destinations_subscribed) {
      for(int i=0;i<dsts;i++) {
	proto_socket->write(("DSTDEL\t"+host_addr.toString()+"\t"+
			     QString::asprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_sources_subscribed) {
      for(int i=0;i<srcs;i++) {
	proto_socket->write(("SRCDEL\t"+host_addr.toString()+"\t"+
			     QString::asprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_nodes_subscribed) {
      proto_socket->write(("NODEDEL\t"+host_addr.toString()+"\r\n").toUtf8());
    }
  }
  delete q;
}


void ProtocolD::nodeChanged(const QHostAddress &host_addr)
{
  QString sql;
  SqlQuery *q;

  if(proto_nodes_subscribed) {
    sql=NodeSqlFields()+"where "+
      "`NODES`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODE",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::sourceChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(proto_sources_subscribed) {
    sql=SourceSqlFields()+"where "+
      "`SOURCES`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      "`SOURCES`.`SLOT`="+QString::asprintf("%d && ",slotnum)+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRC",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::destinationChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(proto_destinations_subscribed) {
    sql=DestinationSqlFields()+"where "+
      "`DESTINATIONS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      "`DESTINATIONS`.`SLOT`="+QString::asprintf("%d && ",slotnum)+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DST",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::gpiChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(proto_gpis_subscribed) {
    sql=GpiSqlFields()+"where "+
      "`GPIS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      "`GPIS`.`SLOT`="+QString::asprintf("%d && ",slotnum)+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPI",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::gpoChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  SqlQuery *q;

  if(proto_gpos_subscribed) {
    sql=GpoSqlFields()+"where "+
      "`GPOS`.`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      "`GPOS`.`SLOT`="+QString::asprintf("%d && ",slotnum)+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPO",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::clipChanged(const QHostAddress &host_addr,int slotnum,
			    SyLwrpClient::MeterType meter_type,
			    const QString &tbl_name,int chan)
{
  QString sql;
  SqlQuery *q;

  if(proto_clips_subscribed) {
    sql=AlarmSqlFields(tbl_name,"CLIP",chan)+"from "+tbl_name+" where ";
    sql+="`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SLOT`=%d",slotnum);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(AlarmRecord("CLIP",meter_type,chan,q).toUtf8());
    }
    delete q;
  }
}
 

void ProtocolD::silenceChanged(const QHostAddress &host_addr,int slotnum,
			       SyLwrpClient::MeterType meter_type,
			       const QString &tbl_name,int chan)
{
  QString sql;
  SqlQuery *q;

  if(proto_silences_subscribed) {
    sql=AlarmSqlFields(tbl_name,"SILENCE",chan)+"from "+tbl_name+" where ";
    sql+="`HOST_ADDRESS`='"+host_addr.toString()+"' && "+
      QString::asprintf("`SLOT`=%d",slotnum);
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(AlarmRecord("SILENCE",meter_type,chan,q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::ProcessCommand(const QString &cmd)
{
  QStringList cmds=cmd.split(" ");
  QString keyword=cmds.at(0).toLower();
  QString sql;
  SqlQuery *q;

  if(keyword=="exit") {
    syslog(LOG_DEBUG,"exiting normally");
    quit();
  }

  if(keyword.isEmpty()) {
    return;
  }

  if(keyword=="ping") {
    proto_socket->write("Pong\r\n",6);
    return;
  }

  if(keyword=="listdestinations") {
    sql=DestinationSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DST",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribedestinations") {
    proto_destinations_subscribed=true;
    sql=DestinationSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DSTADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listgpis") {
    sql=GpiSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPIS`.`HOST_ADDRESS`,`GPIS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPI",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribegpis") {
    proto_gpis_subscribed=true;
    sql=GpiSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPIS`.`HOST_ADDRESS`,`GPIS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPIADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listgpos") {
    sql=GpoSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPOS`.`HOST_ADDRESS`,`GPOS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPO",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribegpos") {
    proto_gpos_subscribed=true;
    sql=GpoSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `GPOS`.`HOST_ADDRESS`,`GPOS`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPOADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listnodes") {
    sql=NodeSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `HOST_ADDRESS`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODE",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribenodes") {
    proto_nodes_subscribed=true;
    sql=NodeSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `NODES`.`HOST_ADDRESS`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODEADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listsources") {
    sql=SourceSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRC",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribesources") {
    proto_sources_subscribed=true;
    sql=SourceSqlFields()+"where "+
      QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
      "order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
    q=new SqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRCADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listclips") {
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SOURCES","CLIP",j)+
	"from `SOURCES` left join `NODES` "+
	"on `SOURCES`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIP",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("DESTINATIONS","CLIP",j)+
	"from `DESTINATIONS` left join `NODES` "+
	"on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIP",SyLwrpClient::OutputMeter,j,q).toUtf8());
      }
      delete q;
    }
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribeclips") {
    proto_clips_subscribed=true;
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SOURCES","CLIP",j)+
	"from `SOURCES` left join `NODES` "+
	"on `SOURCES`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIPADD",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("DESTINATIONS","CLIP",j)+
	"from `DESTINATIONS` left join `NODES` "+
	"on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIPADD",SyLwrpClient::OutputMeter,j,q).toUtf8());
      }
      delete q;
    }
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listsilences") {
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SOURCES","SILENCE",j)+
	"from `SOURCES` left join `NODES` "+
	"on `SOURCES`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCE",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("DESTINATIONS","SILENCE",j)+
	"from `DESTINATIONS` left join 'NODES' "+
	"on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCE",SyLwrpClient::OutputMeter,j,q).toUtf8());
      }
      delete q;
    }
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribesilences") {
    proto_silences_subscribed=true;
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SOURCES","SILENCE",j)+
	"from `SOURCES` left join `NODES` "+
	"on `SOURCES`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `SOURCES`.`HOST_ADDRESS`,`SOURCES`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCEADD",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("DESTINATIONS","SILENCE",j)+
	"from `DESTINATIONS` left join `NODES` "+
	"on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` "+
	"where "+
	QString::asprintf("`NODES`.`MATRIX_TYPE`=%u ",Config::LwrpMatrix)+
	"order by `DESTINATIONS`.`HOST_ADDRESS`,`DESTINATIONS`.`SLOT`";
      q=new SqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCEADD",SyLwrpClient::OutputMeter,j,q).toUtf8());
      }
      delete q;
    }
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listtether") {
    sql=QString("select `TETHER`.`IS_ACTIVE` from `TETHER`");
    q=new SqlQuery(sql);
    if(q->first()) {
      proto_socket->
	write((QString("TETHER\t")+q->value(0).toString()+"\r\n").toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribetether") {
    proto_tether_subscribed=true;
    sql=QString("select `TETHER`.`IS_ACTIVE` from `TETHER`");
    q=new SqlQuery(sql);
    if(q->first()) {
      proto_socket->
	write((QString("TETHER\t")+q->value(0).toString()+"\r\n").toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if((keyword=="setcrosspoint")&&(cmds.size()==5)) {
    bool ok;

    QHostAddress dst_addr(cmds.at(1));
    if(!dst_addr.isNull()) {
      int dst_slotnum=cmds.at(2).toInt(&ok);
      if(ok) {
	QHostAddress src_addr(cmds.at(3));
	if(!src_addr.isNull()) {
	  int src_slotnum=cmds.at(4).toInt(&ok);
	  if(ok&&IsLivewire(dst_addr,src_addr)) {
	    setCrosspoint(dst_addr,dst_slotnum,src_addr,src_slotnum);
	    proto_socket->write("ok\r\n");
	    return;
	  }
	}
      }
    }
  }

  if((keyword=="setgpiocrosspoint")&&(cmds.size()==5)) {
    bool ok;

    QHostAddress gpo_addr(cmds.at(1));
    if(!gpo_addr.isNull()) {
      int gpo_slotnum=cmds.at(2).toInt(&ok);
      if(ok) {
	QHostAddress gpi_addr(cmds.at(3));
	if(!gpi_addr.isNull()) {
	  int gpi_slotnum=cmds.at(4).toInt(&ok);
	  if(ok&&IsLivewire(gpo_addr,gpi_addr)) {
	    setGpioCrosspoint(gpo_addr,gpo_slotnum,gpi_addr,gpi_slotnum);
	    proto_socket->write("ok\r\n");
	    return;
	  }
	}
      }
    }
  }

  if((keyword=="clearcrosspoint")&&(cmds.size()==3)) {
    bool ok;

    QHostAddress dst_addr(cmds.at(1));
    if(!dst_addr.isNull()) {
      int dst_slotnum=cmds.at(2).toInt(&ok);
      if(ok&&IsLivewire(dst_addr)) {
	clearCrosspoint(dst_addr,dst_slotnum);
	proto_socket->write("ok\r\n");
	return;
      }
    }
  }

  if((keyword=="cleargpiocrosspoint")&&(cmds.size()==3)) {
    bool ok;

    QHostAddress gpo_addr(cmds.at(1));
    if(!gpo_addr.isNull()) {
      int gpo_slotnum=cmds.at(2).toInt(&ok);
      if(ok&&IsLivewire(gpo_addr)) {
	clearGpioCrosspoint(gpo_addr,gpo_slotnum);
	proto_socket->write("ok\r\n");
	return;
      }
    }
  }

  if((keyword=="setgpistate")&&(cmds.size()==4)) {
    bool ok;

    QHostAddress gpi_addr(cmds.at(1));
    if(!gpi_addr.isNull()) {
      int gpi_slotnum=cmds.at(2).toInt(&ok);
      if(ok&&IsLivewire(gpi_addr)) {
	setGpiState(gpi_addr,gpi_slotnum,cmds.at(3));
	proto_socket->write("ok\r\n");
	return;
      }
    }
  }

  if((keyword=="setgpostate")&&(cmds.size()==4)) {
    bool ok;

    QHostAddress gpo_addr(cmds.at(1));
    if(!gpo_addr.isNull()) {
      int gpo_slotnum=cmds.at(2).toInt(&ok);
      if(ok&&IsLivewire(gpo_addr)) {
	setGpoState(gpo_addr,gpo_slotnum,cmds.at(3));
	proto_socket->write("ok\r\n");
	return;
      }
    }
  }

  proto_socket->write("error\r\n");
}


QString ProtocolD::AlarmSqlFields(const QString &tbl_name,const QString &type,
				  int chan) const
{
  QString sql=QString("select ")+
    "`"+tbl_name+"`.`HOST_ADDRESS`,"+        // 00
    "`"+tbl_name+"`.`SLOT`,";                // 01
  if(chan==0) {
    sql+="`LEFT_"+type+"`";   // 02
  }
  else {
    sql+="`RIGHT_"+type+"`";  // 02
  }
  sql+=" ";
  return sql;
}


QString ProtocolD::AlarmRecord(const QString &keyword,SyLwrpClient::MeterType port,
			       int chan,SqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString::asprintf("%d\t",q->value(1).toInt());
  switch(port) {
  case SyLwrpClient::InputMeter:
    ret+="INPUT\t";
    break;

  case SyLwrpClient::OutputMeter:
    ret+="OUTPUT\t";
    break;

  case SyLwrpClient::LastTypeMeter:
    ret+="UNKNOWN\t";
    break;
  }
  switch(chan) {
  case 0:
    ret+="LEFT\t";
    break;

  case 1:
    ret+="RIGHT\t";
    break;

  default:
    ret+="UNKNOWN\t";
    break;
  }
  ret+=QString::asprintf("%d\t",q->value(2).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::DestinationSqlFields() const
{
  return QString("select ")+
    "`DESTINATIONS`.`HOST_ADDRESS`,"+    // 00
    "`DESTINATIONS`.`SLOT`,"+            // 01
    "`DESTINATIONS`.`HOST_NAME`,"+       // 02
    "`DESTINATIONS`.`STREAM_ADDRESS`,"+  // 03
    "`DESTINATIONS`.`NAME`,"+            // 04
    "`DESTINATIONS`.`CHANNELS` "+        // 05
    "from `DESTINATIONS` left join `NODES` "+
    "on `DESTINATIONS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
}


QString ProtocolD::DestinationRecord(const QString &keyword,SqlQuery *q) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString::asprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=QString::asprintf("%u",q->value(5).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpiSqlFields() const
{
  return QString("select ")+
    "`GPIS`.`HOST_ADDRESS`,"+  // 00
    "`GPIS`.`SLOT`,"+          // 01
    "`GPIS`.`HOST_NAME`,"+     // 02
    "`GPIS`.`CODE` "+          // 03
    "from `GPIS` left join `NODES` "+
    "on `GPIS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
}


QString ProtocolD::GpiRecord(const QString &keyword,SqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString::asprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString();
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpoSqlFields() const
{
  return QString("select ")+
    "`GPOS`.`HOST_ADDRESS`,"+    // 00
    "`GPOS`.`SLOT`,"+            // 01
    "`GPOS`.`HOST_NAME`,"+       // 02
    "`GPOS`.`CODE`,"+            // 03
    "`GPOS`.`NAME`,"+            // 04
    "`GPOS`.`SOURCE_ADDRESS`,"+  // 05
    "`GPOS`.`SOURCE_SLOT` "+     // 06
    "from `GPOS` left join `NODES` "+
    "on `GPOS`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
}


QString ProtocolD::GpoRecord(const QString &keyword,SqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString::asprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=q->value(5).toString()+"\t";
  ret+=QString::asprintf("%d",q->value(6).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::NodeSqlFields() const
{
  return QString("select ")+
    "`NODES`.`HOST_ADDRESS`,"+       // 00
    "`NODES`.`HOST_NAME`,"+          // 01
    "`NODES`.`DEVICE_NAME`,"+        // 02
    "`NODES`.`SOURCE_SLOTS`,"+       // 03
    "`NODES`.`DESTINATION_SLOTS`,"+  // 04
    "`NODES`.`GPI_SLOTS`,"+          // 05
    "`NODES`.`GPO_SLOTS` "+          // 06
    "from `NODES` ";
}


QString ProtocolD::NodeRecord(const QString &keyword,SqlQuery *q) const
{
  QString ret;

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=q->value(1).toString()+"\t";
  ret+=q->value(2).toString()+"\t";
  ret+=QString::asprintf("%u\t",q->value(3).toInt());
  ret+=QString::asprintf("%u\t",q->value(4).toInt());
  ret+=QString::asprintf("%u\t",q->value(5).toInt());
  ret+=QString::asprintf("%u",q->value(6).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::SourceSqlFields() const
{
  return QString("select ")+
    "`SOURCES`.`HOST_ADDRESS`,"+    // 00
    "`SOURCES`.`SLOT`,"+            // 01
    "`SOURCES`.`HOST_NAME`,"+       // 02
    "`SOURCES`.`STREAM_ADDRESS`,"+  // 03
    "`SOURCES`.`NAME`,"+            // 04
    "`SOURCES`.`STREAM_ENABLED`,"+  // 05
    "`SOURCES`.`CHANNELS`,"+        // 06
    "`SOURCES`.`BLOCK_SIZE` "+      // 07
    "from `SOURCES` left join `NODES` "+
    "on `SOURCES`.`HOST_ADDRESS`=`NODES`.`HOST_ADDRESS` ";
}


QString ProtocolD::SourceRecord(const QString &keyword,SqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString::asprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=QString::asprintf("%u\t",q->value(5).toInt());
  ret+=QString::asprintf("%u\t",q->value(6).toInt());
  ret+=QString::asprintf("%u",q->value(7).toInt());
  ret+="\r\n";

  return ret;
}


bool ProtocolD::IsLivewire(const QHostAddress &host_addr1,
			   const QHostAddress &host_addr2)
{
  bool ret;
  int size=1;
  QString sql=NodeSqlFields()+" where "+
    QString::asprintf("`NODES`.`MATRIX_TYPE`=%u && (",Config::LwrpMatrix)+
    "`NODES`.`HOST_ADDRESS`='"+host_addr1.toString()+"' ";
  if(!host_addr2.isNull()) {
    sql+="|| `NODES`.`HOST_ADDRESS`='"+host_addr2.toString()+"'";
    if(host_addr1!=host_addr2) {
      size=2;
    }
  }
  sql+=")";
  SqlQuery *q=new SqlQuery(sql);
  ret=q->size()==size;
  delete q;

  return ret;
}
