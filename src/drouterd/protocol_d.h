// protocol_d.h
//
// Protocol "D" for DRouter
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

#ifndef PROTOCOL_D_H
#define PROTOCOL_D_H

#include "protocol.h"
#include "server_d.h"

class ProtocolD : public Protocol
{
 Q_OBJECT;
 public:
  ProtocolD(DRouter *router,QObject *parent=0);

 private slots:
  void processListDestinationsD(int id);
  void processListNodesD(int id);
  void processListSourcesD(int id);
  void processSubscribeDestinationsD(int id);
  void processSubscribeNodesD(int id);
  void processSubscribeSourcesD(int id);
  void processClearCrosspointD(int id,
			       const QHostAddress &dst_hostaddr,int dst_slot);
  void processSetCrosspointD(int id,
			     const QHostAddress &dst_hostaddr,int dst_slot,
			     const QHostAddress &src_hostaddr,int src_slot);

 protected:
  void processAddedNode(const SyNode &node);
  void processAboutToBeRemovedNode(const SyNode &node);
  void processChangedSource(const SyNode &node,int slot,const SySource &src);
  void processChangedDestination(const SyNode &node,int slot,
				 const SyDestination &dst);

 private:
  QString DestinationRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
			    SyDestination *dst) const;
  QString DestinationRecord(const QString &keyword,const SyNode &node,int slot,
			    const SyDestination &dst) const;
  QString NodeRecord(const QString &keyword,SyLwrpClient *lwrp) const;
  QString NodeRecord(const QString &keyword,const SyNode &node) const;
  QString SourceRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
		       SySource *src) const;
  QString SourceRecord(const QString &keyword,const SyNode &node,int slot,
		       const SySource &src) const;
  ServerD *d_server;
};


#endif  // PROTOCOL_D_H
