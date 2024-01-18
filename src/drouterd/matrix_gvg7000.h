// matrix_gvg7000.h
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

#ifndef MATRIX_GVG7000_H
#define MATRIX_GVG7000_H

#include <QMap>
#include <QTcpSocket>
#include <QTimer>

#include <sy5/sydestination.h>
#include <sy5/synode.h>
#include <sy5/sysource.h>

#include "matrix.h"
#include "watchdog.h"

class MatrixGvg7000 :public Matrix
{
  Q_OBJECT;
 public:
  MatrixGvg7000(unsigned id,Config *conf,QObject *parent=0);
  ~MatrixGvg7000();
  bool isConnected() const;
  QHostAddress hostAddress() const;
  QString hostName() const;
  QString deviceName() const;
  QString description() const;
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
  void connectToHost(const QHostAddress &addr,uint16_t port,const QString &pwd,
		     bool persistent=false);

 private slots:
  void connectedData();
  void disconnectedData();
  void readyReadData();
  void reconnectData();
  void pollData();
  void watchdogPollData();
  void watchdogTimeoutData();

 private:
  void DispatchGvgCommand(const QByteArray &msg);
  void ProcessGvgCommand(const QByteArray &msg);
  void SendGvgCommand(const QString &str);
  QByteArray ToGvgNative(QString str) const;
  bool GvgVerifyChecksum(const QByteArray &msg) const;
  uint8_t GvgChecksum(const QByteArray &msg) const;
  QString GvgPrettify(const QByteArray &msg) const;
  QTcpSocket *d_socket;
  QHostAddress d_host_address;
  uint16_t d_host_port;
  QMap<int,SySource *> d_sources;
  QMap<int,SyDestination *> d_destinations;
  SyNode d_node;
  QTimer *d_reconnect_timer;
  bool d_connected;
  Watchdog *d_watchdog;
  QByteArray d_raw_accum;
  QByteArray d_layer4_accum;
  QTimer *d_poll_timer;
};


#endif  // MATRIX_GVG7000_H
