// dparsertest.h
//
// Test utility for DParser
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DPARSERTEST_H
#define DPARSERTEST_H

#include <stdint.h>

#include <QList>
#include <QObject>
#include <QTimer>

#include <sy/symcastsocket.h>
#include <sy/sylwrp_client.h>

#include "dparser.h"
#include "endpointmap.h"

#define DPARSERTEST_USAGE "--hostname=<host-name>\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void connectedData(bool state);
  void errorData(QAbstractSocket::SocketError err,const QString &err_msg);
  void destinationChangedData(const QHostAddress &addr,int slot,
			      SyDestination *dst);
  void destinationAddedData(const QHostAddress &addr,int slot);
  void destinationRemovedData(const QHostAddress &addr,int slot);
  void nodeAddedData(const QHostAddress &addr);
  void nodeRemovedData(const QHostAddress &addr);
  void sourceChangedData(const QHostAddress &addr,int slot,SySource *src);
  void sourceAddedData(const QHostAddress &addr,int slot);
  void sourceRemovedData(const QHostAddress &addr,int slot);
  void crosspointChangedData(const QHostAddress &daddr,int dslot,
			     const QHostAddress &saddr,int sslot);
  void crosspointClearedData(const QHostAddress &daddr,int dslot);
 
 private:
  DParser *test_parser;
  QString test_hostname;
};


#endif  // DPARSERTEST_H
