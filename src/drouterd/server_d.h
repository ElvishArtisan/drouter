// server_d.h
//
// Software Authority Protocol Server for lwpathd(8).
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SERVER_D_H
#define SERVER_D_H

#include <map>

#include "server_net.h"

#define SERVER_D_PORT 23883

class ServerDConnection
{
 public:
  ServerDConnection();
  bool dstsSubscribed() const;
  void setDstsSubscribed(bool state);
  bool gpisSubscribed() const;
  void setGpisSubscribed(bool state);
  bool gposSubscribed() const;
  void setGposSubscribed(bool state);
  bool nodesSubscribed() const;
  void setNodesSubscribed(bool state);
  bool srcsSubscribed() const;
  void setSrcsSubscribed(bool state);

 private:
  bool dsts_subscribed;
  bool gpis_subscribed;
  bool gpos_subscribed;
  bool nodes_subscribed;
  bool srcs_subscribed;
};




class ServerD : public ServerNet
{
 Q_OBJECT;
 public:
  ServerD(int sock,QObject *parent=0);
  ~ServerD();

 signals:
  void processListDestinations(int id);
  void processListGpis(int id);
  void processListGpos(int id);
  void processListNodes(int id);
  void processListSources(int id);
  void processListClips(int id);
  void processListSilences(int id);
  void processSubscribeDestinations(int id);
  void processSubscribeGpis(int id);
  void processSubscribeGpos(int id);
  void processSubscribeNodes(int id);
  void processSubscribeSources(int id);
  void processClearCrosspoint(int id,
			      const QHostAddress &dst_hostaddr,int dst_slot);
  void processClearGpioCrosspoint(int id,const QHostAddress &gpo_hostaddr,
				  int gpo_slot);
  void processSetCrosspoint(int id,
			    const QHostAddress &dst_hostaddr,int dst_slot,
			    const QHostAddress &src_hostaddr,int src_slot);
  void processSetGpioCrosspoint(int id,
				const QHostAddress &gpo_hostaddr,int gpo_slot,
				const QHostAddress &gpi_hostaddr,int gpi_slot);

 protected:
  void newConnection(int id,NetConnection *conn);
  void aboutToCloseConnection(int id,NetConnection *conn);
  void processCommand(int id,const SyAString &cmd);
};


#endif  // SERVER_D_H
