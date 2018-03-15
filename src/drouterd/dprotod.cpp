// dprotod.cpp
//
// Protocol dispatcher for drouterd(8)
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
#include <signal.h>
#include <unistd.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>

#include "dprotod.h"
#include "protoipc.h"

void SigHandler(int signo)
{
  switch(signo) {
  case SIGCHLD:
    waitpid(-1,NULL,WNOHANG);
    break;
  }
}


MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  int flags;

  proto_socket=NULL;
  proto_ipc_socket=NULL;
  proto_destinations_subscribed=false;
  proto_gpis_subscribed=false;
  proto_gpos_subscribed=false;
  proto_nodes_subscribed=false;
  proto_sources_subscribed=false;
  proto_clips_subscribed=false;
  proto_silences_subscribed=false;

  ::signal(SIGCHLD,SigHandler);

  //
  // The ProtocolD Server
  //
  proto_server=new QTcpServer(this);
  connect(proto_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));
  proto_server->listen(QHostAddress::Any,23883);
  flags=flags|FD_CLOEXEC;
  if((flags=fcntl(proto_server->socketDescriptor(),F_SETFD,&flags))<0) {
    fprintf(stderr,"dprotod: socket error [%s]\n",(const char *)strerror(errno));
    exit(1);
  }
}


void MainObject::newConnectionData()
{
  int sock;
  struct sockaddr_un sa;
  int flags;

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
    // Initialize IPC
    //
    if((sock=socket(AF_UNIX,SOCK_SEQPACKET,0))<0) {
      fprintf(stderr,"unable to start protocol ipc [%s]\n",strerror(errno));
      exit(1);
    }
    memset(&sa,0,sizeof(sa));
    sa.sun_family=AF_UNIX;
    strncpy(sa.sun_path+1,DROUTER_IPC_ADDRESS,UNIX_PATH_MAX-1);
    if(::connect(sock,(struct sockaddr *)(&sa),sizeof(sa))<0) {
      fprintf(stderr,"unable to attach to drouter service [%s]\n",
	      strerror(errno));
      exit(1);
    }
    proto_ipc_socket=new QTcpSocket(this);
    proto_ipc_socket->setSocketDescriptor(sock,QAbstractSocket::ConnectedState);
    connect(proto_ipc_socket,SIGNAL(readyRead()),this,SLOT(ipcReadyReadData()));

    //
    // Connect to the Database
    //
    QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL3");
    db.setHostName("localhost");
    db.setDatabaseName("drouter");
    db.setUserName("drouter");
    db.setPassword("drouter");
    if(!db.open()) {
      proto_socket->write((QString("database error [")+db.lastError().driverText()+"]").toUtf8());
      exit(1);
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


void MainObject::readyReadData()
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


void MainObject::disconnectedData()
{
  Quit();
}


void MainObject::ipcReadyReadData()
{
  char data[1501];
  int n;

  while((n=proto_ipc_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(0xFF&data[i]) {
      case 10:
	break;

      case 13:
	ProcessIpcCommand(proto_ipc_accum);
	proto_ipc_accum="";
	break;

      default:
	proto_ipc_accum+=0xFF&data[i];
	break;
      }
    }
  }  
}


void MainObject::ProcessIpcCommand(const QString &cmd)
{
  printf("IPC CMD: %s\n",(const char *)cmd.toUtf8());

  QString sql;
  QSqlQuery *q;
  QStringList cmds=cmd.split(":");

  if((cmds.at(0)=="NODEADD")&&(cmds.size()==2)){
    if(proto_nodes_subscribed) {
      sql=NodeSqlFields()+"where "+
	"HOST_ADDRESS=\""+cmds.at(1)+"\"";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->write(NodeRecord("NODEADD",q).toUtf8());
      }
      delete q;
    }
    if(proto_sources_subscribed) {
      sql=SourceSqlFields()+"where "+
	"HOST_ADDRESS=\""+cmds.at(1)+"\""+     
	"order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->write(SourceRecord("SRCADD",q).toUtf8());
      }
      delete q;
    }
    if(proto_destinations_subscribed) {
      sql=DestinationSqlFields()+"where "+
	"HOST_ADDRESS=\""+cmds.at(1)+"\""+     
	"order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->write(DestinationRecord("DSTADD",q).toUtf8());
      }
      delete q;
    }
    if(proto_gpis_subscribed) {
      sql=GpiSqlFields()+"where "+
	"HOST_ADDRESS=\""+cmds.at(1)+"\""+     
	"order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->write(GpiRecord("GPIADD",q).toUtf8());
      }
      delete q;
    }
    if(proto_gpos_subscribed) {
      sql=GpoSqlFields()+"where "+
	"HOST_ADDRESS=\""+cmds.at(1)+"\""+     
	"order by HOST_ADDRESS,SLOT";
      q=new QSqlQuery(sql);
      while(q->next()) {
	proto_socket->write(GpoRecord("GPOADD",q).toUtf8());
      }
      delete q;
    }
  }

  if((cmds.at(0)=="NODEDEL")&&(cmds.size()==6)) {
    if(proto_gpos_subscribed) {
      for(int i=0;i<cmds.at(5).toInt();i++) {
	proto_socket->write(("GPODEL\t"+cmds.at(1)+"\t"+
			     QString().sprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_gpis_subscribed) {
      for(int i=0;i<cmds.at(4).toInt();i++) {
	proto_socket->write(("GPIDEL\t"+cmds.at(1)+"\t"+
			     QString().sprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_destinations_subscribed) {
      for(int i=0;i<cmds.at(3).toInt();i++) {
	proto_socket->write(("DSTDEL\t"+cmds.at(1)+"\t"+
			     QString().sprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_sources_subscribed) {
      for(int i=0;i<cmds.at(2).toInt();i++) {
	proto_socket->write(("SRCDEL\t"+cmds.at(1)+"\t"+
			     QString().sprintf("%d\r\n",i)).toUtf8());
      }
    }
    if(proto_nodes_subscribed) {
      proto_socket->write(("NODEDEL\t"+cmds.at(1)+"\r\n").toUtf8());
    }
  }

  if((cmds.at(0)=="NODE")&&(cmds.size()==2)&&proto_nodes_subscribed) {
    sql=NodeSqlFields()+"where "+
      "HOST_ADDRESS=\""+cmds.at(1)+"\"";
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(NodeRecord("NODE",q).toUtf8());
    }
  }

  if((cmds.at(0)=="SRC")&&(cmds.size()==3)&&proto_sources_subscribed) {
    sql=SourceSqlFields()+"where "+
      "HOST_ADDRESS=\""+cmds.at(1)+"\" && "+
      "SLOT="+cmds.at(2);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(SourceRecord("SRC",q).toUtf8());
    }
  }

  if((cmds.at(0)=="DST")&&(cmds.size()==3)&&proto_destinations_subscribed) {
    sql=DestinationSqlFields()+"where "+
      "HOST_ADDRESS=\""+cmds.at(1)+"\" && "+
      "SLOT="+cmds.at(2);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(DestinationRecord("DST",q).toUtf8());
    }
  }

  if((cmds.at(0)=="GPI")&&(cmds.size()==3)&&proto_gpis_subscribed) {
    sql=GpiSqlFields()+"where "+
      "HOST_ADDRESS=\""+cmds.at(1)+"\" && "+
      "SLOT="+cmds.at(2);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpiRecord("GPI",q).toUtf8());
    }
  }

  if((cmds.at(0)=="GPO")&&(cmds.size()==3)&&proto_gpos_subscribed) {
    sql=GpoSqlFields()+"where "+
      "HOST_ADDRESS=\""+cmds.at(1)+"\" && "+
      "SLOT="+cmds.at(2);
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(GpoRecord("GPO",q).toUtf8());
    }
  }

  if((cmds.at(0)=="CLIP")&&(cmds.size()==5)&&proto_clips_subscribed) {
    SyLwrpClient::MeterType meter_type=(SyLwrpClient::MeterType)cmds[1].toInt();
    int chan=cmds[2].toInt();
    if(meter_type==SyLwrpClient::InputMeter) {
      sql=AlarmSqlFields("CLIP",chan)+"from SOURCES where ";
    }
    else {
      sql=AlarmSqlFields("CLIP",chan)+"from DESTINATIONS where ";
    }
    sql+="HOST_ADDRESS=\""+cmds[3]+"\" && "+
      "SLOT="+cmds[4];
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(AlarmRecord("CLIP",meter_type,chan,q).toUtf8());
    }
    delete q;
  }

  if((cmds.at(0)=="SILENCE")&&(cmds.size()==5)&&proto_silences_subscribed) {
    SyLwrpClient::MeterType meter_type=(SyLwrpClient::MeterType)cmds[1].toInt();
    int chan=cmds[2].toInt();
    if(meter_type==SyLwrpClient::InputMeter) {
      sql=AlarmSqlFields("SILENCE",chan)+"from SOURCES where ";
    }
    else {
      sql=AlarmSqlFields("SILENCE",chan)+"from DESTINATIONS where ";
    }
    sql+="HOST_ADDRESS=\""+cmds[3]+"\" && "+
      "SLOT="+cmds[4];
    q=new QSqlQuery(sql);
    while(q->next()) {
      proto_socket->write(AlarmRecord("SILENCE",meter_type,chan,q).toUtf8());
    }
    delete q;
  }
}


void MainObject::Quit()
{
  proto_ipc_socket->write("QUIT\r\n",6);
  qApp->processEvents();
  exit(0);
}


void MainObject::ProcessCommand(const QString &cmd)
{
  //  printf("RECV[%d]: %s\n",proto_socket->socketDescriptor(),(const char *)cmd.toUtf8());
  QStringList cmds=cmd.split("\t");
  QString keyword=cmds.at(0).toLower();
  QString sql;
  QSqlQuery *q;

  if(keyword=="exit") {
    Quit();
  }

  if(keyword.isEmpty()) {
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

  proto_socket->write("error\r\n");
}


QString MainObject::AlarmSqlFields(const QString &type,int chan) const
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


QString MainObject::AlarmRecord(const QString &keyword,
				SyLwrpClient::MeterType port,int chan,
				QSqlQuery *q)
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


QString MainObject::DestinationSqlFields() const
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


QString MainObject::DestinationRecord(const QString &keyword,QSqlQuery *q) const
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


QString MainObject::GpiSqlFields() const
{
  return QString("select ")+
    "HOST_ADDRESS,"+  // 00
    "SLOT,"+          // 01
    "HOST_NAME,"+     // 02
    "CODE "+          // 03
    "from GPIS ";
}


QString MainObject::GpiRecord(const QString &keyword,QSqlQuery *q)
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


QString MainObject::GpoSqlFields() const
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


QString MainObject::GpoRecord(const QString &keyword,QSqlQuery *q)
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


QString MainObject::NodeSqlFields() const
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


QString MainObject::NodeRecord(const QString &keyword,QSqlQuery *q) const
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


QString MainObject::SourceSqlFields() const
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


QString MainObject::SourceRecord(const QString &keyword,QSqlQuery *q)
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


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
