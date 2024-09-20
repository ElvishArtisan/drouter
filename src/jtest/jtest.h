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
#include <QTcpSocket>
#include <QTextStream>

#include "jsontest.h"

#define JTEST_USAGE "[--hostname=<ip-addr>[:<port-num>]] [--connection-type=TCP|WebSocket] --tests=<filename>\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void testCompleteData(int testnum,const QString &testname,bool passed,
			const QString &err_msg);
  
 private:
  void Finish();
  QTextStream *d_tests_stream;
  JsonTest *d_json_test;
  int d_linenum;
  int d_passed;
  int d_failed;
};


#endif  // JTEST_H
