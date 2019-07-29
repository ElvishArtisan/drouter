// exitnotifier.h
//
//  Emit a Qt signal upon reception of SIGINT or SIGTERM
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

#ifndef EXITNOTIFIER_H
#define EXITNOTIFIER_H

#include <QObject>
#include <QSocketNotifier>

class ExitNotifier : public QObject
{
  Q_OBJECT
 public:
  ExitNotifier(QObject *parent=0);
  ~ExitNotifier();

 signals:
  void aboutToExit();

 private slots:
  void activatedData(int sock);

 private:
  QSocketNotifier *exit_notifier;
};


#endif  // EXITNOTIFIER_H
