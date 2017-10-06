// saparser.h
//
// Parser for SoftwareAuthority Protocol
//
//   (C) Copyright 2016-2017 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#ifndef SAPARSER_H
#define SAPARSER_H

#include <stdint.h>

#include <map>
#include <vector>

#include <QList>
#include <QObject>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

#define SAPARSER_HOLDOFF_INTERVAL 5000

class SaParser : public QObject
{
 Q_OBJECT
 public:
  SaParser(QObject *parent=0);
  ~SaParser();
  int routerQuantity() const;
  QString routerName(int router) const;
  int inputQuantity() const;
  QString inputName(int input);
  QString inputLongName(int input);
  int outputQuantity() const;
  QString outputName(int output);
  QString outputLongName(int output);
  int outputCrosspoint(int output);
  void setOutputCrosspoint(int output,int input);
  void connectToHost(unsigned matrix_num,const QString &hostname,uint16_t port,
		     const QString &username,const QString &passwd);
  void connectToHost(unsigned matrix_num,const QString &hostname,uint16_t port,
		     const QString &username,const QString &passwd,
		     const QList<unsigned> &outputs);

 signals:
  void connected(bool state);
  void routerListChanged();
  void inputListChanged();
  void outputListChanged();
  void outputCrosspointChanged(unsigned output,unsigned input);

 private slots:
  void connectedData();
  void connectionClosedData();
  void holdoffReconnectData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);

 private:
  void DispatchCommand(const QString &cmd);
  void ReadRouterName(const QString &cmd);
  void ReadSourceName(const QString &cmd);
  void ReadDestName(const QString &cmd);
  void BubbleSort(std::map<unsigned,QString> *names,
		  std::vector<unsigned> *ptrs);
  void SendCommand(const QString &cmd);
  void MakeSocket();
  QTcpSocket *sa_socket;
  QString sa_hostname;
  uint16_t sa_port;
  QString sa_username;
  QString sa_password;
  unsigned sa_matrix_number;
  QString sa_accum;
  bool sa_reading_routers;
  bool sa_reading_sources;
  bool sa_reading_dests;
  QStringList sa_router_names;
  QStringList sa_input_names;
  QStringList sa_input_long_names;
  QStringList sa_output_names;
  QStringList sa_output_long_names;
  std::map<unsigned,unsigned> sa_output_xpoints;
  QList<unsigned> sa_active_outputs;
  QTimer *sa_holdoff_timer;
};


#endif  // SAPARSER_H
