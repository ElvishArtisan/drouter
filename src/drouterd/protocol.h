// protocol.h
//
// Base class for drouterd(8) protocols
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include <sy/sylwrp_client.h>

class Protocol : public QObject
{
 Q_OBJECT;
 public:
  Protocol(QObject *parent=0);
  bool startIpc(QString *err_msg);
  void clearCrosspoint(const QHostAddress &node_addr,int slotnum);
  void clearGpioCrosspoint(const QHostAddress &node_addr,int slotnum);
  void setCrosspoint(const QHostAddress &dst_node_addr,int dst_slotnum,
		     const QHostAddress &src_node_addr,int src_slotnum);
  void setGpioCrosspoint(const QHostAddress &gpo_node_addr,int gpo_slotnum,
			 const QHostAddress &gpi_node_addr,int gpi_slotnum);
  void setGpiState(const QHostAddress &gpi_node_addr,int gpi_slotnum,
		   const QString &code);
  void setGpoState(const QHostAddress &gpo_node_addr,int gpo_slotnum,
		   const QString &code);

 private slots:
  void ipcReadyReadData();
  void shutdownTimerData();

 protected:
  virtual void nodeAdded(const QHostAddress &host_addr);
  virtual void nodeRemoved(const QHostAddress &host_addr,
			   int srcs,int dsts,int gpis,int gpos);
  virtual void nodeChanged(const QHostAddress &host_addr);
  virtual void sourceChanged(const QHostAddress &host_addr,int slotnum);
  virtual void destinationChanged(const QHostAddress &host_addr,int slotnum);
  virtual void destinationCrosspointChanged(const QHostAddress &host_addr,int slotnum);
  virtual void gpiChanged(const QHostAddress &host_addr,int slotnum);
  virtual void gpiCodeChanged(const QHostAddress &host_addr,int slotnum);
  virtual void gpoChanged(const QHostAddress &host_addr,int slotnum);
  virtual void gpoCrosspointChanged(const QHostAddress &host_addr,int slotnum);
  virtual void gpoCodeChanged(const QHostAddress &host_addr,int slotnum);
  virtual void clipChanged(const QHostAddress &host_addr,int slotnum,
			   SyLwrpClient::MeterType meter_type,
			   const QString &tbl_name,int chan);
  virtual void silenceChanged(const QHostAddress &host_addr,int slotnum,
			      SyLwrpClient::MeterType meter_type,
			      const QString &tbl_name,int chan);
  void quit();

 private:
  void ProcessIpcCommand(const QString &cmd);
  QTcpSocket *proto_ipc_socket;
  QString proto_ipc_accum;
  QTimer *proto_shutdown_timer;
};


#endif  // PROTOCOL_H
