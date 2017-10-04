// dparser.cpp
//
// Parser for Protocol D
//
// (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <QStringList>

#include "dparser.h"

DParser::DParser(QObject *parent)
  : QObject(parent)
{
  d_socket=NULL;
  d_connected=false;

  d_poll_timer=new QTimer(this);
  d_poll_timer->setSingleShot(true);
  connect(d_poll_timer,SIGNAL(timeout()),this,SLOT(pollTimerData()));

  d_watchdog_timer=new QTimer(this);
  d_watchdog_timer->setSingleShot(true);
  connect(d_watchdog_timer,SIGNAL(timeout()),this,SLOT(watchdogTimerData()));
}


QList<QHostAddress> DParser::nodeHostAddresses() const
{
  QList<QHostAddress> addrs;

  for(QMap<unsigned,SyNode *>::const_iterator it=d_nodes.begin();
      it!=d_nodes.end();it++) {
    addrs.push_back(it.value()->hostAddress());
  }
  return addrs;
}


SyNode *DParser::node(const QHostAddress &hostaddr)
{
  try {
    return d_nodes[hostaddr.toIPv4Address()];
  }
  catch(...) {
  }
  return NULL;
}


SySource *DParser::src(const QHostAddress &hostaddr,int slot) const
{
  try {
    return d_sources[ToIndex(hostaddr,slot)];
  }
  catch(...) {
  }
  return NULL;
}


SyDestination *DParser::dst(const QHostAddress &hostaddr,int slot) const
{
  try {
    return d_destinations[ToIndex(hostaddr,slot)];
  }
  catch(...) {
  }
  return NULL;
}


void DParser::connectToHost(const QString &hostname,uint16_t port)
{
  printf("connectToHost(%s,%u)\n",(const char *)hostname.toUtf8(),0xFFFF&port);
  d_hostname=hostname;
  d_port=port;

  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(d_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  d_socket->connectToHost(d_hostname,d_port);
}


void DParser::connectedData()
{
  printf("connectedData()\n");
  SendCommand("SubscribeDestinations");
  SendCommand("SubscribeNodes");
  SendCommand("SubscribeSources");
  SendCommand("Ping");
}


void DParser::readyReadData()
{
  char data[1501];
  int n;
  
  while((n=d_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(0xFF&data[i]) {
      case 13:
	ProcessCommand(d_accum);
	d_accum="";
	break;

      case 10:
	break;

      default:
	d_accum+=0xFF&data[i];
	break;
      }
    }
  }
}


void DParser::errorData(QAbstractSocket::SocketError err)
{
  printf("errorData(%d)\n",err);
  QString err_msg=tr("Socket Error")+QString().sprintf(" %u",err);

  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
    err_msg=tr("connection refused");
    break;

  case QAbstractSocket::RemoteHostClosedError:
    err_msg=tr("remote host closed connection");
    break;

  case QAbstractSocket::HostNotFoundError:
    err_msg=tr("host not found");
    break;

  case QAbstractSocket::SocketAccessError:
    err_msg=tr("socket access error");
    break;

  case QAbstractSocket::SocketTimeoutError:
    err_msg=tr("connection timed out");
    break;

  case QAbstractSocket::NetworkError:
    err_msg=tr("network error");
    break;

  default:
    break;
  }

  emit error(err,err_msg);

  d_watchdog_timer->stop();
  watchdogTimerData();
}


void DParser::pollTimerData()
{
  SendCommand("Ping");
}


void DParser::watchdogTimerData()
{
  delete d_socket;
  d_socket=NULL;
  if(d_connected) {
    d_connected=false;
    emit connected(false);
  }
  connectToHost(d_hostname,d_port);
}


void DParser::ProcessCommand(const QString &cmd)
{
  QStringList cmds=cmd.split("\t");  
  QHostAddress addr;
  SyNode *node;
  SyDestination *dst;
  SySource *src;
  int slot;
  uint64_t sindex;
  uint64_t dindex;
  bool ok=false;
  bool changed=false;

  if((cmds.at(0).toLower()=="dst")&&(cmds.size()==7)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((dindex=ToIndex(addr,slot))>=0) {
	  if((dst=d_destinations[dindex])!=NULL) {
	    QHostAddress saddr(cmds.at(4));
	    if(dst->name()!=cmds.at(5)) {
	      dst->setName(cmds.at(5));
	      changed=true;
	    }
	    if(dst->channels()!=cmds.at(6).toUInt()) {
	      dst->setChannels(cmds.at(6).toUInt());
	      changed=true;
	    }
	    if(dst->streamAddress()!=saddr) {
	      dst->setStreamAddress(saddr);
	      if((sindex=IndexByStreamAddress(saddr))>0) {
		emit crosspointChanged(addr,slot,
				       ToAddress(sindex),ToSlot(sindex));
	      }
	      else {
		emit crosspointCleared(addr,slot);
	      }
	      changed=true;
	    }
	    if(changed) {
	      emit destinationChanged(addr,slot,dst);
	    }
	  }
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="dstadd")&&(cmds.size()==7)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((dindex=ToIndex(addr,slot))>=0) {
	  if(d_destinations[dindex]==NULL) {
	    dst=new SyDestination();
	    dst->setStreamAddress(cmds.at(4));
	    dst->setName(cmds.at(5));
	    dst->setChannels(cmds.at(6).toInt());
	    d_destinations[dindex]=dst;
	    emit destinationAdded(addr,slot);
	  }
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="dstdel")&&(cmds.size()==3)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((dindex=ToIndex(addr,slot))>=0) {
	  if((dst=d_destinations[dindex])!=NULL) {
	    delete dst;
	    d_destinations.erase(d_destinations.find(dindex));
	    emit destinationRemoved(addr,slot);
	  }
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="nodeadd")&&(cmds.size()==8)) {
    if(addr.setAddress(cmds.at(1))) {
      if(d_nodes[addr.toIPv4Address()]==NULL) {
	node=new SyNode();
	node->setHostAddress(addr);
	node->setHostName(cmds.at(2));
	node->setDeviceName(cmds.at(3));
	node->setSrcSlotQuantity(cmds.at(4).toInt());
	node->setDstSlotQuantity(cmds.at(5).toInt());
	node->setGpiSlotQuantity(cmds.at(6).toInt());
	node->setGpoSlotQuantity(cmds.at(7).toInt());
	d_nodes[addr.toIPv4Address()]=node;
	emit nodeAdded(addr);
      }
    }
  }

  if((cmds.at(0).toLower()=="nodedel")&&(cmds.size()==2)) {
    if(addr.setAddress(cmds.at(1))) {
      if((node=d_nodes[addr.toIPv4Address()])!=NULL) {
	delete node;
	d_nodes.erase(d_nodes.find(addr.toIPv4Address()));
	emit nodeRemoved(addr);
      }
    }
  }

  if(cmds.at(0).toLower()=="pong") {
    if(!d_connected) {
      d_connected=true;
      emit connected(true);
    }
    d_watchdog_timer->stop();
    d_watchdog_timer->start(DPARSER_WATCHDOG_TIMEOUT_INTERVAL);
    d_poll_timer->start(DPARSER_WATCHDOG_POLL_INTERVAL);
  }

  if((cmds.at(0).toLower()=="src")&&(cmds.size()==9)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((sindex=ToIndex(addr,slot))>=0) {
	  if((src=d_sources[sindex])!=NULL) {
	    QHostAddress saddr(cmds.at(4));
	    if(src->streamAddress()!=saddr) {
	      src->setStreamAddress(saddr);
	      changed=true;
	    }
	    if(src->name()!=cmds.at(5)) {
	      src->setName(cmds.at(5));
	      changed=true;
	    }
	    if(src->enabled()!=(cmds.at(6)!="0")) {
	      src->setEnabled(cmds.at(6)!="0");
	      changed=true;
	    }
	    if(src->channels()!=cmds.at(7).toUInt()) {
	      src->setChannels(cmds.at(7).toUInt());
	      changed=true;
	    }
	    if(src->packetSize()!=cmds.at(8).toUInt()) {
	      src->setPacketSize(cmds.at(8).toUInt());
	      changed=true;
	    }
	    if(changed) {
	      emit sourceChanged(addr,slot,src);
	    }
	  }
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="srcadd")&&(cmds.size()==9)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((sindex=ToIndex(addr,slot))>=0) {
	  if(d_sources[sindex]==NULL) {
	    src=new SySource();
	    src->setStreamAddress(QHostAddress(cmds.at(4)));
	    src->setName(cmds.at(5));
	    src->setEnabled(cmds.at(6)!="0");
	    src->setChannels(cmds.at(7).toInt());
	    src->setPacketSize(cmds.at(8).toInt());
	    d_sources[sindex]=src;
	    emit sourceAdded(addr,slot);
	  }
	}
      }
    }
  }

  if((cmds.at(0).toLower()=="srcdel")&&(cmds.size()==3)) {
    if(addr.setAddress(cmds.at(1))) {
      slot=cmds.at(2).toInt(&ok);
      if(ok) {
	if((sindex=ToIndex(addr,slot))>=0) {
	  if((src=d_sources[sindex])!=NULL) {
	    delete src;
	    d_sources.erase(d_sources.find(sindex));
	    emit sourceRemoved(addr,slot);
	  }
	}
      }
    }
  }
}


void DParser::SendCommand(const QString &cmd)
{
  if(d_socket!=NULL) {
    d_socket->write((cmd+"\r\n").toUtf8(),cmd.length()+2);
  }
}


uint64_t DParser::IndexByStreamAddress(const QHostAddress &saddr) const
{
  for(QMap<uint64_t,SySource *>::const_iterator it=d_sources.begin();
      it!=d_sources.end();it++) {
    if(it.value()->streamAddress()==saddr) {
      return it.key();
    }
  }
  return 0;
}


uint64_t DParser::ToIndex(const QHostAddress &addr,int slot) const
{
  return (((uint64_t)addr.toIPv4Address())<<32)+(uint64_t)slot;
}


QHostAddress DParser::ToAddress(uint64_t index) const
{
  return QHostAddress(index>>32);
}


int DParser::ToSlot(uint64_t index) const
{
  return 0xFFFFFFFF&index;
}
