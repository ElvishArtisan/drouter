// matrix-bt-41mlr.h
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

#ifndef MATRIX_BT_41MLR_H
#define MATRIX_BT_41MLR_H

#include <QTcpSocket>
#include <QTimer>

#include <sy5/sydestination.h>
#include <sy5/synode.h>
#include <sy5/sysource.h>

#include "matrix.h"
#include "watchdog.h"

#define MATRIX_BT41MLR_SOURCE_QUAN 4
#define MATRIX_BT41MLR_DEST_QUAN 1
#define MATRIX_BT41MLR_GPI_QUAN 1
#define MATRIX_BT41MLR_GPO_QUAN 0
#define MATRIX_BT41MLR_STREAM_ADDR_PREFIX "0.0.0"

class MatrixBt41Mlr :public Matrix
{
  Q_OBJECT;
 public:
  MatrixBt41Mlr(unsigned id,Config *conf,QObject *parent=0);
  ~MatrixBt41Mlr();
  bool isConnected() const;
  QHostAddress hostAddress() const;
  QString hostName() const;
  QString deviceName() const;
  unsigned dstSlots() const;
  unsigned srcSlots() const;
  SySource *src(int slot) const;
  SyDestination *dst(int slot) const;
  int srcNumber(int slot) const;
  QHostAddress srcAddress(int slot) const;
  QString srcName(int slot) const;
  bool srcEnabled(int slot) const;
  unsigned srcChannels(int slot) const;
  QHostAddress dstAddress(int slot) const;
  void setDstAddress(int slot,const QHostAddress &s_addr);
  void setDstAddress(int slot,const QString &s_addr);
  QString dstName(int slot) const;
  unsigned dstChannels(int slot) const;
  unsigned gpis() const;
  SyGpioBundle *gpiBundle(int slot) const;
  bool silenceAlarmActive(int slot,SyLwrpClient::MeterType type,int chan) const;
  void connectToHost(const QHostAddress &addr,uint16_t port,const QString &pwd,
		     bool persistent=false);

 private slots:
  void connectedData();
  void disconnectedData();
  void readyReadData();
  void reconnectData();
  void watchdogPollData();
  void watchdogTimeoutData();

 private:
  QTcpSocket *d_socket;
  QHostAddress d_host_address;
  uint16_t d_host_port;
  SySource *d_sources[MATRIX_BT41MLR_SOURCE_QUAN];
  SyDestination *d_destinations[MATRIX_BT41MLR_DEST_QUAN];
  bool d_silence_alarms[MATRIX_BT41MLR_DEST_QUAN];
  SyGpioBundle *d_gpio_bundles[MATRIX_BT41MLR_GPI_QUAN];
  SyNode d_node;
  QTimer *d_reconnect_timer;
  bool d_connected;
  Watchdog *d_watchdog;
};


#endif  // MATRIX_BT_41MLR_H
