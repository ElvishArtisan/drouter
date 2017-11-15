// server_sa.h
//
// Software Authority Protocol Server for drouterd(8).
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

#ifndef SERVER_SA_H
#define SERVER_SA_H

#include <QMap>

#include "server_net.h"

#define SERVER_SA_PORT 9500
#define SERVER_SA_CONFIG_DIR "/etc/drouter.conf.d"

class ServerSa : public ServerNet
{
 Q_OBJECT;
 public:
  ServerSa(int sock,QObject *parent=0);
  ~ServerSa();

 signals:
  void sendMatrixNames(int id);
  void sendInputNames(int id,unsigned cardnum);
  void sendOutputNames(int id,unsigned cardnum);
  void sendGpiInfo(int id,unsigned cardnum,int input);
  void sendGpoInfo(int id,unsigned cardnum,int output);
  void setRoute(int id,unsigned cardnum,unsigned input,unsigned output);
  void sendRouteInfo(int id,unsigned cardnum,unsigned output);
  void setGpiState(int id,unsigned cardnum,unsigned input,int msecs,
		   const QString &code);
  void setGpoState(int id,unsigned cardnum,unsigned input,int msecs,
		   const QString &code);
  void sendSnapshotNames(int id,unsigned cardnum);
  void activateSnapshot(int id,unsigned cartnum,const QString &snapshot);

 protected:
  void processCommand(int id,const SyAString &cmd);

 private:
  void LoadHelp();
  QMap<QString,QString> sa_help_strings;
};


#endif  // SERVER_SA_H
