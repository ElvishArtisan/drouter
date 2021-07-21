// protocol_d.h
//
// Protocol D handler for DRouter.
//
//   (C) Copyright 2018-2021 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef PROTOCOL_D_H
#define PROTOCOL_D_H

#include <signal.h>

#include <QTcpServer>

#include <sy5/sylwrp_client.h>

#include "protocol.h"
#include "sqlquery.h"

class ProtocolD : public Protocol
{
 Q_OBJECT;
 public:
 ProtocolD(int sock,QObject *parent=0);

 private slots:
  void newConnectionData();
  void readyReadData();
  void disconnectedData();

 protected:
  void tetherStateUpdated(bool state);
  void nodeAdded(const QHostAddress &host_addr);
  void nodeRemoved(const QHostAddress &host_addr,
		   int srcs,int dsts,int gpis,int gpos);
  void nodeChanged(const QHostAddress &host_addr);
  void sourceChanged(const QHostAddress &host_addr,int slotnum);
  void destinationChanged(const QHostAddress &host_addr,int slotnum);
  void gpiChanged(const QHostAddress &host_addr,int slotnum);
  void gpoChanged(const QHostAddress &host_addr,int slotnum);
  void clipChanged(const QHostAddress &host_addr,int slotnum,
		   SyLwrpClient::MeterType meter_type,
		   const QString &tbl_name,int chan);
  void silenceChanged(const QHostAddress &host_addr,int slotnum,
		      SyLwrpClient::MeterType meter_type,
		      const QString &tbl_name,int chan);

 private:
  void ProcessCommand(const QString &cmd);
  QString AlarmSqlFields(const QString &type,int chan) const;
  QString AlarmRecord(const QString &keyword,SyLwrpClient::MeterType port,
		      int chan,SqlQuery *q);
  QString DestinationSqlFields() const;
  QString DestinationRecord(const QString &keyword,SqlQuery *q) const;
  QString GpiSqlFields() const;
  QString GpiRecord(const QString &keyword,SqlQuery *q);
  QString GpoSqlFields() const;
  QString GpoRecord(const QString &keyword,SqlQuery *q);
  QString NodeSqlFields() const;
  QString NodeRecord(const QString &keyword,SqlQuery *q) const;
  QString SourceSqlFields() const;
  QString SourceRecord(const QString &keyword,SqlQuery *q);
  QTcpSocket *proto_socket;
  QTcpServer *proto_server;
  QString proto_accum;
  bool proto_tether_subscribed;
  bool proto_destinations_subscribed;
  bool proto_gpis_subscribed;
  bool proto_gpos_subscribed;
  bool proto_nodes_subscribed;
  bool proto_sources_subscribed;
  bool proto_clips_subscribed;
  bool proto_silences_subscribed;
};


#endif  // PROTOCOL_D_H
