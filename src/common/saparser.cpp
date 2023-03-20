// saparser.cpp
//
// Parser for SoftwareAuthority Protocol
//
//   (C) Copyright 2016-2020 Fred Gleason <fredg@paravelsystems.com>
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
  sa_connected=false;
  sa_reading_routers=false;
  sa_reading_sources=false;
  sa_reading_dests=false;
  sa_reading_snapshots=false;
  sa_current_router=-1;
  sa_last_router=-1;
  sa_prev_input=0;
  sa_prev_output=0;
  sa_last_xpoint_router=-1;
  sa_last_xpoint_output=-1;

  //
  // The Socket
  //
  sa_socket=NULL;

  //
  // Startup Timer
  //
  sa_startup_timer=new QTimer(this);
  sa_startup_timer->setSingleShot(true);
  connect(sa_startup_timer,SIGNAL(timeout()),this,SLOT(startupData()));

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


bool SaParser::isConnected() const
{
  return sa_connected;
}


bool SaParser::gpioSupported(int router) const
{
  return sa_gpio_supporteds.value(router);
}


int SaParser::inputQuantity(int router) const
{
  return sa_input_is_reals[router].size();
}


bool SaParser::inputIsReal(int router,int input) const
{
  return sa_input_is_reals[router][input];
}


QString SaParser::inputNodeName(int router,int input) const
{
  return sa_input_node_names[router][input];
}


QHostAddress SaParser::inputNodeAddress(int router,int input) const
{
  return sa_input_node_addresses[router][input];
}


int SaParser::inputNodeSlotNumber(int router,int input) const
{
  return sa_input_node_slot_numbers[router][input];
}


QString SaParser::inputName(int router,int input) const
{
  return sa_input_names[router][input];
}


QString SaParser::inputLongName(int router,int input) const
{
  return sa_input_long_names[router][input];
}


int SaParser::inputSourceNumber(int router,int input) const
{
  return sa_input_source_numbers[router][input];
}


QHostAddress SaParser::inputStreamAddress(int router,int input) const
{
  return sa_input_stream_addresses[router][input];
}


int SaParser::outputQuantity(int router) const
{
  return sa_output_names[router].size();
}


bool SaParser::outputIsReal(int router,int output) const
{
  return sa_output_is_reals[router][output];
}


QString SaParser::outputNodeName(int router,int output) const
{
  return sa_output_node_names[router][output];
}


QHostAddress SaParser::outputNodeAddress(int router,int output) const
{
  return sa_output_node_addresses[router][output];
}


int SaParser::outputNodeSlotNumber(int router,int output) const
{
  return sa_output_node_slot_numbers[router][output];
}


QString SaParser::outputName(int router,int output) const
{
  return sa_output_names[router][output];
}


QString SaParser::outputLongName(int router,int output) const
{
  return sa_output_long_names[router][output];
}


int SaParser::outputCrosspoint(int router,int output) const
{
  return sa_output_xpoints[router][output];
}


void SaParser::setOutputCrosspoint(int router,int output,int input)
{
  SendCommand(QString::asprintf("ActivateRoute %d %d %d",router,output,input));
}


QString SaParser::gpiState(int router,int input) const
{
  return sa_gpi_states[router][input];
}


void SaParser::setGpiState(int router,int input,const QString &code,int msec)
{
  if(msec<0) {
    SendCommand(QString::asprintf("TriggerGPI %d %d %s",router,input,
				  code.toUtf8().constData()));
  }
  else {
    SendCommand(QString::asprintf("TriggerGPI %d %d %s %d",router,input,
				  code.toUtf8().constData(),msec));
  }
}


QString SaParser::gpoState(int router,int output) const
{
  return sa_gpo_states[router][output];
}


void SaParser::setGpoState(int router,int output,const QString &code,int msec)
{
  if(msec<0) {
    SendCommand(QString::asprintf("TriggerGPO %d %d %s",router,output,
				  code.toUtf8().constData()));
  }
  else {
    SendCommand(QString::asprintf("TriggerGPO %d %d %s %d",router,output,
				  code.toUtf8().constData(),msec));
  }
}


int SaParser::snapshotQuantity(int router) const
{
  return sa_snapshot_names[router].size();
}


QString SaParser::snapshotName(int router,int n) const
{
  return sa_snapshot_names[router].at(n);
}


void SaParser::activateSnapshot(int router,const QString &snapshot)
{
  SendCommand(QString::asprintf("ActivateSnap %d ",router)+snapshot);
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
  QString ret=tr("Unknown")+QString::asprintf(" [%d]",cstate);

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
  sa_connected=false;
  Clear();
  emit connected(false,SaParser::WatchdogActive);
  sa_holdoff_timer->start(SAPARSER_HOLDOFF_INTERVAL);
}


void SaParser::startupData()
{
  sa_connected=true;
  emit connected(true,SaParser::Ok);
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
      /*
      if(sa_accum.left(2)==">>") {
	sa_accum=sa_accum.right(sa_accum.length()-2);
      }
      */
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


void SaParser::Clear()
{
  sa_router_names.clear();
  sa_input_node_names.clear();
  sa_input_node_slot_numbers.clear();
  sa_input_node_addresses.clear();
  sa_input_names.clear();
  sa_input_long_names.clear();
  sa_input_source_numbers.clear();
  sa_input_stream_addresses.clear();
  sa_output_node_names.clear();
  sa_output_node_addresses.clear();
  sa_output_node_slot_numbers.clear();
  sa_output_names.clear();
  sa_output_long_names.clear();
  sa_output_xpoints.clear();
  sa_gpi_states.clear();
  sa_gpo_states.clear();
  sa_gpio_supporteds.clear();
  sa_snapshot_names.clear();
}


void SaParser::DispatchCommand(QString cmd)
{
  //printf("RECV: %s\n",(const char *)cmd.toUtf8());

  bool ok=false;

  QStringList f0=cmd.replace(">>","").toLower().split(" ");

  //
  // Process Login
  //
  if(f0[0]=="login") {
    if((f0.size()==2)&&(f0[1]=="successful")) {
      SendCommand("RouterNames");
    }
    else {
      sa_socket->deleteLater();
      sa_socket=NULL;
      sa_connected=false;
      Clear();
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
	  sa_input_source_numbers[sa_current_router].clear();
	  sa_input_stream_addresses[sa_current_router].clear();
	  sa_prev_input=0;
	  sa_reading_sources=true;
	}
	if(f0[1]=="destnames") {
	  sa_output_names[sa_current_router].clear();
	  sa_output_long_names[sa_current_router].clear();
	  sa_prev_output=0;
	  sa_reading_dests=true;
	}
	if(f0[1]=="snapshotnames") {
	  sa_snapshot_names[sa_current_router].clear();
	  sa_reading_snapshots=true;
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
	  SendCommand(QString::asprintf("SourceNames %u",it.key()));
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
	    SendCommand(QString::asprintf("DestNames %u",it.key()));
	  }
	}
	if((f0[1]=="destnames")&&(sa_current_router==sa_last_router)) {
	  sa_reading_dests=false;
	  emit outputListChanged();
	  for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
	      it!=sa_router_names.end();it++) {
	    SendCommand(QString::asprintf("Snapshots %u",it.key()));
	  }
	}
	if((f0[1]=="snapshotnames")&&(sa_current_router==sa_last_router)) {
	  for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
	      it!=sa_router_names.end();it++) {
	    if(sa_output_names[it.key()].size()>0) {
	      SendCommand(QString::asprintf("RouteStat %u\r\n",it.key()));
	      sa_last_xpoint_router=it.key();
	      QMap<int,QString>::const_iterator it2=
		sa_output_names[it.key()].end();
	      it2--;
	      sa_last_xpoint_output=it2.key();
	    }
	  }
	  sa_reading_snapshots=false;
	  sa_reading_xpoints=true;
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
  if(sa_reading_snapshots) {
    ReadSnapshotName(cmd);
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
	  if((router==sa_last_xpoint_router)&&(output==sa_last_xpoint_output)) {
	    sa_last_xpoint_router=-1;
	    sa_last_xpoint_output=-1;

	    for(QMap<int,QString>::const_iterator it=sa_router_names.begin();
		it!=sa_router_names.end();it++) {
	      SendCommand(QString::asprintf("GPIStat %d",it.key()));
	      SendCommand(QString::asprintf("GPOStat %d",it.key()));
	    }
	    sa_startup_timer->start(SAPARSER_STARTUP_INTERVAL);
	  }
	}
      }
    }
  }

  //
  // Process GPI State Changes
  //
  if((f0[0]=="gpistat")&&(f0.size()==4)) {
    int router=f0[1].toUInt(&ok);
    sa_gpio_supporteds[router]=true;
    if(ok) {
      int input=f0[2].toInt(&ok);
      if(ok) {
	sa_gpi_states[router][input]=f0[3];
	emit gpiStateChanged(router,input,f0[3]);
      }
    }
  }

  //
  // Process GPO State Changes
  //
  if((f0[0]=="gpostat")&&(f0.size()==4)) {
    int router=f0[1].toUInt(&ok);
    if(ok) {
      int output=f0[2].toInt(&ok);
      if(ok) {
	sa_gpo_states[router][output]=f0[3];
	emit gpoStateChanged(router,output,f0[3]);
      }
    }
  }

  //  printf("CMD: %s\n",(const char *)cmd.toUtf8());
}


void SaParser::ReadRouterName(const QString &cmd)
{
  bool ok=false;
  QStringList f0=cmd.split(" ",QString::SkipEmptyParts);
  if(f0.size()>=2) {
    for(int i=2;i<f0.size();i++) {
      f0[1]+=" "+f0[i];
    }
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
  QHostAddress addr;

  if(ok) {
    for(int i=sa_prev_input+1;i<input;i++) {
      sa_input_is_reals[sa_current_router][i]=false;
    }
    if(f0.size()==8) {
      srcnum=f0[6].toInt(&ok);
    }
    if(f0.size()>=3) {
      QStringList f1=f0.at(2).split("ON");
      sa_input_node_names[sa_current_router][input]=f1.back().trimmed();
      if(addr.setAddress(f0.at(3))) {
	sa_input_node_addresses[sa_current_router][input]=addr;
      }
      int slot=f0.at(5).toUInt(&ok);
      if(ok) {
	sa_input_node_slot_numbers[sa_current_router][input]=slot-1;
      }
      if(f0[1].trimmed().isEmpty()) {
	if(sa_gpio_supporteds[sa_current_router]) {
	  sa_input_names[sa_current_router][input]=
	    tr("GPI")+QString::asprintf(" %d",input);
	  sa_input_long_names[sa_current_router][input]=
	    tr("GPI")+QString::asprintf(" %d",input)+f0[2];
	}
	else {
	  sa_input_names[sa_current_router][input]=
	    tr("Input")+QString::asprintf(" %d",input);
	  sa_input_long_names[sa_current_router][input]=
	    tr("Input")+QString::asprintf(" %d",input)+f0[2];
	}
      }
      else {
	sa_input_names[sa_current_router][input]=f0[1];
	sa_input_long_names[sa_current_router][input]=f0[2];
      }
      sa_input_is_reals[sa_current_router][input]=true;
      if(ok) {
	if(srcnum<=0) {
	  //	  sa_input_long_names[sa_current_router][input]=f0[2];
	  sa_input_source_numbers[sa_current_router][input]=-1;
	  sa_input_stream_addresses[sa_current_router][input]=QHostAddress();
	}
	else {
	  //	  sa_input_long_names[sa_current_router][input]=f0[2];
	  sa_input_source_numbers[sa_current_router][input]=f0.at(6).toInt();
	  QHostAddress addr;
	  if(addr.setAddress(f0.at(7))) {
	    sa_input_stream_addresses[sa_current_router][input]=addr;
	  }
	  else {
	    sa_input_stream_addresses[sa_current_router][input]=QHostAddress();
	  }
	}
      }
      else {
	sa_input_long_names[sa_current_router][input]=f0[2];
	sa_input_source_numbers[sa_current_router][input]=-1;
	sa_input_stream_addresses[sa_current_router][input]=QHostAddress();
      }
    }
    sa_prev_input=input;
  }
}


void SaParser::ReadDestName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");
  bool ok=false;
  int output=f0.at(0).toInt(&ok);
  QHostAddress addr;

  if(f0.size()>=3) {
    for(int i=sa_prev_output+1;i<output;i++) {
      sa_output_is_reals[sa_current_router][i]=false;
    }
    if(ok) {
      QStringList f1=f0.at(2).split("ON");
      sa_output_node_names[sa_current_router][output]=f1.back().trimmed();
      sa_output_names[sa_current_router][output]=f0.at(1);
      sa_output_is_reals[sa_current_router][output]=true;
      sa_output_long_names[sa_current_router][output]=f0.at(2);
      if(f0.size()>=4) {
	if(addr.setAddress(f0.at(3))) {
	  sa_output_node_addresses[sa_current_router][output]=addr;
	}
	if(f0.size()>=6) {
	  int slot=f0.at(5).toUInt(&ok);
	  if(ok) {
	    sa_output_node_slot_numbers[sa_current_router][output]=slot-1;
	  }
	}
      }
    }
    sa_prev_output=output;
  }
}


void SaParser::ReadSnapshotName(const QString &cmd)
{
  sa_snapshot_names[sa_current_router].push_back(cmd.trimmed());
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
  connect(sa_socket,SIGNAL(disconnected()),
	  this,SLOT(connectionClosedData()));
  connect(sa_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(sa_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}
