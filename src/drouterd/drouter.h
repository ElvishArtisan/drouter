// drouter.h
//
// Dynamic router database component for Drouter
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
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
#include <QSignalMapper>
#include <QTcpServer>
#include <QTimer>

//#include <sy5/sylwrp_client.h>
#include <sy5/symcastsocket.h>

#include "matrix.h"
#include "config.h"
#include "endpointmap.h"
#include "gpioflasher.h"

class DRouter : public QObject
{
 Q_OBJECT;
 public:
  DRouter(int *proto_socks,QObject *parent=0);
  ~DRouter();
  QList<QHostAddress> nodeHostAddresses() const;
  Matrix *node(const QHostAddress &hostaddr);
  Matrix *nodeBySrcStream(const QHostAddress &strmaddress,int *slot);
  SySource *src(int srcnum) const;
  SySource *src(const QHostAddress &hostaddr,int slot) const;
  SyDestination *dst(const QHostAddress &hostaddr,int slot) const;
  bool clipAlarmActive(const QHostAddress &hostaddr,int slot,
		       SyLwrpClient::MeterType type,int chan) const;
  bool silenceAlarmActive(const QHostAddress &hostaddr,int slot,
			  SyLwrpClient::MeterType type,int chan) const;
  SyGpioBundle *gpi(const QHostAddress &hostaddr,int slot) const;
  SyGpo *gpo(const QHostAddress &hostaddr,int slot) const;
  bool start(QString *err_msg);
  bool isWriteable() const;

 public slots:
   void setWriteable(bool state);

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
  void audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
			  unsigned slotnum,int chan,bool state);
  void audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
			     unsigned slotnum,int chan,bool state);
  void advtReadyReadData(int ifnum);
  void newIpcConnectionData(int listen_sock);
  void ipcReadyReadData(int sock);
  void finalizeEventsData();
  void purgeEventsData();

 private:
  void NotifyProtocols(const QString &type,const QString &id,
		       int srcs=-1,int dsts=-1,int gpis=-1,int gpos=-1);
  bool StartProtocolIpc(QString *err_msg);
  bool ProcessIpcCommand(int sock,const QString &cmd);
  bool StartDb(QString *err_msg);
  bool StartStaticMatrices(QString *err_msg);
  bool StartLivewire(QString *err_msg);
  Matrix *StartMatrix(Config::MatrixType type,unsigned id);
  void LockTables() const;
  void UnlockTables() const;
  void LoadMaps();
  void SendProtoSocket(int dest_sock,int proto_sock);
  void Log(int prio,const QString &msg) const;
  void FinalizeSAAudioRoute(int event_id,int router,int output,int input);
  void FinalizeSAGpioRoute(int event_id,int router,int output,int input);
  void FinalizeSARouteEvent(int event_id,bool status) const;
  void WriteCommentEvent(const QString &str) const;
  QMap<unsigned,Matrix *> drouter_nodes;
  QList<SyMcastSocket *> drouter_advt_sockets;
  QMap<int,QTcpSocket *> drouter_ipc_sockets;
  QMap<int,QString> drouter_ipc_accums;
  QMap<int,EndPointMap *> drouter_maps;
  QSignalMapper *drouter_ipc_ready_mapper;
  QTcpServer *drouter_ipc_server;
  int *drouter_proto_socks;
  bool drouter_writeable;
  GpioFlasher *drouter_flasher;
  QTimer *drouter_finalize_timer;
  QTimer *drouter_purge_events_timer;
  Config *drouter_config;
};


#endif  // DROUTER_H
