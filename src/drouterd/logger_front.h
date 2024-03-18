// logger_front.h
//
// Front end for the event logger in drouterd(8)
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

#ifndef LOGGER_FRONT_H
#define LOGGER_FRONT_H

#include <QHostInfo>
#include <QMap>
#include <QObject>

class LoggerFront : public QObject
{
 Q_OBJECT;
 public:
  LoggerFront(QObject *parent=0);
  ~LoggerFront();
  void addRouteEvent(const QHostAddress &addr,const QString &username,
		     int router,int output,int input);
  void addSnapEvent(const QHostAddress &addr,const QString &username,
		    int router,const QString &name);
  void writeCommentEvent(const QString &str);

 signals:
  void eventAdded(int evt_id);

 private slots:
  void routeHostLookupFinishedData(const QHostInfo &info);
  void snapHostLookupFinishedData(const QHostInfo &info);

 private:
  QMap <int,int> d_event_lookups;
};


#endif  // LOGGER_FRONT_H
