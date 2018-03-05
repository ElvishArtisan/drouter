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
  ProtocolD(DRouter *router,int sock,QObject *parent=0);

 private slots:
  void processListClipsD(int id);
  void processListSilencesD(int id);
  void processListDestinationsD(int id);
  void processListGpisD(int id);
  void processListGposD(int id);
  void processListNodesD(int id);
  void processListSourcesD(int id);
  void processSubscribeDestinationsD(int id);
  void processSubscribeGpisD(int id);
  void processSubscribeGposD(int id);
  void processSubscribeNodesD(int id);
  void processSubscribeSourcesD(int id);
  void processSubscribeClipsD(int id);
  void processSubscribeSilencesD(int id);
  void processClearCrosspointD(int id,
			       const QHostAddress &dst_hostaddr,int dst_slot);
  void processClearGpioCrosspointD(int id,const QHostAddress &gpo_hostaddr,
				   int gpo_slot);
  void processSetCrosspointD(int id,
			     const QHostAddress &dst_hostaddr,int dst_slot,
			     const QHostAddress &src_hostaddr,int src_slot);
  void processSetGpioCrosspointD(int id,
				 const QHostAddress &gpo_hostaddr,int gpo_slot,
				 const QHostAddress &gpi_hostaddr,int gpi_slot);
  void processSetGpoStateD(int id,const QHostAddress &gpo_hostaddr,int gpo_slot,
			   const QString &code);
  void processSetGpiStateD(int id,const QHostAddress &gpi_hostaddr,int gpi_slot,
			   const QString &code);

 protected:
  void processAddedNode(const SyNode &node);
  void processAboutToBeRemovedNode(const SyNode &node);
  void processChangedSource(const SyNode &node,int slot,const SySource &src);
  void processChangedDestination(const SyNode &node,int slot,
				 const SyDestination &dst);
  void processChangedGpi(const SyNode &node,int slot,const SyGpioBundle &gpi);
  void processChangedGpo(const SyNode &node,int slot,const SyGpo &gpo);
  void processClipAlarm(const SyNode &node,int slot,
			SyLwrpClient::MeterType type,int chan,bool state);
  void processSilenceAlarm(const SyNode &node,int slot,
			   SyLwrpClient::MeterType type,int chan,bool state);

 private:
  QString AlarmRecord(const QString &keyword,const QHostAddress &hostaddr,
		      int slot,SyLwrpClient::MeterType type,int chan,
		      bool state);
  QString DestinationRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
			    SyDestination *dst) const;
  QString DestinationRecord(const QString &keyword,const SyNode &node,int slot,
			    const SyDestination &dst) const;
  QString GpiRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
		    SyGpioBundle *gpi);
  QString GpiRecord(const QString &keyword,const SyNode &node,int slot,
		    const SyGpioBundle &gpi);
  QString GpoRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
		    SyGpo *gpo);
  QString GpoRecord(const QString &keyword,const SyNode &node,int slot,
		    const SyGpo &gpo);
  QString NodeRecord(const QString &keyword,SyLwrpClient *lwrp) const;
  QString NodeRecord(const QString &keyword,const SyNode &node) const;
  QString SourceRecord(const QString &keyword,SyLwrpClient *lwrp,int slot,
		       SySource *src) const;
  QString SourceRecord(const QString &keyword,const SyNode &node,int slot,
		       const SySource &src) const;
  ServerD *d_server;
};


#endif  // PROTOCOL_D_H
