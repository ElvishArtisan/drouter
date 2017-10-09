// server_net.h
//
// Abstract base class for telnetd(8)-like TCP servers.
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <stdint.h>

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QSignalMapper>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

#include <sy/syastring.h>

class NetConnection
{
 public:
  NetConnection(QTcpSocket *sock);
  ~NetConnection();
  QTcpSocket *socket;
  QString buffer;
  bool zombie;
  void *priv;
};


class ServerNet : public QObject
{
 Q_OBJECT;
 public:
 ServerNet(int sock,uint16_t port,QObject *parent=0);
 ServerNet(int sock,uint16_t port,const QHostAddress &m_addr,QObject *parent=0);
  ~ServerNet();
  NetConnection *connection(int id) const;
  QList<NetConnection *> connections() const;

 public slots:
  void setReady();
  void send(const QString &cmd,int id=-1);
  void closeConnection(int id);
  void closeAll();

 protected:
  virtual void newConnection(int id,NetConnection *conn);
  virtual void aboutToCloseConnection(int id,NetConnection *conn);
  virtual void processCommand(int id,const SyAString &cmd)=0;

 private slots:
  void newConnectionData();
  void readyReadData(int id);
  void udpReadyReadData();
  void disconnectedData(int id);
  void garbageData();

 private:
  void Initialize(int sock,uint16_t port);
  unsigned NewConnectionId();
  QList<NetConnection *> net_connections;
  QTcpServer *net_tcp_server;
  QUdpSocket *net_udp_socket;
  QHostAddress net_multicast_address;
  uint16_t net_port;
  QSignalMapper *net_tcp_ready_mapper;
  QSignalMapper *net_tcp_discon_mapper;
  QTimer *net_garbage_timer;
  bool net_ready;
};


#endif  // SERVER_NET_H
