// protocol.cpp
//
// Base class for drouterd(8) protocols
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
#include <linux/un.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>


#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>

#include <sy/sylwrp_client.h>

#include "protocol.h"
#include "protoipc.h"

bool global_shutting_down=false;

void SigHandler(int signo)
{
  switch(signo) {
  case SIGCHLD:
    waitpid(-1,NULL,WNOHANG);
    break;

  case SIGINT:
  case SIGTERM:
    global_shutting_down=true;
    break;
  }
}


Protocol::Protocol(QObject *parent)
  : QObject(parent)
{
  proto_ipc_socket=NULL;
  proto_shutdown_timer=new QTimer(this);
  connect(proto_shutdown_timer,SIGNAL(timeout()),
	  this,SLOT(shutdownTimerData()));

  proto_config=new Config();
  proto_config->load();

  ::signal(SIGCHLD,SigHandler);
}


bool Protocol::startIpc(QString *err_msg)
{
  int sock;
  struct sockaddr_un sa;

  //
  // Connect to DRouter process
  //
  if((sock=socket(AF_UNIX,SOCK_SEQPACKET,0))<0) {
    *err_msg=QString("unable to start protocol ipc [")+strerror(errno)+"]";
    return false;
  }
  memset(&sa,0,sizeof(sa));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path+1,DROUTER_IPC_ADDRESS,UNIX_PATH_MAX-1);
  if(::connect(sock,(struct sockaddr *)(&sa),sizeof(sa))<0) {
    *err_msg=QString("unable to attach to drouter service [")+
      strerror(errno)+"]";
    return false;
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
    *err_msg="database error ["+db.lastError().driverText()+"]";
    return false;
  }

  //
  // Set Signals
  //
  //  ::signal(SIGINT,SigHandler);
  //  ::signal(SIGTERM,SigHandler);
  proto_shutdown_timer->start(500);

  return true;
}


void Protocol::clearCrosspoint(const QHostAddress &node_addr,int slotnum)
{
  proto_ipc_socket->write(("ClearCrosspoint "+node_addr.toString()+" "+
			   QString().sprintf("%d\r\n",slotnum)).toUtf8());
}


void Protocol::clearGpioCrosspoint(const QHostAddress &node_addr,int slotnum)
{
  proto_ipc_socket->write(("ClearGpioCrosspoint "+node_addr.toString()+" "+
			   QString().sprintf("%d\r\n",slotnum)).toUtf8());
}


void Protocol::setCrosspoint(const QHostAddress &dst_node_addr,int dst_slotnum,
			     const QHostAddress &src_node_addr,int src_slotnum)
{
  proto_ipc_socket->write(("SetCrosspoint "+dst_node_addr.toString()+
			   QString().sprintf(" %d ",dst_slotnum)+
			   src_node_addr.toString()+
			   QString().sprintf(" %d\r\n",src_slotnum)).toUtf8());
}


void Protocol::setGpioCrosspoint(const QHostAddress &gpo_node_addr,
				 int gpo_slotnum,
				 const QHostAddress &gpi_node_addr,
				 int gpi_slotnum)
{
  proto_ipc_socket->write(("SetGpioCrosspoint "+gpo_node_addr.toString()+
			   QString().sprintf(" %d ",gpo_slotnum)+
			   gpi_node_addr.toString()+
			   QString().sprintf(" %d\r\n",gpi_slotnum)).toUtf8());
}


void Protocol::setGpiState(const QHostAddress &gpi_node_addr,int gpi_slotnum,
			   const QString &code)
{
  proto_ipc_socket->write(("SetGpiState "+gpi_node_addr.toString()+
			   QString().sprintf(" %d ",gpi_slotnum)+code+"\r\n").
			  toUtf8());
}


void Protocol::setGpoState(const QHostAddress &gpo_node_addr,int gpo_slotnum,
			   const QString &code)
{
  proto_ipc_socket->write(("SetGpoState "+gpo_node_addr.toString()+
			   QString().sprintf(" %d ",gpo_slotnum)+code+"\r\n").
			  toUtf8());
}


void Protocol::ipcReadyReadData()
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


void Protocol::shutdownTimerData()
{
  if(global_shutting_down) {
    quit();
  }
}


void Protocol::tetherStateUpdated(bool state)
{
}


void Protocol::nodeAdded(const QHostAddress &host_addr)
{
}


void Protocol::nodeRemoved(const QHostAddress &host_addr,
			   int srcs,int dsts,int gpis,int gpos)
{
}


void Protocol::nodeChanged(const QHostAddress &host_addr)
{
}


void Protocol::sourceChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::destinationChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::gpiChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::gpiCodeChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::gpoChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::gpoCodeChanged(const QHostAddress &host_addr,int slotnum)
{
}


void Protocol::clipChanged(const QHostAddress &host_addr,int slotnum,
			   SyLwrpClient::MeterType meter_type,
			   const QString &tbl_name,int chan)
{
}


void Protocol::silenceChanged(const QHostAddress &host_addr,int slotnum,
			      SyLwrpClient::MeterType meter_type,
			      const QString &tbl_name,int chan)
{
}


Config *Protocol::config()
{
  return proto_config;
}


void Protocol::logIpc(const QString &msg)
{
  if(proto_config->ipcLogPriority()>=0) {
    syslog(proto_config->ipcLogPriority(),msg.toUtf8());
  }
}


void Protocol::quit()
{
  proto_ipc_socket->write("QUIT\r\n",6);
  qApp->processEvents();
  exit(0);
}


void Protocol::ProcessIpcCommand(const QString &cmd)
{
  logIpc("received IPC cmd: \""+cmd+"\"");

  QStringList cmds=cmd.split(":");

  if((cmds.at(0)=="TETHER")&&(cmds.size()==2)){
    tetherStateUpdated(cmds.at(1)=="Y");
  }

  if((cmds.at(0)=="NODEADD")&&(cmds.size()==2)){
    nodeAdded(QHostAddress(cmds.at(1)));
  }

  if((cmds.at(0)=="NODEDEL")&&(cmds.size()==6)) {
    nodeRemoved(QHostAddress(cmds.at(1)),cmds.at(2).toInt(),cmds.at(3).toInt(),
		cmds.at(4).toInt(),cmds.at(5).toInt());
  }

  if((cmds.at(0)=="NODE")&&(cmds.size()==2)) {
    nodeChanged(QHostAddress(cmds.at(1)));
  }

  if((cmds.at(0)=="SRC")&&(cmds.size()==3)) {
    sourceChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="DST")&&(cmds.size()==3)) {
    destinationChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="DSTX")&&(cmds.size()==3)) {
    destinationCrosspointChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="GPI")&&(cmds.size()==3)) {
    gpiChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="GPICODE")&&(cmds.size()==3)) {
    gpiCodeChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="GPOX")&&(cmds.size()==3)) {
    gpoCrosspointChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="GPOCODE")&&(cmds.size()==3)) {
    gpoCodeChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="GPO")&&(cmds.size()==3)) {
    gpoChanged(QHostAddress(cmds.at(1)),cmds.at(2).toInt());
  }

  if((cmds.at(0)=="CLIP")&&(cmds.size()==5)) {
    SyLwrpClient::MeterType meter_type=(SyLwrpClient::MeterType)cmds[1].toInt();
    QString table_name="DESTINATIONS";
    if(meter_type==SyLwrpClient::InputMeter) {
      table_name="SOURCES";
    }
    clipChanged(QHostAddress(cmds.at(3)),cmds.at(4).toInt(),meter_type,table_name,cmds.at(2).toInt());
  }

  if((cmds.at(0)=="SILENCE")&&(cmds.size()==5)) {
    SyLwrpClient::MeterType meter_type=(SyLwrpClient::MeterType)cmds[1].toInt();
    QString table_name="DESTINATIONS";
    if(meter_type==SyLwrpClient::InputMeter) {
      table_name="SOURCES";
    }
    silenceChanged(QHostAddress(cmds.at(3)),cmds.at(4).toInt(),meter_type,table_name,cmds.at(2).toInt());
  }

}
