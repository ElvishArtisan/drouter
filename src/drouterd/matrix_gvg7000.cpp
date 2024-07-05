// matrix_gvg7000.cpp
//
// Router matrix implementation for Broadcast Tools Universal 4.1 MLR>>Web
//
// (C) 2024 Fred Gleason <fredg@paravelsystems.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of version 2.1 of the GNU Lesser General Public
//    License as published by the Free Software Foundation;
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, 
//    Boston, MA  02111-1307  USA
//

#include <syslog.h>

#include <QStringList>

#include "matrix_gvg7000.h"

MatrixGvg7000::MatrixGvg7000(unsigned id,Config *conf,QObject *parent)
  : Matrix(Config::Gvg7000Matrix,id,conf,parent)
{
  d_host_port=0;
  d_connected=false;

  //
  // Connection Socket
  //
  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  d_poll_timer=new QTimer(this);
  connect(d_poll_timer,SIGNAL(timeout()),this,SLOT(pollData()));

  //
  // Watchdog
  //
  d_reconnect_timer=new QTimer(this);
  d_reconnect_timer->setSingleShot(true);
  connect(d_reconnect_timer,SIGNAL(timeout()),this,SLOT(reconnectData()));
  d_watchdog=new Watchdog(this);
  connect(d_watchdog,SIGNAL(poll()),this,SLOT(watchdogPollData()));
  connect(d_watchdog,SIGNAL(timeout()),this,SLOT(watchdogTimeoutData()));
}


MatrixGvg7000::~MatrixGvg7000()
{
  delete d_socket;

  for(QMap<int,SySource *>::const_iterator it=d_sources.begin();
      it!=d_sources.end();it++) {
    delete it.value();
  }
  for(QMap<int,SyDestination *>::const_iterator it=d_destinations.begin();
      it!=d_destinations.end();it++) {
    delete it.value();
  }
}


bool MatrixGvg7000::isConnected() const
{
  return d_connected;
}


QHostAddress MatrixGvg7000::hostAddress() const
{
  return d_host_address;
}


QString MatrixGvg7000::hostName() const
{
  return d_host_address.toString();
}


QString MatrixGvg7000::deviceName() const
{
  return Config::matrixTypeString(matrixType());
}


QString MatrixGvg7000::description() const
{
  return QString("Grass Valley 7000 Compatible Device");
}


unsigned MatrixGvg7000::dstSlots() const
{
  return d_destinations.size();
}


unsigned MatrixGvg7000::srcSlots() const
{
  return d_sources.size();
}


SySource *MatrixGvg7000::src(int slot) const
{
  return d_sources.value(slot);
}


SyDestination *MatrixGvg7000::dst(int slot) const
{
  return d_destinations.value(slot);
}


int MatrixGvg7000::srcNumber(int slot) const
{
  return slot+1;
}


QHostAddress MatrixGvg7000::srcAddress(int slot) const
{
  return d_sources.value(slot)->streamAddress();
}


QString MatrixGvg7000::srcName(int slot) const
{
  return d_sources.value(slot)->name();
}


bool MatrixGvg7000::srcEnabled(int slot) const
{
  return true;
}


unsigned MatrixGvg7000::srcChannels(int slot) const
{
  return 2;
}


QHostAddress MatrixGvg7000::dstAddress(int slot) const
{
  return d_destinations.value(slot)->streamAddress();
}


void MatrixGvg7000::setDstAddress(int slot,const QHostAddress &s_addr)
{
  if(d_destinations.value(slot)->streamAddress()!=s_addr) {
    int src_slot=s_addr.toIPv4Address();
    if(src_slot>=0) {  // Mute is not supported!
      SendGvgCommand(QString::asprintf("TI,%02X,%02X",slot,src_slot-1));
    }
  }
}


void MatrixGvg7000::setDstAddress(int slot,const QString &s_addr)
{
  setDstAddress(slot,QHostAddress(s_addr));
}


QString MatrixGvg7000::dstName(int slot) const
{
  return d_destinations.value(slot)->name();
}


unsigned MatrixGvg7000::dstChannels(int slot) const
{
  return 2;
}


void MatrixGvg7000::connectToHost(const QHostAddress &addr,uint16_t port,
				  const QString &pwd,bool persistent)
{
  d_host_address=addr;
  d_host_port=port;

  d_socket->connectToHost(d_host_address,d_host_port);
  //  d_watchdog->start();
}


void MatrixGvg7000::connectedData()
{
  d_node.setHostAddress(d_host_address);
  d_node.setDeviceName(Config::matrixTypeString(Config::Gvg7000Matrix));
  d_node.setProductName("Grass Valley Series 7000 Protocol");
  d_node.setSrcSlotQuantity(0);
  d_node.setDstSlotQuantity(0);

  SendGvgCommand("QN,IS");  // Enumerate sources
  SendGvgCommand("QN,ID");  // Enumerate destinations
  SendGvgCommand("BK,D");   // Request crosspoint states
  SendGvgCommand("QJ");
  SendGvgCommand("QT");     // Indicate that initialization is complete

  d_poll_timer->start(200);
}


void MatrixGvg7000::disconnectedData()
{
  emit connected(id(),false);

  d_poll_timer->stop();
  d_connected=false;
  d_socket->deleteLater();
  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));

  d_reconnect_timer->start(0);
}


void MatrixGvg7000::readyReadData()
{
  QByteArray data=d_socket->readAll();
  //  printf("RECV: %s\n",data.constData());
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


void MatrixGvg7000::reconnectData()
{
  connectToHost(d_host_address,d_host_port,"",false);
}


void MatrixGvg7000::pollData()
{
  SendGvgCommand("QJ");
}


void MatrixGvg7000::watchdogPollData()
{
  SendGvgCommand("QT");
}


void MatrixGvg7000::watchdogTimeoutData()
{
  d_socket->close();
}


void MatrixGvg7000::DispatchGvgCommand(const QByteArray &msg)
{
  bool ok=GvgVerifyChecksum(msg);

  if(ok) {
    if(msg.at(0)=='N') {
      d_layer4_accum+=msg.mid(2,msg.length()-4);
      ProcessGvgCommand(d_layer4_accum);
      d_layer4_accum.clear();
    }
    else {
      syslog(LOG_WARNING,
	     "received GVG7000 message \"%s\" with unknown protocol ID \"%c\"",
	     GvgPrettify(msg).toUtf8().constData(),0xff&msg.at(0));
    }
  }
  else {
    syslog(LOG_WARNING,"received invalid GVG7000 message \"%s\"",
	   GvgPrettify(msg).toUtf8().constData());
  }
}


void MatrixGvg7000::ProcessGvgCommand(const QByteArray &msg)
{
  bool ok=false;
  bool was_processed=false;
  QStringList f0=QString(msg).split('\t',Qt::KeepEmptyParts);

  if((f0.at(0)=="ST")&&(f0.size()==2)) {
    QDateTime dt=QDateTime::fromString(f0.at(1),"yyyyddMMhhmmss");
    syslog(LOG_DEBUG,
	   "date/time on GVG7000 device at connection %s:%u is: %s UTC\n",
	   d_socket->peerAddress().toString().toUtf8().constData(),
	   0xffff&d_socket->peerPort(),
	   dt.toString("yyyy-MM-dd hh:mm:ss").toUtf8().constData());
    was_processed=true;

    d_connected=true;
    emit connected(id(),true);
  }

  if((f0.at(0)=="JQ")&&(f0.size()>=3)) {
    int dst_num=f0.at(1).toInt(&ok,16);
    if(ok) {
      int src_quan=f0.at(2).toInt();
      if((src_quan>0)&&(f0.size()>=6)) {
	int src_num=f0.at(5).toUInt(&ok,16);  // FIXME: support multiple sources
	if(ok&&(dst_num<d_destinations.size())&&(src_num<d_sources.size())) {
	  QHostAddress s_addr(1+src_num);
	  if(d_destinations.value(dst_num)->streamAddress()!=s_addr) {
	    d_destinations.value(dst_num)->setStreamAddress(s_addr);
	    emit destinationChanged(id(),dst_num,d_node,
				    *(d_destinations.value(dst_num)));
	    printf("destChanged: %d:%s\n",dst_num,d_destinations.value(dst_num)->streamAddress().toString().toUtf8().constData());
	  }
	}
      }
    }
    was_processed=true;
  }

  if((f0.at(0)=="NQ")&&(f0.size()>=2)) {
    if((f0.at(1)=="S")&&(f0.size()>=3)) {   // Sources
      int size=f0.at(2).toInt();
      for(int i=0;i<size;i++) {
	QString name=f0.at(3+i*4);
	unsigned num=f0.at(3+i*4+1).toUInt(&ok,16);
	if(ok) {
	  SySource *src=d_sources.value(num);
	  if(src==NULL) {
	    src=new SySource();
	    d_sources[num]=src;
	  }
	  src->setName(name);
	  src->setChannels(2);
	  src->setStreamAddress(QHostAddress(1+num));
	  if(num>d_node.srcSlotQuantity()) {
	    d_node.setSrcSlotQuantity(num);
	  }
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
	  SyDestination *dst=d_destinations.value(num);
	  if(dst==NULL) {
	    dst=new SyDestination();
	    d_destinations[num]=dst;
	  }
	  dst->setName(name);
	  dst->setChannels(2);
	  if(num>d_node.dstSlotQuantity()) {
	    d_node.setDstSlotQuantity(num);
	  }
	}
      }
      was_processed=true;
    }

    //    printf("RECV: %s\n",msg.constData());
  }

  if(!was_processed) {
    syslog(LOG_INFO,"received unimplemented GVG7000 message [%s]",
	   GvgPrettify(msg).toUtf8().constData());
  }
}


void MatrixGvg7000::SendGvgCommand(const QString &str)
{
  //  printf("SEND: %s\n",GvgPrettify(str.toUtf8()).toUtf8().constData());
  d_socket->write(ToGvgNative(str));
}


QByteArray MatrixGvg7000::ToGvgNative(QString str) const
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


bool MatrixGvg7000::GvgVerifyChecksum(const QByteArray &msg) const
{
  uint8_t sum=GvgChecksum(msg.left(msg.length()-2));

  return QString::asprintf("%02X",0xff&sum)==QString(msg.right(2).constData());
}


uint8_t MatrixGvg7000::GvgChecksum(const QByteArray &msg) const
{
  uint8_t sum=0;

  for(int i=0;i<msg.length();i++) {
    sum+=msg.at(i);
  }
  sum=0x100-sum;

  return sum;
}


QString MatrixGvg7000::GvgPrettify(const QByteArray &msg) const
{
  return QString(msg).replace("\t",",");
}
