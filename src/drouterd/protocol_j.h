// protocol_j.h
//
// Protocol J protocol handler for DRouter.
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef PROTOCOL_J_H
#define PROTOCOL_J_H

#include <signal.h>

#include <QHostInfo>
#include <QTcpServer>

#include <sy5/sylwrp_client.h>

#include <drendpointmap.h>
#include <drjparser.h>
#include <drsqlquery.h>

#include "protocol.h"

class ProtocolJ : public Protocol
{
 Q_OBJECT;
 public:
  ProtocolJ(int sock,QObject *parent=0);

 signals:
  void parserError(DRJParser::ErrorType err,const QString &remarks);

 private slots:
  void newConnectionData();
  void readyReadData();
  void disconnectedData();
  void snapshotHostLookupFinishedData(const QHostInfo &info);
  void routeHostLookupFinishedData(const QHostInfo &info);

 protected:
  void destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum);
  void gpiCodeChanged(const QHostAddress &host_addr,int slotnum);
  void gpoCodeChanged(const QHostAddress &host_addr,int slotnum);
  void gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum);
  void quitting();

 private:
  void DispatchMessage(const QJsonDocument &jdoc);
  void ActivateRoute(unsigned router,unsigned output,unsigned input);
  void TriggerGpi(unsigned router,unsigned input,unsigned msecs,
		  const QString &code);
  void TriggerGpo(unsigned router,unsigned output,unsigned msecs,
		  const QString &code);
  void SendSnapshotNames(unsigned router);
  void SendSnapshotRoutes(unsigned router,const QString &snap_name);
  void ActivateSnapshot(unsigned router,const QString &snapshot_name);
  void SendSourceInfo(unsigned router);
  QString SourceNamesSqlFields(DREndPointMap::RouterType type) const;
  QString SourceNamesMessage(DREndPointMap::RouterType type,DRSqlQuery *q,
			     int padding=0,bool final=false);
  void SendDestInfo(unsigned router);
  QString DestNamesSqlFields(DREndPointMap::RouterType type) const;
  QString DestNamesMessage(DREndPointMap::RouterType type,DRSqlQuery *q,
			   int padding=0,bool final=false);
  void SendActionInfo(unsigned router);
  QString ActionListSqlFields() const;
  QString ActionListMessage(DRSqlQuery *q,int padding=0,bool final=false);
  void SendGpiInfo(unsigned router,int input);
  QString GPIStatSqlFields() const;
  QString GPIStatMessage(DRSqlQuery *q);
  void SendGpoInfo(unsigned router,int output);
  QString GPOStatSqlFields() const;
  QString GPOStatMessage(DRSqlQuery *q);
  void SendRouteInfo(unsigned router,int output);
  QString RouteStatSqlFields(DREndPointMap::RouterType type);
  QString RouteStatMessage(int router,int output,int input);
  QString RouteStatMessage(DRSqlQuery *q);
  void MaskGpiStat(bool state);
  void MaskGpoStat(bool state);
  void MaskRouteStat(bool state);
  void MaskStat(bool state);
  void HelpMessage(const QString &keyword);
  void SendPingResponse();
  void LoadMaps();
  void LoadHelp();
  void AddRouteEvent(int router,int output,int input);
  void AddSnapEvent(int router,const QString &name);
  void SendError(DRJParser::ErrorType etype,const QString &remarks=QString());
  QMap<QString,QString> proto_help_patterns;
  QMap<QString,QString> proto_help_comments;
  QTcpSocket *proto_socket;
  QTcpServer *proto_server;
  QByteArray proto_accum;
  bool proto_accum_quoted;
  int proto_accum_level;
  QMap<int,DREndPointMap *> proto_maps;
  QMap <int,int> proto_event_lookups;
  QString proto_username;
  QString proto_hostname;
  bool proto_destinations_subscribed;
  bool proto_gpis_subscribed;
  bool proto_gpos_subscribed;
  bool proto_nodes_subscribed;
  bool proto_sources_subscribed;
  bool proto_clips_subscribed;
  bool proto_silences_subscribed;
  bool proto_gpistat_masked;
  bool proto_gpostat_masked;
  bool proto_routestat_masked;
};


#endif  // PROTOCOL_J_H
