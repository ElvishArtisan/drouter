// scriptengine.h
//
// Run state scripts.
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QMap>
#include <QObject>
#include <QTimer>

#define SCRIPTENGINE_RESTART_INTERVAL 1000
#define SCRIPTENGINE_SCRIPTS_DIRECTORY QString("/etc/drouter.d/scripts")
#define SCRIPTENGINE_SCRIPTS_FILTER QString("*.py")

class ScriptEngine : public QObject
{
 Q_OBJECT;
 public:
  ScriptEngine(QObject *parent=0);
  void start();
  void stop();

 private slots:
  void scanData();

 private:
  QString Pathname(const QString &script) const;
  QMap<pid_t,QString> script_scripts;
  QTimer *script_scan_timer;
};


#endif  // SCRIPTENGINE_H
