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

#include "endpointmap.h"
#include "jparser.h"
#include "protocol.h"
#include "sqlquery.h"

class ProtocolJ : public Protocol
{
 Q_OBJECT;
 public:
  ProtocolJ(int sock,QObject *parent=0);

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
  void ActivateRoute(unsigned router,unsigned output,unsigned input);
  void TriggerGpi(unsigned router,unsigned input,unsigned msecs,const QString &code);
  void TriggerGpo(unsigned router,unsigned output,unsigned msecs,const QString &code);
  void SendSnapshotNames(unsigned router);
  void SendSnapshotRoutes(unsigned router,const QString &snap_name);
  void ActivateSnapshot(unsigned router,const QString &snapshot_name);
  void SendSourceInfo(unsigned router);
  QString SourceNamesSqlFields(EndPointMap::RouterType type) const;
  QString SourceNamesMessage(EndPointMap::RouterType type,SqlQuery *q,
			     int padding=0,bool final=false);
  void SendDestInfo(unsigned router);
  QString DestNamesSqlFields(EndPointMap::RouterType type) const;
  QString DestNamesMessage(EndPointMap::RouterType type,SqlQuery *q,
			   int padding=0,bool final=false);
  void SendGpiInfo(unsigned router,int input);
  QString GPIStatSqlFields() const;
  QString GPIStatMessage(SqlQuery *q);
  void SendGpoInfo(unsigned router,int output);
  QString GPOStatSqlFields() const;
  QString GPOStatMessage(SqlQuery *q);
  void SendRouteInfo(unsigned router,int output);
  QString RouteStatSqlFields(EndPointMap::RouterType type);
  QString RouteStatMessage(int router,int output,int input);
  QString RouteStatMessage(SqlQuery *q);
  void DrouterMaskGpiStat(bool state);
  void DrouterMaskGpoStat(bool state);
  void DrouterMaskRouteStat(bool state);
  void DrouterMaskStat(bool state);
  void SendPingResponse();
  void ProcessCommand(const QString &cmd);
  void LoadMaps();
  void LoadHelp();
  void AddRouteEvent(int router,int output,int input);
  void AddSnapEvent(int router,const QString &name);

  void SendError(JParser::ErrorType etype,const QString &remarks=QString());

  QString JsonPadding(int padding);
  QString JsonEscape(const QString &str);
  QString JsonNullField(const QString &name,int padding=0,
			bool final=false);
  QString JsonField(const QString &name,bool value,int padding=0,
		    bool final=false);
  QString JsonField(const QString &name,int value,int padding=0,
		    bool final=false);
  QString JsonField(const QString &name,unsigned value,int padding=0,
		    bool final=false);
  QString JsonField(const QString &name,const QString &value,
		    int padding=0,bool final=false);
  QString JsonField(const QString &name,const QDateTime &value,
		    int padding=0,bool final=false);
  QString JsonCloseBlock(bool final);

  //
  // XML xs:date format
  //
  QDate ParseXmlDate(const QString &str,bool *ok);
  QString WriteXmlDate(const QDate &date);

  //
  // XML xs:time format
  //
  QTime ParseXmlTime(const QString &str,bool *ok,int *day_offset=NULL);
  QString WriteXmlTime(const QTime &time);

  //
  // XML xs:dateTime format
  //
  QDateTime ParseXmlDateTime(const QString &str,bool *ok);
  QString WriteXmlDateTime(const QDateTime &dt);

  int TimeZoneOffset();

  QMap<QString,QString> proto_help_strings;
  QTcpSocket *proto_socket;
  QTcpServer *proto_server;
  QByteArray proto_accum;
  int proto_accum_level;
  QMap<int,EndPointMap *> proto_maps;
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
