// logger_back.h
//
// Methods for finalizing provisional event log entries in drouterd(8)
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

#ifndef LOGGER_BACK_H
#define LOGGER_BACK_H

#include <QHostInfo>
#include <QMap>
#include <QObject>
#include <QTimer>

#include <drouter/drendpointmap.h>

class LoggerBack : public QObject
{
 Q_OBJECT;
 public:
  LoggerBack(QMap<int,DREndPointMap *> *router_maps,QObject *parent=0);
  ~LoggerBack();
  void setWriteable(bool state);

 signals:
  void eventAdded(int evt_id);

 private slots:
  void finalizeEventsData();

 private:
  void FinalizeSAAudioRoute(int event_id,int router,int output,int input);
  void FinalizeSAGpioRoute(int event_id,int router,int output,int input);
  void FinalizeSARouteEvent(int event_id,bool status);
  QMap<int,DREndPointMap *> *d_router_maps;
  QTimer *d_finalize_timer;
};


#endif  // LOGGER_BACK_H
