// protocol_sa.h
//
// Software Authority protocol handler for DRouter.
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef PROTOCOL_SA_H
#define PROTOCOL_SA_H

#include <signal.h>

#include <QSqlQuery>
#include <QTcpServer>

#include <sy/sylwrp_client.h>

#include "endpointmap.h"
#include "protocol.h"

class ProtocolSa : public Protocol
{
 Q_OBJECT;
 public:
  ProtocolSa(QObject *parent=0);

 private slots:
  void newConnectionData();
  void readyReadData();
  void disconnectedData();

 protected:
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
  void SendSourceInfo(unsigned router);
  QString SourceNamesSqlFields(EndPointMap::RouterType type) const;
  QString SourceNamesMessage(EndPointMap::RouterType type,QSqlQuery *q);
  void SendDestInfo(unsigned router);
  QString DestNamesSqlFields(EndPointMap::RouterType type) const;
  QString DestNamesMessage(EndPointMap::RouterType type,QSqlQuery *q);
  void SendGpiInfo(unsigned router,int input);
  QString GPIStatSqlFields() const;
  QString GPIStatMessage(QSqlQuery *q);
  void SendGpoInfo(unsigned router,int output);
  QString GPOStatSqlFields() const;
  QString GPOStatMessage(QSqlQuery *q);
  void ProcessCommand(const QString &cmd);
  void LoadMaps();
  void LoadHelp();
  QMap<QString,QString> proto_help_strings;
  QTcpSocket *proto_socket;
  QTcpServer *proto_server;
  QString proto_accum;
  QMap<int,EndPointMap *> proto_maps;

  bool proto_destinations_subscribed;
  bool proto_gpis_subscribed;
  bool proto_gpos_subscribed;
  bool proto_nodes_subscribed;
  bool proto_sources_subscribed;
  bool proto_clips_subscribed;
  bool proto_silences_subscribed;
};


#endif  // PROTOCOL_SA_H
