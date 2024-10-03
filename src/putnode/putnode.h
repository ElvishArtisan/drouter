// putnode.h
//
// putnode() LWRP restore utility
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

#ifndef PUTNODE_H
#define PUTNODE_H

#include <stdio.h>

#include <QTcpSocket>
#include <QTextStream>

#define PUTNODE_USAGE "[--hostname=<ip-addr>[:<port-num>]] [--password=<str>]\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void connectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);
  
 private:
  void SendCommand(const QString &msg);
  QTcpSocket *d_socket;
  QByteArray d_accum;
  int d_end_count;
  QString d_password;
  FILE *d_file;
};


#endif  // PUTNODE_H
