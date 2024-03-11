// routeengine.h
//
// Run Route Actions
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ROUTEENGINE_H
#define ROUTEENGINE_H

#include <QMap>
#include <QObject>

#include <drouter/drsqlquery.h>

#include "timeengine.h"

class RouteAction
{
 public:
  RouteAction(DRSqlQuery *q);
  int id() const;
  QTime time() const;
  bool dayOfWeek(int dow) const;
  int routerNumber() const;
  int destinationNumber() const;
  int sourceNumber() const;
  QString comment() const;
  void update(DRSqlQuery *q);
  static QString sqlFields();

 private:
  int d_id;
  QTime d_time;
  bool d_day_of_week[7];
  int d_router_number;
  int d_destination_number;
  int d_source_number;
  QString d_comment;
};




class RouteEngine : public QObject
{
 Q_OBJECT;
 public:
  RouteEngine(QObject *parent=0);
  bool load();

 public slots:
  void refresh(int action_id);

 signals:
  void setCrosspoint(int router,int output,int input);

 private slots:
  void actionData(int action_id);

 private:
  QMap<int,RouteAction *> d_route_actions;
  TimeEngine *d_time_engine;
};


#endif  // ROUTEENGINE_H
