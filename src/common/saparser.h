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
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

#define SAPARSER_STARTUP_INTERVAL 1000
#define SAPARSER_HOLDOFF_INTERVAL 5000

class SaParser : public QObject
{
 Q_OBJECT
 public:
  enum ConnectionState {Ok=0,InvalidLogin=1,WatchdogActive=2};
  SaParser(QObject *parent=0);
  ~SaParser();
  QMap<int,QString> routers() const;
  bool isConnected() const;
  bool gpioSupported(int router) const;
  int inputQuantity(int router) const;
  QString inputNodeName(int router,int input) const;
  QString inputName(int router,int input) const;
  QString inputLongName(int router,int input) const;
  int outputQuantity(int router) const;
  QString outputName(int router,int output) const;
  QString outputLongName(int router,int output) const;
  int outputCrosspoint(int router,int output) const;
  void setOutputCrosspoint(int router,int output,int input);
  QString gpiState(int router,int input) const;
  void setGpiState(int router,int input,const QString &code,int msec=-1);
  QString gpoState(int router,int output) const;
  void setGpoState(int router,int output,const QString &code,int msec=-1);
  int snapshotQuantity(int router) const;
  QString snapshotName(int router,int n) const;
  void activateSnapshot(int router,const QString &snapshot);
  void connectToHost(const QString &hostname,uint16_t port,
		     const QString &username,const QString &passwd);
  static QString connectionStateString(ConnectionState cstate);

 signals:
  void connected(bool state,SaParser::ConnectionState code);
  void error(QAbstractSocket::SocketError err);
  void routerListChanged();
  void inputListChanged();
  void outputListChanged();
  void outputCrosspointChanged(int router,int output,int input);
  void gpiStateChanged(int router,int input,const QString &code);
  void gpoStateChanged(int router,int output,const QString &code);

 private slots:
  void connectedData();
  void connectionClosedData();
  void startupData();
  void holdoffReconnectData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);

 private:
  void DispatchCommand(QString cmd);
  void ReadRouterName(const QString &cmd);
  void ReadSourceName(const QString &cmd);
  void ReadDestName(const QString &cmd);
  void ReadSnapshotName(const QString &cmd);
  void BubbleSort(std::map<unsigned,QString> *names,
		  std::vector<unsigned> *ptrs);
  void SendCommand(const QString &cmd);
  void MakeSocket();
  QTcpSocket *sa_socket;
  QString sa_hostname;
  uint16_t sa_port;
  QString sa_username;
  QString sa_password;
  bool sa_connected;
  QString sa_accum;
  bool sa_reading_routers;
  bool sa_reading_sources;
  bool sa_reading_dests;
  bool sa_reading_xpoints;
  bool sa_reading_snapshots;
  int sa_last_xpoint_router;
  int sa_last_xpoint_output;
  QMap<int,QString> sa_router_names;
  int sa_current_router;
  int sa_last_router;
  QMap<int,QMap<int,QString> > sa_input_node_names;
  QMap<int,QMap<int,QString> > sa_input_names;
  QMap<int,QMap<int,QString> > sa_input_long_names;
  QMap<int,QMap<int,QString> > sa_output_node_names;
  QMap<int,QMap<int,QString> > sa_output_names;
  QMap<int,QMap<int,QString> > sa_output_long_names;
  QMap<int,QMap<int,int> > sa_output_xpoints;
  QMap<int,QMap<int,QString> > sa_gpi_states;
  QMap<int,QMap<int,QString> > sa_gpo_states;
  QMap<int,bool> sa_gpio_supporteds;
  QMap<int,QStringList> sa_snapshot_names;
  QTimer *sa_startup_timer;
  QTimer *sa_holdoff_timer;
};


#endif  // SAPARSER_H
