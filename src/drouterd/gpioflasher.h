// gpioflasher.h
//
// Component for flashing GPIO devices via LWRP
//
//   (C) Copyright 2021 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef GPIOFLASHER_H
#define GPIOFLASHER_H

#include <QObject>
#include <QString>
#include <QTimer>

#include <sy/sygpio_server.h>
#include <sy/sylwrp_client.h>

#include "config.h"

class GpioFlasher : public QObject
{
 Q_OBJECT;
 public:
  GpioFlasher(QObject *parent=0);
  ~GpioFlasher();
  void addGpio(Config::TetherRole role,SyLwrpClient *lwrp,
	       SyGpioBundleEvent::Type type,int slot,const QString &code);
  bool isActive() const;
  void setActive(bool state);

 private slots:
  void timeoutData();

 protected:
  bool d_state;
  QTimer *d_timer;
  SyLwrpClient * d_lwrps[2];
  SyGpioBundleEvent::Type d_types[2];
  int d_slots[2];
  QString d_codes[2];
};


#endif  // GPIOFLASHER_H
