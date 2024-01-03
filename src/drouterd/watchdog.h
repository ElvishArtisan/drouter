// watchdog.h
//
// Abstract watchdog
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

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <QObject>
#include <QTimer>

#define WATCHDOG_DEFAULT_POLL_INTERVAL 1000
#define WATCHDOG_DEFAULT_TIMEOUT_INTERVAL 3000

class Watchdog : public QObject
{
 Q_OBJECT;
 public:
  Watchdog(QObject *parent=0);
  ~Watchdog();
  int pollInterval() const;
  void setPollInterval(int msecs);
  int timeoutInterval() const;
  void setTimeoutInterval(int msecs);
  bool isActive();

 public slots:
  void start();
  void stop();
  void touch();

 signals:
  void poll();
  void timeout();

 private slots:
  void pollData();
  void timeoutData();

 private:
  int d_poll_interval;
  QTimer *d_poll_timer;
  int d_timeout_interval;
  QTimer *d_timeout_timer;
};


#endif  // WATCHDOG_H
