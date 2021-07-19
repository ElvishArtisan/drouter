// dparser.h
//
// Parser for Protocol D
//
// (C) Copyright 2017-2021 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DPARSER_H
#define DPARSER_H

#include <stdint.h>

#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include <sy5/sydestination.h>
#include <sy5/synode.h>
#include <sy5/sysource.h>

#define DPARSER_WATCHDOG_POLL_INTERVAL 1000
#define DPARSER_WATCHDOG_TIMEOUT_INTERVAL 3000

class DParser : public QObject
{
  Q_OBJECT;
 public:
  DParser(QObject *parent=0);
  QList<QHostAddress> nodeHostAddresses() const;
  SyNode *node(const QHostAddress &hostaddr);
  SySource *src(const QHostAddress &hostaddr,int slot) const;
  SyDestination *dst(const QHostAddress &hostaddr,int slot) const;
  void connectToHost(const QString &hostname,uint16_t port);

 signals:
  void connected(bool state);
  void error(QAbstractSocket::SocketError err,const QString &err_msg);
  void destinationChanged(const QHostAddress &host_addr,int slot,
			  SyDestination *dst);
  void destinationAdded(const QHostAddress &host_addr,int slot);
  void destinationRemoved(const QHostAddress &host_addr,int slot);
  void nodeAdded(const QHostAddress &node_addr);
  void nodeRemoved(const QHostAddress &node_addr);
  void sourceChanged(const QHostAddress &host_addr,int slot,SySource *dst);
  void sourceAdded(const QHostAddress &host_addr,int slot);
  void sourceRemoved(const QHostAddress &host_addr,int slot);
  void crosspointChanged(const QHostAddress &dst_host_addr,int dst_slot,
			 const QHostAddress &src_host_addr,int src_slot);
  void crosspointCleared(const QHostAddress &dst_host_addr,int dst_slot);

 private slots:
  void connectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);
  void pollTimerData();
  void watchdogTimerData();

 private:
  void ProcessCommand(const QString &cmd);
  void SendCommand(const QString &cmd);
  uint64_t IndexByStreamAddress(const QHostAddress &saddr) const;
  uint64_t ToIndex(const QHostAddress &addr,int slot) const;
  QHostAddress ToAddress(uint64_t index) const;
  int ToSlot(uint64_t index) const;
  QString d_hostname;
  uint16_t d_port;
  QTcpSocket *d_socket;
  QMap<unsigned,SyNode *> d_nodes;
  QMap<uint64_t,SyDestination *> d_destinations;
  QMap<uint64_t,SySource *> d_sources;
  QString d_accum;
  bool d_connected;
  QTimer *d_poll_timer;
  QTimer *d_watchdog_timer;
};


#endif  // DPARSER_H
