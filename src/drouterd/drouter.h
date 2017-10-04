// drouter.h
//
// Dynamic router for Livewire networks
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DROUTER_H
#define DROUTER_H

#include <QList>
#include <QMap>
#include <QObject>

#include <sy/sylwrp_client.h>
#include <sy/symcastsocket.h>
#include <sy/synode.h>

#include "drouter.h"

class DRouter : public QObject
{
 Q_OBJECT;
 public:
  DRouter(QObject *parent=0);
  QList<QHostAddress> nodeHostAddresses() const;
  SyLwrpClient *node(const QHostAddress &hostaddr);
  SyLwrpClient *nodeBySrcStream(const QHostAddress &strmaddress,int *slot);
  SySource *src(int srcnum) const;
  SySource *src(const QHostAddress &hostaddr,int slot) const;
  SyDestination *dst(const QHostAddress &hostaddr,int slot) const;
  SyGpioBundle *gpi(const QHostAddress &hostaddr,int slot) const;
  SyGpo *gpo(const QHostAddress &hostaddr,int slot) const;

 public slots:
  bool clearCrosspoint(const QHostAddress &dst_hostaddr,int dst_slot);
  bool setCrosspoint(const QHostAddress &dst_hostaddr,int dst_slot,
		     const QHostAddress &src_hostaddr,int src_slot);
  bool clearGpioCrosspoint(const QHostAddress &gpo_hostaddr,int gpo_slot);
  bool setGpioCrosspoint(const QHostAddress &gpo_hostaddr,int gpo_slot,
			 const QHostAddress &gpi_hostaddr,int gpi_slot);

 signals:
  void nodeAdded(const SyNode &node);
  void nodeAboutToBeRemoved(const SyNode &node);
  void srcChanged(const SyNode &node,int slot,const SySource &src);
  void dstChanged(const SyNode &node,int slot,const SyDestination &dst);
  void gpiChanged(const SyNode &node,int slot,const SyGpioBundle &gpi);
  void gpoChanged(const SyNode &node,int slot,const SyGpo &gpo);

 private slots:
  void nodeConnectedData(unsigned id,bool state);
  void sourceChangedData(unsigned id,int slotnum,const SyNode &node,
			 const SySource &src);
  void destinationChangedData(unsigned id,int slotnum,const SyNode &node,
			      const SyDestination &dst);
  void gpiChangedData(unsigned id,int slotnum,const SyNode &node,
		      const SyGpioBundle &gpi);
  void gpoChangedData(unsigned id,int slotnum,const SyNode &node,
		      const SyGpo &gpo);
  void advtReadyReadData(int ifnum);

 private:
  QMap<unsigned,SyLwrpClient *> drouter_nodes;
  QList<SyMcastSocket *> drouter_advt_sockets;
};


#endif  // DROUTER_H
