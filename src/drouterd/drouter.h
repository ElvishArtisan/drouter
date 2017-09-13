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
  QList<QHostAddress> nodes() const;
  SyLwrpClient *node(const QHostAddress &hostaddr);
  SySource *src(int srcnum) const;
  SySource *src(const QHostAddress &hostaddr,int slot) const;
  SyDestination *dst(const QHostAddress &hostaddr,int slot) const;

 public slots:
  bool setCrosspoint(const QHostAddress &dst_hostaddr,int dst_slot,
		     const QHostAddress &src_hostaddr,int src_slot);

 signals:
  void nodeAdded(const SyNode &node);
  void nodeAboutToBeRemoved(const SyNode &node);
  void srcChanged(const SyNode &node,int slot,const SySource &src);
  void dstChanged(const SyNode &node,int slot,const SyDestination &dst);

 private slots:
  void nodeConnectedData(unsigned id,bool state);
  void sourceChangedData(unsigned id,int slotnum,const SyNode &node,
			 const SySource &src);
  void destinationChangedData(unsigned id,int slotnum,const SyNode &node,
			      const SyDestination &dst);
  void advtReadyReadData();

 private:
  QMap<unsigned,SyLwrpClient *> drouter_nodes;
  SyMcastSocket *drouter_advt_socket;
};


#endif  // DROUTER_H
