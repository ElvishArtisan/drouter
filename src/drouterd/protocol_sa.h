// protocol_sa.h
//
// Software Authority protocol handler for DRouter.
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

#ifndef PROTOCOL_SA_H
#define PROTOCOL_SA_H

#include <signal.h>

#include <QTcpServer>

#include <sy5/sylwrp_client.h>

#include "endpointmap.h"
#include "protocol.h"
#include "sqlquery.h"

class ProtocolSa : public Protocol
{
 Q_OBJECT;
 public:
  ProtocolSa(int sock,QObject *parent=0);

 private slots:
  void newConnectionData();
  void readyReadData();
  void disconnectedData();

 protected:
  void destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum);
  void gpiCodeChanged(const QHostAddress &host_addr,int slotnum);
  void gpoCodeChanged(const QHostAddress &host_addr,int slotnum);
  void gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum);

 private:
  void ActivateRoute(unsigned router,unsigned output,unsigned input);
  void TriggerGpi(unsigned router,unsigned input,unsigned msecs,const QString &code);
  void TriggerGpo(unsigned router,unsigned output,unsigned msecs,const QString &code);
  void SendSnapshotNames(unsigned router);
  void ActivateSnapshot(unsigned router,const QString &snapshot_name);
  void SendSourceInfo(unsigned router);
  QString SourceNamesSqlFields(EndPointMap::RouterType type) const;
  QString SourceNamesMessage(EndPointMap::RouterType type,SqlQuery *q);
  void SendDestInfo(unsigned router);
  QString DestNamesSqlFields(EndPointMap::RouterType type) const;
  QString DestNamesMessage(EndPointMap::RouterType type,SqlQuery *q);
  void SendGpiInfo(unsigned router,int input);
  QString GPIStatSqlFields() const;
  QString GPIStatMessage(SqlQuery *q);
  void SendGpoInfo(unsigned router,int output);
  QString GPOStatSqlFields() const;
  QString GPOStatMessage(SqlQuery *q);
  void SendRouteInfo(unsigned router,int output);
  QString RouteStatSqlFields(EndPointMap::RouterType type);
  QString RouteStatMessage(SqlQuery *q);
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
