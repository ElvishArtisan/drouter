// vgpionode.h
//
// Virtual GPIO node
//
//   (C) Copyright 2023 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef VGPIONODE_H
#define VGPIONODE_H

#include <stdint.h>

#include <QHostAddress>
#include <QObject>

#include <sy5/syadv_server.h>
#include <sy5/sygpio_server.h>
#include <sy5/sylwrp_server.h>
#include <sy5/syrouting.h>

#include "config.h"

#define VGPIONODE_USAGE "[options]\n"

class VGpioNode : public QObject
{
 Q_OBJECT;
 public:
  VGpioNode(int slot_quan,const QHostAddress &iface_addr,
	    const QHostAddress &iface_mask,QObject *parent=0);
  ~VGpioNode();
  SyRouting::GpoMode gpoMode(int slot) const;
  void setGpoMode(int slot,SyRouting::GpoMode gpo_mode);
  void setGpoAddress(int slot,const QHostAddress &saddr);
  void setGpoAddress(int slot,int srca);

 private slots:
  void gpiStateSetData(int slot,const QString &code);
  void gpoStateSetData(int slot,const QString &code);

 private:
  SyLwrpServer *d_lwrp_server;
  SyGpioServer *d_gpio_server;
  SyAdvServer *d_adv_server;
  SyRouting *d_routing;
  QString d_gpi_code;
};


#endif  // VGPIONODE_H
