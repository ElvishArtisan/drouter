// endpointmap.h
//
// Map integers to DRouter endpoints.
//
// (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ENDPOINTMAP_H
#define ENDPOINTMAP_H

#include <stdio.h>

#include <QHostAddress>
#include <QList>
#include <QString>
#include <QStringList>

#define ENDPOINTMAP_MAP_DIRECTORY "/etc/drouter.d/maps"
#define ENDPOINTMAP_MAP_FILTER QString("*.map")

class Snapshot
{
 public:
  Snapshot(const QString &name);
  QString name() const;
  void setName(const QString &str);
  int routeQuantity() const;
  int routeInput(int n) const;
  int routeOutput(int n) const;
  void addRoute(int output,int input);

 private:
  QString snap_name;
  QList<int> snap_inputs;
  QList<int> snap_outputs;
};




class EndPointMap
{
 public:
  enum RouterType {AudioRouter=0,GpioRouter=1,LastRouter=2};
  enum Type {Input=0,Output=1,LastType=3};
  EndPointMap();
  RouterType routerType() const;
  void setRouterType(RouterType type);
  QString routerName() const;
  void setRouterName(const QString &str);
  int routerNumber() const;
  void setRouterNumber(int num);
  int quantity(Type type) const;
  QHostAddress hostAddress(Type type,int n) const;
  void setHostAddress(Type type,int n,const QHostAddress &addr);
  void setHostAddress(Type type,int n,const QString &addr);
  int slot(Type type,int n) const;
  void setSlot(Type type,int n,int slot);
  QString name(Type type,int n,const QString &orig_name=QString()) const;
  void setName(Type type,int n,const QString &str);
  int endPoint(Type type,const QHostAddress &hostaddr,int slot) const;
  int endPoint(Type type,const QString &hostaddr,int slot) const;
  void insert(Type type,int n,const QHostAddress &host_addr,int slot);
  void insert(Type type,int n,const QString &host_addr,int slot);
  void erase(Type type,int n);
  int snapshotQuantity() const;
  Snapshot *snapshot(int n) const;
  Snapshot *snapshot(const QString &name);
  bool load(const QString &filename,QStringList *unused_lines=NULL);
  bool save(const QString &filename) const;
  void save(FILE *f) const;
  static bool loadSet(QMap<int,EndPointMap *> *maps,QStringList *msgs);
  static QString routerTypeString(RouterType type);
  static QString typeString(Type type);

 private:
  QString map_router_name;
  int map_router_number;
  RouterType map_router_type;
  QList<QHostAddress> map_host_addresses[EndPointMap::LastType];
  QList<int> map_slots[EndPointMap::LastType];
  QStringList map_names[EndPointMap::LastType];
  QList<Snapshot *> map_snapshots;
};


#endif  // ENDPOINTMAP_H
