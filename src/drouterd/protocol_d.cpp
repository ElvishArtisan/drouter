// protocol_d.cpp
//
// Protocol D handler for DRouter.
//
//   (C) Copyright 2018-2019 Fred Gleason <fredg@paravelsystems.com>
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

ProtocolD::ProtocolD(int sock,QObject *parent)
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
    syslog(LOG_ERR,"socket error [%s], aborting",(const char *)strerror(errno));
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


void ProtocolD::nodeAdded(const QHostAddress &host_addr)
{
  QString sql;
  QSqlQuery *q;

  if(proto_nodes_subscribed) {
    sql=NodeSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\"";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODEADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_sources_subscribed) {
    sql=SourceSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\""+     
      "order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRCADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_destinations_subscribed) {
    sql=DestinationSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\""+     
      "order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DSTADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_gpis_subscribed) {
    sql=GpiSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\""+     
      "order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPIADD",q).toUtf8());
    }
    delete q;
  }
  if(proto_gpos_subscribed) {
    sql=GpoSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\""+     
      "order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPOADD",q).toUtf8());
    }
    delete q;
  }
}


void ProtocolD::nodeRemoved(const QHostAddress &host_addr,
			    int srcs,int dsts,int gpis,int gpos)
{
  if(proto_gpos_subscribed) {
    for(int i=0;i<gpos;i++) {
      proto_socket->write(("GPODEL\t"+host_addr.toString()+"\t"+
			   QString().sprintf("%d\r\n",i)).toUtf8());
    }
  }
  if(proto_gpis_subscribed) {
    for(int i=0;i<gpis;i++) {
      proto_socket->write(("GPIDEL\t"+host_addr.toString()+"\t"+
			   QString().sprintf("%d\r\n",i)).toUtf8());
    }
  }
  if(proto_destinations_subscribed) {
    for(int i=0;i<dsts;i++) {
      proto_socket->write(("DSTDEL\t"+host_addr.toString()+"\t"+
			   QString().sprintf("%d\r\n",i)).toUtf8());
    }
  }
  if(proto_sources_subscribed) {
    for(int i=0;i<srcs;i++) {
      proto_socket->write(("SRCDEL\t"+host_addr.toString()+"\t"+
			   QString().sprintf("%d\r\n",i)).toUtf8());
    }
  }
  if(proto_nodes_subscribed) {
    proto_socket->write(("NODEDEL\t"+host_addr.toString()+"\r\n").toUtf8());
  }
}


void ProtocolD::nodeChanged(const QHostAddress &host_addr)
{
  QString sql;
  QSqlQuery *q;

  if(proto_nodes_subscribed) {
    sql=NodeSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\"";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODE",q).toUtf8());
    }
  }
}


void ProtocolD::sourceChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  if(proto_sources_subscribed) {
    sql=SourceSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      "SLOT="+QString().sprintf("%d",slotnum);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRC",q).toUtf8());
    }
  }
}


void ProtocolD::destinationChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  if(proto_destinations_subscribed) {
    sql=DestinationSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      "SLOT="+QString().sprintf("%d",slotnum);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DST",q).toUtf8());
    }
  }
}


void ProtocolD::gpiChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  if(proto_gpis_subscribed) {
    sql=GpiSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      "SLOT="+QString().sprintf("%d",slotnum);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPI",q).toUtf8());
    }
  }
}


void ProtocolD::gpoChanged(const QHostAddress &host_addr,int slotnum)
{
  QString sql;
  QSqlQuery *q;

  if(proto_gpos_subscribed) {
    sql=GpoSqlFields()+"where "+
      "HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      "SLOT="+QString().sprintf("%d",slotnum);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPO",q).toUtf8());
    }
  }
}


void ProtocolD::clipChanged(const QHostAddress &host_addr,int slotnum,
			    SyLwrpClient::MeterType meter_type,
			    const QString &tbl_name,int chan)
{
  QString sql;
  QSqlQuery *q;

  if(proto_clips_subscribed) {
    sql=AlarmSqlFields("CLIP",chan)+"from "+tbl_name+" where ";
    sql+="HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      QString().sprintf("SLOT=%d",slotnum);
    q=new QSqlQuery(sql);
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
  QSqlQuery *q;

  if(proto_silences_subscribed) {
    sql=AlarmSqlFields("SILENCE",chan)+"from "+tbl_name+" where ";
    sql+="HOST_ADDRESS=\""+host_addr.toString()+"\" && "+
      QString().sprintf("SLOT=%d",slotnum);
    q=new QSqlQuery(sql);
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
  QSqlQuery *q;

  if(keyword=="exit") {
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
    sql=DestinationSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DST",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribedestinations") {
    proto_destinations_subscribed=true;
    sql=DestinationSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DSTADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listgpis") {
    sql=GpiSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPI",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribegpis") {
    proto_gpis_subscribed=true;
    sql=GpiSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPIADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listgpos") {
    sql=GpoSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPO",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribegpos") {
    proto_gpos_subscribed=true;
    sql=GpoSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPOADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listnodes") {
    sql=NodeSqlFields()+"order by HOST_ADDRESS";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODE",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribenodes") {
    proto_nodes_subscribed=true;
    sql=NodeSqlFields()+"order by HOST_ADDRESS";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODEADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listsources") {
    sql=SourceSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRC",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="subscribesources") {
    proto_sources_subscribed=true;
    sql=SourceSqlFields()+"order by HOST_ADDRESS,SLOT";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRCADD",q).toUtf8());
    }
    delete q;
    proto_socket->write("ok\r\n");
    return;
  }

  if(keyword=="listclips") {
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("CLIP",j)+
	"from SOURCES order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIP",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("CLIP",j)+
	"from DESTINATIONS order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
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
      sql=AlarmSqlFields("CLIP",j)+
	"from SOURCES order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("CLIPADD",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("CLIP",j)+
	"from DESTINATIONS order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
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
      sql=AlarmSqlFields("SILENCE",j)+
	"from SOURCES order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCE",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SILENCE",j)+
	"from DESTINATIONS order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
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
      sql=AlarmSqlFields("SILENCE",j)+
	"from SOURCES order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCEADD",SyLwrpClient::InputMeter,j,q).toUtf8());
      }
      delete q;
    }
    for(int j=0;j<2;j++) {
      sql=AlarmSqlFields("SILENCE",j)+
	"from DESTINATIONS order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->
	  write(AlarmRecord("SILENCEADD",SyLwrpClient::OutputMeter,j,q).toUtf8());
      }
      delete q;
    }
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
	  if(ok) {
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
	  if(ok) {
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
      if(ok) {
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
      if(ok) {
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
      if(ok) {
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
      if(ok) {
	setGpoState(gpo_addr,gpo_slotnum,cmds.at(3));
	proto_socket->write("ok\r\n");
	return;
      }
    }
  }

  proto_socket->write("error\r\n");
}


QString ProtocolD::AlarmSqlFields(const QString &type,int chan) const
{
  QString sql=QString("select ")+
    "HOST_ADDRESS,"+     // 00
    "SLOT,";             // 01
  if(chan==0) {
    sql+="LEFT_"+type;   // 02
  }
  else {
    sql+="RIGHT_"+type;  // 02
  }
  sql+=" ";
  return sql;
}


QString ProtocolD::AlarmRecord(const QString &keyword,SyLwrpClient::MeterType port,
			       int chan,QSqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString().sprintf("%d\t",q->value(1).toInt());
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
  ret+=QString().sprintf("%d\t",q->value(2).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::DestinationSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+    // 00
    "SLOT,"+            // 01
    "HOST_NAME,"+       // 02
    "STREAM_ADDRESS,"+  // 03
    "NAME,"+            // 04
    "CHANNELS "+        // 05
    "from DESTINATIONS ";
}


QString ProtocolD::DestinationRecord(const QString &keyword,QSqlQuery *q) const
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString().sprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=QString().sprintf("%u",q->value(5).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpiSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+  // 00
    "SLOT,"+          // 01
    "HOST_NAME,"+     // 02
    "CODE "+          // 03
    "from GPIS ";
}


QString ProtocolD::GpiRecord(const QString &keyword,QSqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString().sprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString();
  ret+="\r\n";

  return ret;
}


QString ProtocolD::GpoSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+    // 00
    "SLOT,"+            // 01
    "HOST_NAME,"+       // 02
    "CODE,"+            // 03
    "NAME,"+            // 04
    "SOURCE_ADDRESS,"+  // 05
    "SOURCE_SLOT "+     // 06
    "from GPOS ";
}


QString ProtocolD::GpoRecord(const QString &keyword,QSqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString().sprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=q->value(5).toString()+"\t";
  ret+=QString().sprintf("%d",q->value(6).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::NodeSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+       // 00
    "HOST_NAME,"+          // 01
    "DEVICE_NAME,"+        // 02
    "SOURCE_SLOTS,"+       // 03
    "DESTINATION_SLOTS,"+  // 04
    "GPI_SLOTS,"+          // 05
    "GPO_SLOTS "+          // 06
    "from NODES ";
}


QString ProtocolD::NodeRecord(const QString &keyword,QSqlQuery *q) const
{
  QString ret;

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=q->value(1).toString()+"\t";
  ret+=q->value(2).toString()+"\t";
  ret+=QString().sprintf("%u\t",q->value(3).toInt());
  ret+=QString().sprintf("%u\t",q->value(4).toInt());
  ret+=QString().sprintf("%u\t",q->value(5).toInt());
  ret+=QString().sprintf("%u",q->value(6).toInt());
  ret+="\r\n";

  return ret;
}


QString ProtocolD::SourceSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+    // 00
    "SLOT,"+            // 01
    "HOST_NAME,"+       // 02
    "STREAM_ADDRESS,"+  // 03
    "NAME,"+            // 04
    "STREAM_ENABLED,"+  // 05
    "CHANNELS,"+        // 06
    "BLOCK_SIZE "+      // 07
    "from SOURCES ";
}


QString ProtocolD::SourceRecord(const QString &keyword,QSqlQuery *q)
{
  QString ret="";

  ret+=keyword+"\t";
  ret+=q->value(0).toString()+"\t";
  ret+=QString().sprintf("%d\t",q->value(1).toInt());
  ret+=q->value(2).toString()+"\t";
  ret+=q->value(3).toString()+"\t";
  ret+=q->value(4).toString()+"\t";
  ret+=QString().sprintf("%u\t",q->value(5).toInt());
  ret+=QString().sprintf("%u\t",q->value(6).toInt());
  ret+=QString().sprintf("%u",q->value(7).toInt());
  ret+="\r\n";

  return ret;
}
