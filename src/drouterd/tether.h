// tether.h
//
//  State Manager for twin Drouter instances
//
//   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Library General Public License 
//   version 2 as published by the Free Software Foundation.
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

#ifndef TETHER_H
#define TETHER_H

#include <QHostAddress>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUdpSocket>

#include "drconfig.h"
#include "ttydevice.h"

class Tether : public QObject
{
  Q_OBJECT;
 public:
  Tether(QObject *parent=0);
  bool instanceIsActive() const;
  bool start(DRConfig *config,QString *err_msg);

 public slots:
  void cleanup();

 signals:
  void instanceStateChanged(bool state);

 private slots:
  void udpReadyReadData();
  void ttyReadyReadData();
  void intervalTimeoutData();
  void windowTimeoutData();

 private:
  void Backoff();
  int GetInterval() const;
  bool ModifySharedAddress(const QString &keyword);
  QUdpSocket *tether_udp_socket;
  char tether_udp_state;
  bool tether_udp_replied;
  TTYDevice *tether_tty_device;
  char tether_tty_state;
  bool tether_tty_replied;
  QTimer *tether_interval_timer;
  QTimer *tether_window_timer;
  bool tether_active_state;
  DRConfig *tether_config;
  QStringList tether_exit_args;
};


#endif  // TETHER_H
