// drouterd.h
//
// Dynamic router service for Livewire networks
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

#ifndef DROUTERD_H
#define DROUTERD_H

#include <QObject>
#include <QTimer>

#include "drouter.h"

#define DROUTERD_SCRIPTS_DIRECTORY QString("/etc/drouter.d/scripts")
#define DROUTERD_SCRIPTS_FILTER QString("*.py")
#define DROUTERD_USAGE "[--no-scripts]\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void signalData();

 private:
  DRouter *main_drouter;
  QTimer *main_signal_timer;
};


#endif  // DROUTERD_H
