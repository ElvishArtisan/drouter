// matrix-bt-41mlr.cpp
//
// Router matrix implementation for Broadcast Tools Universal 4.1 MLR>>Web
//
// (C) 2023-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include "matrix_bt-41mlr.h"

MatrixBt41Mlr::MatrixBt41Mlr(unsigned id,Config *conf,QObject *parent)
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


MatrixBt41Mlr::~MatrixBt41Mlr()
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


bool MatrixBt41Mlr::isConnected() const
{
  return d_connected;
  //  return d_socket->state()==QAbstractSocket::ConnectedState;
}


QHostAddress MatrixBt41Mlr::hostAddress() const
{
  return d_host_address;
}


QString MatrixBt41Mlr::hostName() const
{
  return d_host_address.toString();
}


QString MatrixBt41Mlr::deviceName() const
{
  return Config::matrixTypeString(matrixType());
}


QString MatrixBt41Mlr::description() const
{
  return QString("Broadcast Tools Universal 4.1 MLR>>Web");
}


unsigned MatrixBt41Mlr::dstSlots() const
{
  return MATRIX_BT41MLR_DEST_QUAN;
}


unsigned MatrixBt41Mlr::srcSlots() const
{
  return MATRIX_BT41MLR_SOURCE_QUAN;
}


SySource *MatrixBt41Mlr::src(int slot) const
{
  return d_sources[slot];
}


SyDestination *MatrixBt41Mlr::dst(int slot) const
{
  return d_destinations[slot];
}


int MatrixBt41Mlr::srcNumber(int slot) const
{
  return slot+1;
}


QHostAddress MatrixBt41Mlr::srcAddress(int slot) const
{
  return d_sources[slot]->streamAddress();
}


QString MatrixBt41Mlr::srcName(int slot) const
{
  return d_sources[slot]->name();
}


bool MatrixBt41Mlr::srcEnabled(int slot) const
{
  return true;
}


unsigned MatrixBt41Mlr::srcChannels(int slot) const
{
  return 2;
}


QHostAddress MatrixBt41Mlr::dstAddress(int slot) const
{
  return d_destinations[slot]->streamAddress();
}


void MatrixBt41Mlr::setDstAddress(int slot,const QHostAddress &s_addr)
{
  if(d_destinations[slot]->streamAddress()!=s_addr) {
    d_socket->
      write(QString::asprintf("*0%02d",0xff&s_addr.toIPv4Address()).toUtf8());
  }
}


void MatrixBt41Mlr::setDstAddress(int slot,const QString &s_addr)
{
  setDstAddress(slot,QHostAddress(s_addr));
}


QString MatrixBt41Mlr::dstName(int slot) const
{
  return d_destinations[slot]->name();
}


unsigned MatrixBt41Mlr::dstChannels(int slot) const
{
  return 2;
}


unsigned MatrixBt41Mlr::gpis() const
{
  return MATRIX_BT41MLR_GPI_QUAN;
}


SyGpioBundle *MatrixBt41Mlr::gpiBundle(int slot) const
{
  return d_gpio_bundles[slot];
}


bool MatrixBt41Mlr::silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				       int chan) const
{
  if(type==SyLwrpClient::OutputMeter) {
    return d_silence_alarms[slot];
  }
  return false;
}


void MatrixBt41Mlr::connectToHost(const QHostAddress &addr,uint16_t port,
				  const QString &pwd,bool persistent)
{
  d_host_address=addr;
  d_host_port=port;

  d_socket->connectToHost(d_host_address,d_host_port);
  d_watchdog->start();
}


void MatrixBt41Mlr::connectedData()
{
  d_node.setHostAddress(d_host_address);
  d_node.setDeviceName(Config::matrixTypeString(Config::Bt41MlrMatrix));
  d_node.setProductName("Broadcast Tools Universal 4.1 MLR>>Web");
  d_node.setSrcSlotQuantity(MATRIX_BT41MLR_SOURCE_QUAN);
  d_node.setDstSlotQuantity(MATRIX_BT41MLR_DEST_QUAN);

  d_socket->write("*0SL");   // Request audio crosspoint state
}


void MatrixBt41Mlr::disconnectedData()
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


void MatrixBt41Mlr::readyReadData()
{
  bool found=false;
  QByteArray data=d_socket->readAll();
  QStringList f0=QString::fromUtf8(data).trimmed().
    split(",",Qt::KeepEmptyParts);
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


void MatrixBt41Mlr::reconnectData()
{
  connectToHost(d_host_address,d_host_port,"",false);
}


void MatrixBt41Mlr::watchdogPollData()
{
  d_socket->write("*0SS");
}


void MatrixBt41Mlr::watchdogTimeoutData()
{
  d_socket->close();
}
