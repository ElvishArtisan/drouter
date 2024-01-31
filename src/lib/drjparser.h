// drjparser.h
//
// Parser for Protocol J Protocol
//
//   (C) Copyright 2016-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DRJPARSER_H
#define DRJPARSER_H

#include <stdint.h>

#include <map>
#include <vector>

#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

#include "drendpointlistmodel.h"
#include "drrouterlistmodel.h"

#define DRJPARSER_STARTUP_INTERVAL 1000
#define DRJPARSER_HOLDOFF_INTERVAL 5000

class DRJParser : public QObject
{
 Q_OBJECT
 public:
  enum ConnectionState {Ok=0,InvalidLogin=1,WatchdogActive=2};
  enum ErrorType {OkError=0,JsonError=1,ParameterError=2,NoRouterError=3,
		  NoSnapshotError=4,NoSourceError=5,NoDestinationError=6,
		  NotGpioRouterError=7,NoCommandError=8,LastError=9};
  DRJParser(bool use_long_names,QObject *parent=0);
  ~DRJParser();
  QMap<int,QString> routers() const;
  //
  // Models
  //
  DRRouterListModel *routerModel() const;
  DREndPointListModel *outputModel(int router) const;
  DREndPointListModel *inputModel(int router) const;

  bool isConnected() const;
  bool gpioSupported(int router) const;
  int inputQuantity(int router) const;
  bool inputIsReal(int router,int input) const;
  QString inputNodeName(int router,int input) const;
  QHostAddress inputNodeAddress(int router,int input) const;
  int inputNodeSlotNumber(int router,int input) const;
  QString inputName(int router,int input) const;
  QString inputLongName(int router,int input) const;
  int inputSourceNumber(int router,int input) const;
  QHostAddress inputStreamAddress(int router,int input) const;
  int outputQuantity(int router) const;
  bool outputIsReal(int router,int output) const;
  QString outputNodeName(int router,int output) const;
  QHostAddress outputNodeAddress(int router,int output) const;
  int outputNodeSlotNumber(int router,int output) const;
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
  static QString errorString(ErrorType err);

 signals:
  void connected(bool state,DRJParser::ConnectionState code);
  void error(QAbstractSocket::SocketError err);
  void parserError(DRJParser::ErrorType err,const QString &remarks);
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
  void Clear();
  void DispatchMessage(const QJsonDocument &jdoc);
  //  void DispatchCommand(QString cmd);
  void ReadRouterName(const QString &cmd);
  void ReadSourceName(const QString &cmd);
  void ReadDestName(const QString &cmd);
  void ReadSnapshotName(const QString &cmd);
  void BubbleSort(std::map<unsigned,QString> *names,
		  std::vector<unsigned> *ptrs);
  void SendCommand(const QString &cmd);
  void MakeSocket();

  DRRouterListModel *j_router_model;
  QMap<int,DREndPointListModel *> j_output_models;
  QMap<int,DREndPointListModel *> j_input_models;

  QTcpSocket *j_socket;
  QString j_hostname;
  uint16_t j_port;
  QString j_username;
  QString j_password;
  bool j_use_long_names;
  bool j_connected;
  QByteArray j_accum;
  bool j_accum_quoted;
  int j_accum_level;
  bool j_reading_routers;
  bool j_reading_sources;
  bool j_reading_dests;
  bool j_reading_xpoints;
  bool j_reading_snapshots;
  int j_last_xpoint_router;
  int j_last_xpoint_output;
  QMap<int,QString> j_router_names;
  int j_current_router;
  int j_last_router;
  int j_prev_input;
  int j_prev_output;
  QMap<int,int> j_input_quantities;
  QMap<int,QMap<int,QString> > j_input_node_names;
  QMap<int,QMap<int,QHostAddress> > j_input_node_addresses;
  QMap<int,QMap<int,int> > j_input_node_slot_numbers;
  QMap<int,QMap<int,bool> > j_input_is_reals;
  QMap<int,QMap<int,QString> > j_input_names;
  QMap<int,QMap<int,QString> > j_input_long_names;
  QMap<int,QMap<int,int> > j_input_source_numbers;
  QMap<int,QMap<int,QHostAddress> > j_input_stream_addresses;
  QMap<int,QMap<int,QString> > j_output_node_names;
  QMap<int,QMap<int,QHostAddress> > j_output_node_addresses;
  QMap<int,QMap<int,int> > j_output_node_slot_numbers;
  QMap<int,int> j_output_quantities;
  QMap<int,QMap<int,bool> > j_output_is_reals;
  QMap<int,QMap<int,QString> > j_output_names;
  QMap<int,QMap<int,QString> > j_output_long_names;
  QMap<int,QMap<int,int> > j_output_xpoints;
  QMap<int,QMap<int,QString> > j_gpi_states;
  QMap<int,QMap<int,QString> > j_gpo_states;
  QMap<int,bool> j_gpio_supporteds;
  QMap<int,QStringList> j_snapshot_names;
  QTimer *j_startup_timer;
  QTimer *j_holdoff_timer;
};


#endif  // DRJPARSER_H
