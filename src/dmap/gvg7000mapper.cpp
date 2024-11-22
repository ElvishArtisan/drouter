// gvg7000mapper.cpp
//
// Generate a Drouter map from a GVG7000 device
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QDateTime>

#include <sy5/symcastsocket.h>

#include "gvg7000mapper.h"

Gvg7000Mapper::Gvg7000Mapper(DREndPointMap::RouterType type,const
			     QString &name,int number,QObject *parent)
  : QObject(parent)
{
  //
  // Endpoint Map
  //
  d_map=new DREndPointMap();
  d_map->setRouterType(type);
  d_map->setRouterName(name);
  d_map->setRouterNumber(number-1);

  //
  // Connection Socket
  //
  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(d_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}


Gvg7000Mapper::~Gvg7000Mapper()
{
  delete d_map;
  delete d_socket;
}


DREndPointMap *Gvg7000Mapper::endpointMap() const
{
  return d_map;
}


void Gvg7000Mapper::readDevice(const QString &hostname,uint16_t port)
{
  if(d_map->routerType()!=DREndPointMap::AudioRouter) {
    emit completed(false,"unsupported router type");
    return;
  }
  d_err_msg="OK";
  d_socket->connectToHost(hostname,port);
}


void Gvg7000Mapper::connectedData()
{
  SendGvgCommand("QN,IS");  // Enumerate sources
  SendGvgCommand("QN,ID");  // Enumerate destinations
  SendGvgCommand("QT");     // Request Date/Time
}


void Gvg7000Mapper::disconnectedData()
{
  emit completed(false,"remote end closed connection");
}


void Gvg7000Mapper::readyReadData()
{
  QByteArray data=d_socket->readAll();

  for(int i=0;i<data.size();i++) {
    switch(data.at(i)) {
    case 1:  // SOH
      d_raw_accum.clear();
      break;

    case 4:  // EOT
      DispatchGvgCommand(d_raw_accum);
      break;

    default:
      d_raw_accum+=data.at(i);
      break;
    }
  }
}


void Gvg7000Mapper::errorData(QAbstractSocket::SocketError err)
{
  emit completed(false,SyMcastSocket::socketErrorText(err));
}


void Gvg7000Mapper::DispatchGvgCommand(const QByteArray &msg)
{
  bool ok=GvgVerifyChecksum(msg);

  if(ok) {
    if(msg.at(0)=='N') {
      d_layer4_accum+=msg.mid(2,msg.length()-4);
      ProcessGvgCommand(d_layer4_accum);
      d_layer4_accum.clear();
    }
    else {
      fprintf(stderr,
	    "Gvg7000Mapper: received GVG7000 message \"%s\" with unknown protocol ID \"%c\"\n",
	      GvgPrettify(msg).toUtf8().constData(),0xff&msg.at(0));
    }
  }
  else {
    fprintf(stderr,"Gvg7000Mapper: received invalid GVG7000 message \"%s\"\n",
	    GvgPrettify(msg).toUtf8().constData());
  }
}


void Gvg7000Mapper::ProcessGvgCommand(const QByteArray &msg)
{
  bool ok=false;
  bool was_processed=false;
  QStringList f0=QString(msg).split('\t',Qt::KeepEmptyParts);

  if((f0.at(0)=="ST")&&(f0.size()==2)) {
    //
    // Save and Exit
    //
    emit completed(true,d_err_msg);
  }
  
  if((f0.at(0)=="NQ")&&(f0.size()>=2)) {
    if((f0.at(1)=="S")&&(f0.size()>=3)) {   // Sources
      int size=f0.at(2).toInt();
      for(int i=0;i<size;i++) {
	QString name=f0.at(3+i*4);
	unsigned num=f0.at(3+i*4+1).toUInt(&ok,16);
	if(ok) {
	  d_map->
	    insert(DREndPointMap::Input,num,d_socket->peerAddress(),num,name);
	}
      }
      was_processed=true;
    }

    if((f0.at(1)=="D")&&(f0.size()>=3)) {   // Destinations
      int size=f0.at(2).toInt();
      for(int i=0;i<size;i++) {
	QString name=f0.at(3+i*4);
	unsigned num=f0.at(3+i*4+1).toInt(&ok,16);
	if(ok) {
	  d_map->
	    insert(DREndPointMap::Output,num,d_socket->peerAddress(),num,name);
	}
      }
      was_processed=true;
    }
  }

  if(!was_processed) {
    fprintf(stderr,"received unimplemented GVG7000 message [%s]\n",
	   GvgPrettify(msg).toUtf8().constData());
  }
}


void Gvg7000Mapper::SendGvgCommand(const QString &str)
{
  d_socket->write(ToGvgNative(str));
}


QByteArray Gvg7000Mapper::ToGvgNative(QString str) const
{
  str.replace(",","\t");
  if(str.right(1)!="\t") {
    str+="\t";
  }

  QString msg=QString("N")+  // Protocol ID
    "0"+                     // Sequence Flag
    str;                     // Data

  uint8_t sum=GvgChecksum(msg.toUtf8());

  QString ret=QChar(1)+             // Header
    msg+                            // Message
    QString::asprintf("%02X",sum)+  // Checksum
    QChar(4);                       // Footer

  return ret.toUtf8();
}


bool Gvg7000Mapper::GvgVerifyChecksum(const QByteArray &msg) const
{
  uint8_t sum=GvgChecksum(msg.left(msg.length()-2));

  return QString::asprintf("%02X",0xff&sum)==QString(msg.right(2).constData());
}


uint8_t Gvg7000Mapper::GvgChecksum(const QByteArray &msg) const
{
  uint8_t sum=0;

  for(int i=0;i<msg.length();i++) {
    sum+=msg.at(i);
  }
  sum=0x100-sum;

  return sum;
}


QString Gvg7000Mapper::GvgPrettify(const QByteArray &msg) const
{
  return QString(msg).replace("\t",",");
}
