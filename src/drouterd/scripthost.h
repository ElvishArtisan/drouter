// scripthost.h
//
// Run a state script.
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

#ifndef SCRIPTHOST_H
#define SCRIPTHOST_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#define SCRIPTHOST_RESTART_INTERVAL 1000

class ScriptHost : public QObject
{
 Q_OBJECT;
 public:
  ScriptHost(const QString &exec,QObject *parent=0);
  ~ScriptHost();

 public slots:
  void start();
  void terminate();
  void kill();

 private slots:
  void finishedData(int exit_code,QProcess::ExitStatus status);
  void errorData(QProcess::ProcessError err);

 private:
  QProcess *script_process;
  QStringList script_arguments;
  QTimer *script_restart_timer;
  bool script_restart;
};


#endif  // SCRIPTHOST_H
