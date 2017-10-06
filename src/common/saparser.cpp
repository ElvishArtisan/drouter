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

  //
  // The Socket
  //
  sa_socket=NULL;
  MakeSocket();

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


int SaParser::routerQuantity() const
{
  return sa_router_names.size();
}


QString SaParser::routerName(int router) const
{
  return sa_router_names.at(router);
}


int SaParser::inputQuantity() const
{
  return sa_input_names.size();
}


QString SaParser::inputName(int input)
{
  if(input>=sa_input_names.size()) {
    return QString();
  }
  return sa_input_names[input];
}


QString SaParser::inputLongName(int input)
{
  if(input>=sa_input_long_names.size()) {
    return QString();
  }
  return sa_input_long_names[input];
}


int SaParser::outputQuantity() const
{
  return sa_output_names.size();
}


QString SaParser::outputName(int output)
{
  if(output>=sa_output_names.size()) {
    return QString();
  }
  return sa_output_names[output];
}


QString SaParser::outputLongName(int output)
{
  if(output>=sa_output_long_names.size()) {
    return QString();
  }
  return sa_output_long_names[output];
}


int SaParser::outputCrosspoint(int output)
{
  return sa_output_xpoints[output];
}


void SaParser::setOutputCrosspoint(int output,int input)
{
  SendCommand(QString().sprintf("ActivateRoute %u %u %u",sa_matrix_number,
				output,input));
}


void SaParser::connectToHost(unsigned matrix_num,const QString &hostname,
			       uint16_t port,const QString &username,
			       const QString &passwd)
{
  QList<unsigned> empty;
  connectToHost(matrix_num,hostname,port,username,passwd,empty);
}


void SaParser::connectToHost(unsigned matrix_num,const QString &hostname,
			       uint16_t port,const QString &username,
			       const QString &passwd,
			       const QList<unsigned> &outputs)
{
  sa_matrix_number=matrix_num;
  sa_hostname=hostname;
  sa_port=port;
  sa_username=username;
  sa_password=passwd;
  sa_active_outputs=outputs;
  sa_socket->connectToHost(hostname,port);
}


void SaParser::connectedData()
{
  SendCommand("Login "+sa_username+" "+sa_password);
}


void SaParser::connectionClosedData()
{
  emit connected(false);
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
  sa_holdoff_timer->start(SAPARSER_HOLDOFF_INTERVAL);
}


void SaParser::DispatchCommand(const QString &cmd)
{
  //  printf("RECV: %s\n",(const char *)cmd.toUtf8());

  unsigned matrix;
  bool ok=false;

  QStringList f0=cmd.toLower().split(" ");

  //
  // Process Login
  //
  if(f0[0]=="login") {
    if((f0.size()==2)&&(f0[1]=="successful")) {
      //      SendCommand(QString().sprintf("SourceNames %u",sa_matrix_number));
      SendCommand("routernames");
    }
    else {
      emit connected(false);
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
      return;
    }
    if(f0.size()==4) {
      matrix=f0[3].toUInt(&ok);
      if(ok&&(matrix==sa_matrix_number)) {
	if(f0[1]=="sourcenames") {
	  sa_input_names.clear();
	  sa_input_long_names.clear();
	  sa_reading_sources=true;
	}
	if(f0[1]=="destnames") {
	  sa_output_names.clear();
	  sa_output_long_names.clear();
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
	SendCommand(QString().sprintf("SourceNames %u",sa_matrix_number));
      }
      return;
    }
    if(f0.size()==4) {
      matrix=f0[3].toUInt(&ok);
      if(ok&&(matrix==sa_matrix_number)) {
	if(f0[1]=="sourcenames") {
	  sa_reading_sources=false;
	  emit inputListChanged();
	  SendCommand(QString().sprintf("DestNames %u",sa_matrix_number));
	}
	if(f0[1]=="destnames") {
	  emit outputListChanged();
	  if(sa_active_outputs.isEmpty()) {
	    SendCommand(QString().sprintf("RouteStat %u\r\n",
					  sa_matrix_number));
	  }
	  else {
	    for(int i=0;i<sa_active_outputs.size();i++) {
	    SendCommand(QString().sprintf("RouteStat %u %u\r\n",
					  sa_matrix_number,
					  sa_active_outputs[i]));
	    }
	  }
	  sa_reading_dests=false;
	  emit connected(true);
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
    matrix=f0[1].toUInt(&ok);
    if(ok&&(matrix==sa_matrix_number)) {
      unsigned output=f0[2].toUInt(&ok);
      if(ok) {
	unsigned input=f0[3].toUInt(&ok);
	if(ok) {
	  sa_output_xpoints[output]=input;
	  emit outputCrosspointChanged(output,input);
	}
      }
    }
  }

  //  printf("CMD: %s\n",(const char *)cmd.toUtf8());
}


void SaParser::ReadRouterName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");

  if(f0.size()==2) {
    //
    // This blows up if the names are enumerated out of order!
    //
    sa_router_names.push_back(f0[1]);
  }
}


void SaParser::ReadSourceName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");

  if(f0.size()==8) {
    //
    // This blows up if the names are enumerated out of order!
    //
    sa_input_names.push_back(f0[1]);
    if(f0[6].toInt()<=0) {
      sa_input_long_names.push_back(f0[2]+" ["+tr("inactive")+"]");
    }
    else {
      sa_input_long_names.push_back(f0[2]+" ["+f0[6]+"]");
    }
  }
}


void SaParser::ReadDestName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");

  if(f0.size()==6) {
    //
    // This blows up if the names are enumerated out of order!
    //
    sa_output_names.push_back(f0[1]);
    sa_output_long_names.push_back(f0[2]);
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
