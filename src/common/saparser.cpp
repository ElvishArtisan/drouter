// saparser.cpp
//
// Parser for SoftwareAuthority Protocol
//
//   (C) Copyright 2016-2017 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#include <QStringList>

#include "saparser.h"

SaParser::SaParser(QObject *parent)
  : QObject(parent)
{
  sa_reading_routers=false;
  sa_reading_sources=false;
  sa_reading_dests=false;
  sa_current_router=-1;
  sa_last_router=-1;

  //
  // The Socket
  //
  sa_socket=NULL;

  //
  // Watchdog Timers
  //
  sa_holdoff_timer=new QTimer(this);
  sa_holdoff_timer->setSingleShot(true);
  connect(sa_holdoff_timer,SIGNAL(timeout()),
	  this,SLOT(holdoffReconnectData()));
}


SaParser::~SaParser()
{
}


QMap<int,QString> SaParser::routers() const
{
  return sa_router_names;
}


int SaParser::inputQuantity(int router) const
{
  return sa_input_names[router].size();
}


QString SaParser::inputName(int router,int input)
{
  return sa_input_names[router][input];
}


QString SaParser::inputLongName(int router,int input)
{
  return sa_input_long_names[router][input];
}


int SaParser::outputQuantity(int router) const
{
  return sa_output_names[router].size();
}


QString SaParser::outputName(int router,int output)
{
  return sa_output_names[router][output];
}


QString SaParser::outputLongName(int router,int output)
{
  return sa_output_long_names[router][output];
}


int SaParser::outputCrosspoint(int router,int output)
{
  return sa_output_xpoints[router][output];
}


void SaParser::setOutputCrosspoint(int router,int output,int input)
{
  SendCommand(QString().sprintf("ActivateRoute %d %d %d",router,output,input));
}


void SaParser::connectToHost(const QString &hostname,uint16_t port,
			     const QString &username,const QString &passwd)
{
  MakeSocket();
  sa_hostname=hostname;
  sa_port=port;
  sa_username=username;
  sa_password=passwd;
  sa_socket->connectToHost(hostname,port);
}


QString SaParser::connectionStateString(ConnectionState cstate)
{
  QString ret=tr("Unknown")+QString().sprintf(" [%d]",cstate);

  switch(cstate) {
  case SaParser::Ok:
    ret=tr("OK");
    break;

  case SaParser::InvalidLogin:
    ret=tr("Invalid login");
    break;

  case SaParser::WatchdogActive:
    ret=tr("Watchdog active");
    break;
  }

  return ret;
}


void SaParser::connectedData()
{
  SendCommand("Login "+sa_username+" "+sa_password);
}


void SaParser::connectionClosedData()
{
  emit connected(false,SaParser::WatchdogActive);
  sa_holdoff_timer->start(SAPARSER_HOLDOFF_INTERVAL);
}


void SaParser::holdoffReconnectData()
{
  MakeSocket();
  sa_socket->connectToHost(sa_hostname,sa_port);
}


void SaParser::readyReadData()
{
  QByteArray data;

  data=sa_socket->readAll();
  for(int i=0;i<data.length();i++) {
    switch(0xFFF&data[i]) {
    case 13:
      break;

    case 10:
      if(sa_accum.left(2)==">>") {
	sa_accum=sa_accum.right(sa_accum.length()-2);
      }
      DispatchCommand(sa_accum);
      sa_accum="";
      break;

    default:
      sa_accum+=data[i];
      break;
    }
  }
}


void SaParser::errorData(QAbstractSocket::SocketError err)
{
  emit error(err);
  sa_holdoff_timer->start(SAPARSER_HOLDOFF_INTERVAL);
}


void SaParser::DispatchCommand(const QString &cmd)
{
  //  printf("RECV: %s\n",(const char *)cmd.toUtf8());

  //  unsigned matrix;
  bool ok=false;

  QStringList f0=cmd.toLower().split(" ");

  //
  // Process Login
  //
  if(f0[0]=="login") {
    if((f0.size()==2)&&(f0[1]=="successful")) {
      SendCommand("routernames");
    }
    else {
      sa_socket->deleteLater();
      sa_socket=NULL;
      emit connected(false,SaParser::InvalidLogin);
    }
    return;
  }

  //
  // Check for delimiters
  //
  if(f0[0]=="begin") {
    if(f0.size()==2) {
      if(f0[1]=="routernames") {
	sa_router_names.clear();
	sa_reading_routers=true;
      }
    }
    if(f0.size()==4) {
      sa_current_router=f0[3].toInt(&ok);
      if(ok) {
	if(f0[1]=="sourcenames") {
	  sa_input_names[sa_current_router].clear();
	  sa_input_long_names[sa_current_router].clear();
	  sa_reading_sources=true;
	}
	if(f0[1]=="destnames") {
	  sa_output_names[sa_current_router].clear();
	  sa_output_long_names[sa_current_router].clear();
	  sa_reading_dests=true;
	}
      }
    }
    return;
  }
  if(f0[0]=="end") {
    if(f0.size()==2) {
      if(f0[1]=="routernames") {
	sa_reading_routers=false;
	emit routerListChanged();
	for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
	    it!=sa_router_names.end();it++) {
	  SendCommand(QString().sprintf("SourceNames %u",it.key()));
	  sa_last_router=it.key();
	}
      }
    }
    if(f0.size()==4) {
      sa_current_router=f0[3].toInt(&ok);
      if(ok) {
	if((f0[1]=="sourcenames")&&(sa_current_router==sa_last_router)) {
	  sa_reading_sources=false;
	  emit inputListChanged();
	  for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
	      it!=sa_router_names.end();it++) {
	    SendCommand(QString().sprintf("DestNames %u",it.key()));
	  }
	}
	if((f0[1]=="destnames")&&(sa_current_router==sa_last_router)) {
	  emit outputListChanged();
	  for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
	      it!=sa_router_names.end();it++) {
	    SendCommand(QString().sprintf("RouteStat %u\r\n",it.key()));
	  }
	  sa_reading_dests=false;
	  emit connected(true,SaParser::Ok);
	}
      }
    }
    return;
  }

  //
  // Populate Endpoint Names
  //
  if(sa_reading_routers) {
    ReadRouterName(cmd);
    return;
  }
  if(sa_reading_sources) {
    ReadSourceName(cmd);
    return;
  }
  if(sa_reading_dests) {
    ReadDestName(cmd);
    return;
  }

  //
  // Process Crosspoint Changes
  //
  if((f0[0]=="routestat")&&(f0.size()==5)) {
    int router=f0[1].toUInt(&ok);
    if(ok) {
      int output=f0[2].toInt(&ok);
      if(ok) {
	int input=f0[3].toInt(&ok);
	if(ok) {
	  sa_output_xpoints[router][output]=input;
	  emit outputCrosspointChanged(router,output,input);
	}
      }
    }
  }

  //  printf("CMD: %s\n",(const char *)cmd.toUtf8());
}


void SaParser::ReadRouterName(const QString &cmd)
{
  bool ok=false;
  QStringList f0=cmd.split(" ",QString::SkipEmptyParts);
  if(f0.size()==2) {
    int router=f0.at(0).toInt(&ok);
    if(ok) {
      sa_router_names[router]=f0.at(1);
    }
  }
}


void SaParser::ReadSourceName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");
  bool ok=false;
  int srcnum=0;
  int input=f0.at(0).toInt(&ok);

  if(ok) {
    if(f0.size()==8) {
      srcnum=f0[6].toInt(&ok);
    }
    if(f0.size()>=3) {
      sa_input_names[sa_current_router][input]=f0[1];
      if(ok) {
	if(srcnum<=0) {
	  sa_input_long_names[sa_current_router][input]=
	    f0[2]+" ["+tr("inactive")+"]";
	}
	else {
	  sa_input_long_names[sa_current_router][input]=f0[2]+" ["+f0[6]+"]";
	}
      }
      else {
	sa_input_long_names[sa_current_router][input]=f0[2];
      }
    }
  }
}


void SaParser::ReadDestName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");
  bool ok=false;
  int output=f0.at(0).toInt(&ok);

  if(f0.size()>=3) {
    if(ok) {
      sa_output_names[sa_current_router][output]=f0[1];
      sa_output_long_names[sa_current_router][output]=f0[2];
    }
  }
}


void SaParser::BubbleSort(std::map<unsigned,QString> *names,
			    std::vector<unsigned> *ptrs)
{
  //
  // Reset Pointer Table
  //
  ptrs->clear();
  for(unsigned i=0;i<names->size();i++) {
    ptrs->push_back(i);
  }

  //
  // Sort
  //
  bool changed=true;
  while(changed) {
    changed=false;
    for(unsigned i=1;i<names->size();i++) {
      if((*names)[ptrs->at(i-1)]>(*names)[ptrs->at(i)]) {
	unsigned ptr=ptrs->at(i-1);
	ptrs->at(i-1)=ptrs->at(i);
	ptrs->at(i)=ptr;
	changed=true;
      }
    }
  }
}


void SaParser::SendCommand(const QString &cmd)
{
  //  printf("SendCommand(%s)\n",(const char *)cmd.toUtf8());
  sa_socket->write((cmd+"\r\n").toUtf8(),cmd.length()+2);
}


void SaParser::MakeSocket()
{
  if(sa_socket!=NULL) {
    delete sa_socket;
  }
  sa_socket=new QTcpSocket(this);
  connect(sa_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(sa_socket,SIGNAL(connectionClosed()),
	  this,SLOT(connectionClosedData()));
  connect(sa_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(sa_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}
