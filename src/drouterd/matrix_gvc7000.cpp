// matrix_gvc7000.cpp
//
// Router matrix implementation for Broadcast Tools Universal 4.1 MLR>>Web
//
// (C) 2023 Fred Gleason <fredg@paravelsystems.com>
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

#include <QStringList>

#include "matrix_gvc7000.h"

MatrixGvc7000::MatrixGvc7000(unsigned id,Config *conf,QObject *parent)
  : Matrix(Config::Bt41MlrMatrix,id,conf,parent)
{
  d_host_port=0;
  d_connected=false;

  //
  // Sources
  //
  for(int i=0;i<MATRIX_BT41MLR_SOURCE_QUAN;i++) {
    d_sources[i]=
      new SySource(QHostAddress(QString::asprintf("%s.%d",MATRIX_BT41MLR_STREAM_ADDR_PREFIX,i+1)),
			      QString::asprintf("SRC %d",i+1),true);
  }

  //
  // Destinations
  //
  for(int i=0;i<MATRIX_BT41MLR_DEST_QUAN;i++) {
    d_destinations[i]=new SyDestination();
    d_destinations[i]->setName(QString::asprintf("DST %d",i+1));
    d_destinations[i]->setChannels(2);
    d_silence_alarms[i]=false;;
  }

  //
  // GPI Bundles
  //
  for(int i=0;i<MATRIX_BT41MLR_GPI_QUAN;i++) {
    d_gpio_bundles[i]=new SyGpioBundle();
    d_gpio_bundles[i]->setCode("hhhhh");
  }

  //
  // Connection Socket
  //
  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));

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


MatrixGvc7000::~MatrixGvc7000()
{
  delete d_socket;

  for(int i=0;i<MATRIX_BT41MLR_SOURCE_QUAN;i++) {
    delete d_sources[i];
  }
  for(int i=0;i<MATRIX_BT41MLR_DEST_QUAN;i++) {
    delete d_destinations[i];
  }
  for(int i=0;i<MATRIX_BT41MLR_GPI_QUAN;i++) {
    delete d_gpio_bundles[i];
  }
}


bool MatrixGvc7000::isConnected() const
{
  return d_connected;
  //  return d_socket->state()==QAbstractSocket::ConnectedState;
}


QHostAddress MatrixGvc7000::hostAddress() const
{
  return d_host_address;
}


QString MatrixGvc7000::hostName() const
{
  return d_host_address.toString();
}


QString MatrixGvc7000::deviceName() const
{
  return Config::matrixTypeString(matrixType());
}


unsigned MatrixGvc7000::dstSlots() const
{
  return MATRIX_BT41MLR_DEST_QUAN;
}


unsigned MatrixGvc7000::srcSlots() const
{
  return MATRIX_BT41MLR_SOURCE_QUAN;
}


SySource *MatrixGvc7000::src(int slot) const
{
  return d_sources[slot];
}


SyDestination *MatrixGvc7000::dst(int slot) const
{
  return d_destinations[slot];
}


int MatrixGvc7000::srcNumber(int slot) const
{
  return slot+1;
}


QHostAddress MatrixGvc7000::srcAddress(int slot) const
{
  return d_sources[slot]->streamAddress();
}


QString MatrixGvc7000::srcName(int slot) const
{
  return d_sources[slot]->name();
}


bool MatrixGvc7000::srcEnabled(int slot) const
{
  return true;
}


unsigned MatrixGvc7000::srcChannels(int slot) const
{
  return 2;
}


QHostAddress MatrixGvc7000::dstAddress(int slot) const
{
  return d_destinations[slot]->streamAddress();
}


void MatrixGvc7000::setDstAddress(int slot,const QHostAddress &s_addr)
{
  if(d_destinations[slot]->streamAddress()!=s_addr) {
    d_socket->
      write(QString::asprintf("*0%02d",0xff&s_addr.toIPv4Address()).toUtf8());
  }
}


void MatrixGvc7000::setDstAddress(int slot,const QString &s_addr)
{
  setDstAddress(slot,QHostAddress(s_addr));
}


QString MatrixGvc7000::dstName(int slot) const
{
  return d_destinations[slot]->name();
}


unsigned MatrixGvc7000::dstChannels(int slot) const
{
  return 2;
}


unsigned MatrixGvc7000::gpis() const
{
  return MATRIX_BT41MLR_GPI_QUAN;
}


SyGpioBundle *MatrixGvc7000::gpiBundle(int slot) const
{
  return d_gpio_bundles[slot];
}


bool MatrixGvc7000::silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				       int chan) const
{
  if(type==SyLwrpClient::OutputMeter) {
    return d_silence_alarms[slot];
  }
  return false;
}


void MatrixGvc7000::connectToHost(const QHostAddress &addr,uint16_t port,
				  const QString &pwd,bool persistent)
{
  d_host_address=addr;
  d_host_port=port;

  d_socket->connectToHost(d_host_address,d_host_port);
  d_watchdog->start();
}


void MatrixGvc7000::connectedData()
{
  d_node.setHostAddress(d_host_address);
  d_node.setDeviceName(Config::matrixTypeString(Config::Bt41MlrMatrix));
  d_node.setProductName("Broadcast Tools Universal 4.1 MLR>>Web");
  d_node.setSrcSlotQuantity(MATRIX_BT41MLR_SOURCE_QUAN);
  d_node.setDstSlotQuantity(MATRIX_BT41MLR_DEST_QUAN);

  d_socket->write("*0SL");   // Request audio crosspoint state
}


void MatrixGvc7000::disconnectedData()
{
  emit connected(id(),false);

  d_connected=false;
  d_socket->deleteLater();
  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));

  d_reconnect_timer->start(0);
}


void MatrixGvc7000::readyReadData()
{
  bool found=false;
  QByteArray data=d_socket->readAll();
  QStringList f0=QString::fromUtf8(data).trimmed().
    split(",",QString::KeepEmptyParts);
  if((f0.at(0)=="S0L")&&(f0.size()==5)) {  // Audio crosspoint changed
    for(int i=1;i<=MATRIX_BT41MLR_SOURCE_QUAN;i++) {
      if(f0.at(i)=="1") {
	QHostAddress s_addr(QString::asprintf("%s.%d",
				      MATRIX_BT41MLR_STREAM_ADDR_PREFIX,i));
	if(d_destinations[0]->streamAddress()!=s_addr) {
	  d_destinations[0]->setStreamAddress(s_addr);
	  emit destinationChanged(d_host_address.toIPv4Address(),
				  0,d_node,*(d_destinations[0]));
	}
	found=true;
      }
    }
    if(!found) {
      if(!d_destinations[0]->streamAddress().isNull()) {
	d_destinations[0]->setStreamAddress(QHostAddress());
	emit destinationChanged(d_host_address.toIPv4Address(),
				0,d_node,*(d_destinations[0]));
      }
    }
    if(!d_connected) {
      d_socket->write("*0SPA");  // Request GPI states
    }
  }

  if((f0.at(0)=="S0P")&&(f0.size()==7)) {  // GPI state changed
    QString code;
    for(int i=0;i<5;i++) {
      if(f0.at(2+i)=="1") {
	code+="l";
      }
      else {
	code+="h";
      }
    }
    d_gpio_bundles[0]->setCode(code);
    if(!d_connected) {
      d_connected=true;
      emit connected(id(),true);
    }
    emit gpiChanged(id(),0,d_node,*(d_gpio_bundles[0]));
  }

  if((f0.at(0)=="S0S")&&(f0.size()==2)) {  // Silence alarm state changed
    if(d_silence_alarms[0]!=f0.at(1)) {
      d_silence_alarms[0]=f0.at(1)=="1";
      emit audioSilenceAlarm(id(),SyLwrpClient::OutputMeter,0,0,
			     d_silence_alarms[0]);
      emit audioSilenceAlarm(id(),SyLwrpClient::OutputMeter,0,1,
			     d_silence_alarms[0]);
    }
    d_watchdog->touch();
  }
}


void MatrixGvc7000::reconnectData()
{
  connectToHost(d_host_address,d_host_port,"",false);
}


void MatrixGvc7000::watchdogPollData()
{
  d_socket->write("*0SS");
}


void MatrixGvc7000::watchdogTimeoutData()
{
  d_socket->close();
}
