// dmap.h
//
// dmap(8) map utility
//
//   (C) Copyright 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include <sy5/symcastsocket.h>
#include <sy5/sylwrp_client.h>

#include <drouter/drdparser.h>
#include <drouter/drendpointmap.h>

#include "gvg7000mapper.h"

#define DMAP_USAGE "[options]\n"
#define DMAP_DEFAULT_SCAN_DURATION 25000
#define DMAP_CONNECTION_RETRY_LIMIT 2

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  enum DeviceType {DeviceLivewire=0,DeviceGvg7000=1};
  MainObject(QObject *parent=0);

 private slots:
  void connectedData(bool state);
  void errorData(QAbstractSocket::SocketError err,const QString &err_msg);
  void gvg7000ReadCompletedData(bool result,const QString &err_msg);

private:
  void LivewireCheck();
  void LivewireGenerate();
  int ProcessUseNodeList(const QString &filename);
  int ProcessSkipNodeList(const QString &filename);
  void Gvg7000Generate();
  DeviceType map_device_type;
  QString map_device_hostname;
  unsigned map_device_port;
  DRDParser *map_parser;
  QString map_output_map;
  DREndPointMap::RouterType map_router_type;
  DREndPointMap *map_map;
  int map_router_number;
  bool map_save_names;
  QString map_router_name;
  int map_max_nodes;
  bool map_verbose;
  QList<QHostAddress> map_node_addresses;
  QList<QHostAddress> map_skip_node_addresses;

  Gvg7000Mapper *map_gvg7000_mapper;
};


#endif  // DMAP_H
