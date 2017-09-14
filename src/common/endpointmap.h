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

#include <QHostAddress>
#include <QList>
#include <QString>

class EndPointMap
{
 public:
  enum Type {Src=0,Dst=1,LastType=2};
  EndPointMap();
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
  int endPoint(Type type,const QHostAddress &hostaddr,int slot) const;
  int endPoint(Type type,const QString &hostaddr,int slot) const;
  void insert(Type type,int n,const QHostAddress &host_addr,int slot);
  void insert(Type type,int n,const QString &host_addr,int slot);
  void erase(Type type,int n);
  bool load(const QString &filename);
  bool save(const QString &filename) const;
  static QString typeString(Type type);

 private:
  QString map_router_name;
  int map_router_number;
  QList<QHostAddress> map_host_addresses[EndPointMap::LastType];
  QList<int> map_slots[EndPointMap::LastType];
};


#endif  // ENDPOINTMAP_H
