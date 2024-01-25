// drendpointmap.h
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

#ifndef DRENDPOINTMAP_H
#define DRENDPOINTMAP_H

#include <stdio.h>

#include <QHostAddress>
#include <QList>
#include <QString>
#include <QStringList>

#define DRENDPOINTMAP_MAP_DIRECTORY "/etc/drouter/maps.d"
#define DRENDPOINTMAP_MAP_FILTER QString("*.map")

class DRSnapshot
{
 public:
  DRSnapshot(const QString &name);
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




class DREndPointMap
{
 public:
  enum RouterType {AudioRouter=0,GpioRouter=1,LastRouter=2};
  enum Type {Input=0,Output=1,LastType=3};
  DREndPointMap();
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
  bool nameIsCustom(Type type,int n) const;
  void setName(Type type,int n,const QString &str);
  int endPoint(Type type,const QHostAddress &hostaddr,int slot) const;
  int endPoint(Type type,const QString &hostaddr,int slot) const;
  void insert(Type type,int n,const QHostAddress &host_addr,int slot,
	      const QString &name=QString());
  void insert(Type type,int n,const QString &host_addr,int slot,
	      const QString &name=QString());
  void erase(Type type,int n);
  int snapshotQuantity() const;
  DRSnapshot *snapshot(int n) const;
  DRSnapshot *snapshot(const QString &name);
  bool load(const QString &filename,QStringList *unused_lines=NULL);
  bool save(const QString &filename,bool incl_names) const;
  void save(FILE *f,bool incl_names) const;
  static bool loadSet(QMap<int,DREndPointMap *> *maps,QStringList *msgs);
  static QString routerTypeString(RouterType type);
  static QString typeString(Type type);

 private:
  QString map_router_name;
  int map_router_number;
  RouterType map_router_type;
  QList<QHostAddress> map_host_addresses[DREndPointMap::LastType];
  QList<int> map_slots[DREndPointMap::LastType];
  QStringList map_names[DREndPointMap::LastType];
  QList<bool> map_name_is_customs[DREndPointMap::LastType];
  QList<DRSnapshot *> map_snapshots;
};


#endif  // DRENDPOINTMAP_H
