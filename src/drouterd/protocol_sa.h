// protocol_sa.h
//
// Software Authority Protocol
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

#ifndef PROTOCOL_SA_H
#define PROTOCOL_SA_H

#include "endpointmap.h"
#include "protocol.h"
#include "server_sa.h"

class ProtocolSa : public Protocol
{
 Q_OBJECT;
 public:
 ProtocolSa(DRouter *router,int sock,QObject *parent=0);
 void reload();

 public slots:
  void inputNameChanged(int mid,unsigned input,const QString &name);
  void gpiChanged(int mid,unsigned input,const QString &code);
  void outputNameChanged(int mid,unsigned output,const QString &name);
  void outputCrosspointChanged(int mid,unsigned output,unsigned input);
  void gpoChanged(int mid,unsigned output,const QString &code);

 protected:
  void processChangedDestination(const SyNode &node,int slot,
				 const SyDestination &dst);
  void processChangedGpi(const SyNode &node,int slot,const SyGpioBundle &gpi);
  void processChangedGpo(const SyNode &node,int slot,const SyGpo &gpo);

 private slots:
  void sendMatrixNamesSa(int id);
  void sendInputNamesSa(int id,unsigned cardnum);
  void sendOutputNamesSa(int id,unsigned cardnum);
  void setRouteSa(int id,unsigned cardnum,unsigned input,unsigned output);
  void sendRouteInfoSa(int id,unsigned cardnum,unsigned output);
  void sendGpiInfoSa(int id,unsigned cardnum,int input);
  void sendGpoInfoSa(int id,unsigned cardnum,int output);
  void setGpiStateSa(int id,unsigned cardnum,unsigned input,int msecs,
		     const QString &code);
  void setGpoStateSa(int id,unsigned cardnum,unsigned input,int msecs,
		     const QString &code);
  void sendSnapshotNamesSa(int id,unsigned cardnum);
  void activateSnapshotSa(int id,unsigned cartnum,const QString &snapshot);

 private:
  int GetCrosspointInput(EndPointMap *map,int output) const;
  void SetRoute(int id,EndPointMap *map,int output,int input);
  ServerSa *sa_server;
  QMap<int,EndPointMap *> sa_maps;
  QTimer *sa_ready_timer;
};


#endif  // PROTOCOL_SA_H
