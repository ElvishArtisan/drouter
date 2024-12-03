// jtest.h
//
// jtest() Testing harness for ProtocolJ
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef JTEST_H
#define JTEST_H

#include <QJsonDocument>
#include <QTextStream>

#include <drouter/drjsonsocket.h>

#include "jsontest.h"
#include "profile.h"

#define JTEST_USAGE "[--generate-diffs] [--hostname=<ip-addr>[:<port-num>]] [--connection-type=TCP|WebSocket] --tests=<filename>\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void testCompleteData(int testnum,const QString &testname,bool passed,
			const QString &err_msg,const QString &diff);
  void prologueConnectedData();
  void prologueDocumentReceivedData(const QJsonDocument &jdoc);
  void prologueParseErrorData(const QByteArray &json,
			      const QJsonParseError &jerr);
  void prologueErrorData(QAbstractSocket::SocketError err);
  void prologueTimeoutData();
  
 private:
  void Finish();
  JsonTest *d_json_test;
  int d_linenum;
  int d_passed;
  int d_failed;
  bool d_generate_diffs;
  int d_test_number;
  Profile *d_test_profile;
  QStringList d_prologue_commands;
  DRJsonSocket *d_prologue_socket;
  QTimer *d_prologue_timer;
};


#endif  // JTEST_H
