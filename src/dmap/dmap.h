// dmap.h
//
// dmap(8) map utility
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

#ifndef DMAP_H
#define DMAP_H

#include <stdint.h>

#include <QList>
#include <QObject>
#include <QTimer>

#include <sy/symcastsocket.h>
#include <sy/sylwrp_client.h>

#include "endpointmap.h"

#define DMAP_USAGE "[options]\n"
#define DMAP_DEFAULT_SCAN_DURATION 25000
#define DMAP_CONNECTION_RETRY_LIMIT 2

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);
  void startNodeProcessing();

 private slots:
  void advtReadyReadData(int ifnum);
  void scanTimeoutData();
  void garbageTimeoutData();
  void connectionTimeoutData();
  void nodeConnectedData(unsigned id,bool state);
  void nodeErrorData(unsigned id,QAbstractSocket::SocketError err);

 private:
  int ProcessUseNodeList(const QString &filename);
  int ProcessSkipNodeList(const QString &filename);
  void PurgeSkippedNodes();
  void DumpNodeList();
  void SortAddresses(QList<uint32_t> &addrs);
  //  void LoadMatrix(int mtxnum);
  void Verbose(const QString &msg);
  bool map_verbose;
  //  QHostAddress map_interface_address;
  QString map_output_map;
  QString map_node_password;
  bool map_no_off_source;
  bool map_scan_only;
  int map_scan_duration;
  QList<SyMcastSocket *> map_advert_sockets;
  QTimer *map_scan_timer;
  QTimer *map_garbage_timer;
  QTimer *map_connection_timer;
  SyLwrpClient *map_lwrp;
  QList<uint32_t> map_node_addresses;
  QList<uint32_t> map_skip_node_addresses;
  EndPointMap *map_map;
  int map_router_number;
  QString map_router_name;
  int map_max_nodes;
  unsigned map_current_id;
  unsigned map_retry_count;
};


#endif  // DMAP_H
