// drouterd.h
//
// Dynamic router service for Livewire networks
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DROUTERD_H
#define DROUTERD_H

#include <QList>
#include <QObject>
#include <QTimer>

#include <sy5/sysignalnotifier.h>

#include "config.h"
#include "drouter.h"
#include "routeengine.h"
#include "scriptengine.h"
#include "tether.h"

#define DROUTERD_PROTOCOL_START_INTERVAL 30000
#define DROUTERD_USAGE "[--no-scripts]\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void protocolData();
  void scriptsData();
  void instanceStateChangedData(bool this_state);
  void exitData(int signum);

 private:
  DRouter *main_drouter;
  RouteEngine *main_route_engine;
  QTimer *main_protocol_timer;
  QList<pid_t> main_protocol_pids;
  int main_protocol_socks[3];
  bool main_no_protocols;
  bool main_no_scripts;
  bool main_no_tether;
  QTimer *main_scripts_timer;
  ScriptEngine *main_script_engine;
  SySignalNotifier *main_exit_notifier;
  Tether *main_tether;
  Config *main_config;
};


#endif  // DROUTERD_H
