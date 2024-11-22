// gvg7000mapper.h
//
// dmap(8) map utility
//
//   (C) Copyright 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef GVG7000MAPPER_H
#define GVG7000MAPPER_H

#include <QTcpSocket>

#include <drouter/drendpointmap.h>

class Gvg7000Mapper : public QObject
{
 Q_OBJECT;
 public:
  Gvg7000Mapper(DREndPointMap::RouterType type,const QString &name,int number,
		QObject *parent=0);
  ~Gvg7000Mapper();
  DREndPointMap *endpointMap() const;
  void readDevice(const QString &hostname,uint16_t port);

 signals:
  void completed(bool result,const QString &err_msg);
  
 private slots:
  void connectedData();
  void disconnectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);

 private:
  void DispatchGvgCommand(const QByteArray &msg);
  void ProcessGvgCommand(const QByteArray &msg);
  void SendGvgCommand(const QString &str);
  QByteArray ToGvgNative(QString str) const;
  bool GvgVerifyChecksum(const QByteArray &msg) const;
  uint8_t GvgChecksum(const QByteArray &msg) const;
  QString GvgPrettify(const QByteArray &msg) const;
  QString d_device_hostname;
  unsigned d_device_port;
  DREndPointMap *d_map;
  QByteArray d_raw_accum;
  QByteArray d_layer4_accum;
  QTcpSocket *d_socket;
  QString d_err_msg;
};


#endif  // GVG7000MAPPER_H
